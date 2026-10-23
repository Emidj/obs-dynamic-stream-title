// SPDX-License-Identifier: GPL-2.0-or-later
#include "state/shared-state.hpp"
#include "events/event-engine.hpp"
#include "events/simulation.hpp"
#include <atomic>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <thread>
using namespace oto;
namespace {
void require(bool ok, const char *why) { if (!ok) throw std::runtime_error(why); }
events::EventPtr fixture(events::EventType type, int sequence = 1, std::string channel = "channel", int when = 1000)
{
	events::EventEngine engine; engine.start();
	auto draft = events::makeSimulation(type, sequence, std::chrono::system_clock::time_point(std::chrono::seconds(when)));
	draft.channelId = std::move(channel);
	return engine.process(std::move(draft)).event;
}
}
int main()
{
	try {
		state::TextCache cache(2, 8);
		const auto a = cache.intern("aa"), b = cache.intern("bb");
		require(cache.intern("aa").get() == a.get(), "cache did not share text");
		cache.intern("cc");
		require(cache.stats().entries == 2 && cache.stats().evictions == 1, "LRU entry bound");
		require(cache.intern("bb").get() != b.get() && *b == "bb", "LRU or retained handle invalid");
		cache.intern("12345678");
		require(cache.stats().entries == 1 && cache.stats().bytes == 8, "byte bound");
		cache.intern("123456789"); require(cache.stats().entries == 1, "oversize text evicted cache");
		require(cache.invalidate("12345678") && !cache.invalidate("12345678"), "invalidation");
		cache.clear(); require(cache.stats().entries == 0 && *a == "aa", "clear invalidated retained text");
		state::TextCache names;
		state::SharedState shared;
		require(!shared.snapshot(), "state active before start");
		shared.start(); const auto empty = shared.snapshot(); shared.start();
		require(shared.snapshot() == empty && shared.prepare({}, names) == empty, "idle state allocates/publishes");
		const auto follow = fixture(events::EventType::Follow);
		const auto sub = fixture(events::EventType::Sub, 2);
		const auto draft = shared.prepare({follow, sub, fixture(events::EventType::Cheer, 3), fixture(events::EventType::Gift, 4),
			fixture(events::EventType::Raid, 5), fixture(events::EventType::Chat, 6), fixture(events::EventType::HypeTrain, 7), fixture(events::EventType::Shoutout, 8), fixture(events::EventType::ChatModeration, 9)}, names);
		require(shared.snapshot() == empty && empty->channels.empty(), "prepare mutated published state");
		shared.publish(draft);
		const auto channel = draft->channels.begin()->second;
		require(channel->sessionBits == 100 && channel->sessionSubs == 6 && channel->sessionGifts == 5, "wrong session counters");
		for (auto count : channel->delivered) require(count == 1, "event type not reduced");
		require(channel->latest[0].event.get() == follow.get(), "state copied event");
		require(channel->latest[0].displayName == channel->latest[1].displayName && names.stats().entries == 1, "names not shared");
		const auto older = fixture(events::EventType::Follow, 8, "channel", 999);
		shared.publish(shared.prepare({older}, names));
		const auto late = shared.snapshot()->channels.begin()->second;
		require(late->latest[0].event == follow && late->delivered[0] == 2 && late->latest[0].revision == 1,
			"late event regressed latest data/revision");
		require(channel->delivered[0] == 1 && draft->revision == 1, "old snapshot mutated");
		shared.publish(shared.prepare({fixture(events::EventType::Follow, 9, "other")}, names));
		require(shared.snapshot()->channels.at({events::EventOrigin::Simulation, "channel"}) == late, "unchanged channel copied");
		state::SharedState bounded(1); bounded.start();
		const auto beforeFailure = bounded.snapshot();
		bool failed = false;
		try { bounded.prepare({follow, fixture(events::EventType::Follow, 10, "other")}, names); }
		catch (const std::length_error &) { failed = true; }
		require(failed && bounded.snapshot() == beforeFailure, "capacity failure partially published");
		events::EventEngine engine; engine.start();
		auto huge = fixture(events::EventType::Cheer)->data();
		std::get<events::CheerEvent>(huge.payload).bits = std::numeric_limits<std::uint64_t>::max();
		std::get<events::CheerEvent>(huge.payload).user.reset();
		shared.publish(shared.prepare({engine.process(huge).event}, names));
		const auto overflowed = shared.snapshot()->channels.at({events::EventOrigin::Simulation, "channel"});
		require(overflowed->saturated && overflowed->sessionBits == std::numeric_limits<std::uint64_t>::max(), "counter wrapped");
		require(!overflowed->latest[state::slot(events::EventType::Cheer)].displayName, "anonymous user invented");
		// Concurrent readers only load immutable snapshots; production creates no threads.
		std::atomic<bool> done{false}, valid{true};
		std::thread reader([&] {
			while (!done.load()) {
				const auto view = shared.snapshot();
				if (view && !view->channels.empty() && view->channels.begin()->second->revision > view->revision) valid = false;
			}
		});
		for (int i = 0; i < 100; ++i) shared.publish(shared.prepare({fixture(events::EventType::Follow, i+20)}, names));
		shared.stop(); done = true; reader.join();
		require(valid && !shared.snapshot() && !draft->channels.empty(), "concurrent publication/retained snapshot");
		shared.start(); require(shared.snapshot()->channels.empty() && shared.snapshot()->generation != draft->generation,
			"restart retained session data or reused generation");
		std::cout << "PASS: immutable snapshots, channel isolation, revisions, counters, late events, saturation, bounded LRU, sharing, lifecycle, concurrent readers\n";
		return 0;
	} catch (const std::exception &error) { std::cerr << error.what() << '\n'; return 1; }
}
