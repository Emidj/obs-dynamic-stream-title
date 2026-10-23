// SPDX-License-Identifier: GPL-2.0-or-later
#include "core/plugin-core.hpp"

#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
void require(bool condition, const char *message)
{
	if (!condition)
		throw std::runtime_error(message);
}

struct MemoryStore final : oto::IConfigStore {
	oto::ConfigLoadResult next{oto::ConfigStatus::Loaded, {false}};
	int loads = 0;
	int saves = 0;
	bool failSave = false;
	oto::Settings saved;
	oto::ConfigLoadResult load() override { ++loads; return next; }
	bool save(oto::Settings value) override { ++saves; saved = value; return !failSave; }
};

struct LogSink final : oto::ILogger {
	mutable std::vector<std::string> messages;
	void write(oto::LogLevel, std::string_view message) const noexcept override
	{
		messages.emplace_back(message);
	}
};
} // namespace

int main()
{
	try {
		auto store = std::make_unique<MemoryStore>();
		auto sink = std::make_unique<LogSink>();
		auto *storage = store.get();
		auto *logging = sink.get();
		auto core = std::make_shared<oto::PluginCore>(std::move(store), std::move(sink));
		std::shared_ptr<const oto::IPluginServices> consumerA = core;
		std::shared_ptr<const oto::IPluginServices> consumerB = core;
		require(core->setDiagnosticsEnabled(true) == oto::SettingsUpdate::NotRunning, "write before start");
		core->start();
		core->start();
		require(storage->loads == 1 && storage->saves == 0, "start is not idempotent");
		const auto simulated = core->simulateEvent(oto::events::EventType::Follow);
		const auto second = core->simulateEvent(oto::events::EventType::Follow);
		require(simulated.event && second.event && simulated.event->data().id != second.event->data().id,
			"Core simulation ingress or identifiers failed");
		auto invalidEvent = simulated.event->data();
		invalidEvent.channelId.clear();
		require(core->processEvent(invalidEvent).error == oto::events::EventError::InvalidId, "Core bypasses event validation");
		require(storage->saves == 0, "simulation persisted configuration or event history");
		require(core->pendingEventCount() == 2, "simulation bypassed central queue or invalid event queued");
		const auto repeated = core->processEvent(simulated.event->data());
		require(!repeated && repeated.queue.status == oto::execution::EnqueueStatus::Duplicate, "Core ingress bypasses deduplication");
		const auto beforeBatch = consumerA->sharedSnapshot();
		require(beforeBatch && beforeBatch->channels.empty(), "enqueuing published state too early");
		const auto delivered = core->processPendingEvents(1);
		require(delivered.processed == 1 && delivered.snapshot->channels.begin()->second->latest[0].event.get() == simulated.event.get(),
			"Core state did not share normalized event");
		require(consumerA->sharedSnapshot() == consumerB->sharedSnapshot(), "consumers receive different snapshots");
		require(!core->processEvent(simulated.event->data()) && core->sharedSnapshot() == delivered.snapshot,
			"duplicate delivered event changed shared state");
		require(core->cancelEvent(second.queue.jobId) && core->pendingEventCount() == 0, "Core cancellation failed");
		require(core->processPendingEvents().snapshot == delivered.snapshot, "empty/cancelled batch changed state");
		core->simulateEvent(oto::events::EventType::Sub); core->processPendingEvents();
		require(core->cacheStats().entries == 1 && core->cacheStats().hits == 1, "Core has duplicate name caches");
		core->simulateEvent(oto::events::EventType::Raid);
		require(consumerA.get() == consumerB.get() && consumerA->running(), "services are not shared");
		const auto before = logging->messages.size();
		consumerA->log(oto::LogLevel::Debug, "hidden");
		require(logging->messages.size() == before, "debug logging enabled by default");
		storage->failSave = true;
		require(core->setDiagnosticsEnabled(true) == oto::SettingsUpdate::SaveFailed, "save failure lost");
		require(!consumerB->settings().diagnosticsEnabled, "failed save changed active settings");
		storage->failSave = false;
		require(core->setDiagnosticsEnabled(true) == oto::SettingsUpdate::Applied, "settings not applied");
		require(consumerB->settings().diagnosticsEnabled && storage->saved.diagnosticsEnabled, "shared settings stale");
		const auto saves = storage->saves;
		require(core->setDiagnosticsEnabled(true) == oto::SettingsUpdate::Unchanged && storage->saves == saves,
			"unchanged settings cause disk writes");
		consumerA->log(oto::LogLevel::Debug, "visible");
		require(logging->messages.back() == "visible", "debug logging not enabled");
		core->stop();
		const auto stopped = logging->messages.size();
		require(core->pendingEventCount() == 0 && core->processPendingEvents().processed == 0, "Core stop left pending work");
		require(!consumerA->sharedSnapshot() && core->cacheStats().entries == 0, "Core stop retained shared state/cache");
		require(delivered.snapshot->channels.begin()->second->latest[0].event == simulated.event, "retained snapshot invalidated at stop");
		require(core->simulateEvent(oto::events::EventType::Follow).error == oto::events::EventError::Stopped,
			"simulation accepted after Core stopped");
		require(core->processEvent(simulated.event->data()).error == oto::events::EventError::Stopped,
			"direct ingress accepted after Core stopped");
		require(simulated.event->data().channelId == "sim-channel", "event lifetime tied to Core activity");
		core->stop();
		consumerB->log(oto::LogLevel::Info, "after stop");
		require(logging->messages.size() == stopped && !consumerA->running(), "stop is not idempotent");
		core.reset();
		require(!consumerB->running(), "retained consumer handle unsafe after owner release");

		for (auto status : {oto::ConfigStatus::Invalid, oto::ConfigStatus::UnsupportedVersion, oto::ConfigStatus::Unavailable}) {
			auto invalid = std::make_unique<MemoryStore>();
			auto *record = invalid.get();
			invalid->next = {status, {true}};
			oto::PluginCore guarded(std::move(invalid), std::make_unique<LogSink>());
			guarded.start();
			require(bool(guarded.simulateEvent(oto::events::EventType::Chat)), "protected configuration prevents simulation");
			require(!guarded.settings().diagnosticsEnabled && !guarded.configurationWritable(), "unsafe defaults");
			require(guarded.setDiagnosticsEnabled(true) == oto::SettingsUpdate::ReadOnly && record->saves == 0,
				"invalid configuration overwritten");
		}
		auto missing = std::make_unique<MemoryStore>();
		auto *fresh = missing.get();
		missing->next.status = oto::ConfigStatus::Missing;
		oto::PluginCore firstRun(std::move(missing), std::make_unique<LogSink>());
		firstRun.start();
		require(fresh->saves == 1 && !fresh->saved.diagnosticsEnabled, "default configuration not saved");
		std::cout << "PASS: shared services, lifecycle, logging, defaults, persistence failure, protected config\n";
		// Capacity failure must leave both published state and queue untouched.
		for (int i = 0; i < 17; ++i) {
			auto draft = simulated.event->data(); draft.channelId = "channel-" + std::to_string(i);
			require(bool(firstRun.processEvent(std::move(draft))), "failed to queue channel fixture");
		}
		const auto beforeFailure = firstRun.sharedSnapshot();
		bool failed = false;
		try { firstRun.processPendingEvents(); } catch (const std::length_error &) { failed = true; }
		require(failed && firstRun.pendingEventCount() == 17 && firstRun.sharedSnapshot() == beforeFailure,
			"failed state batch lost work or partially published");
		firstRun.cancelPendingEvents(); firstRun.simulateEvent(oto::events::EventType::Cheer);
		require(firstRun.processPendingEvents().processed == 1, "Core did not recover after failed batch");
		return 0;
	} catch (const std::exception &error) {
		std::cerr << error.what() << '\n';
		return 1;
	}
}
