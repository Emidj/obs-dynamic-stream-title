// SPDX-License-Identifier: GPL-2.0-or-later
#include "execution/execution-queue.hpp"
#include "events/event-engine.hpp"
#include "events/simulation.hpp"
#include <iostream>
#include <stdexcept>

using namespace oto::events;
using namespace oto::execution;
namespace {
void require(bool value, const char *message) { if (!value) throw std::runtime_error(message); }
const auto clockStart = Clock::time_point(std::chrono::seconds(1000));
EventPtr fixture(std::uint64_t id, EventType type = EventType::Follow, std::string channel = "channel",
	HypeTrainStage stage = HypeTrainStage::Progress, int seconds = 1000)
{
	EventEngine engine; engine.start();
	auto draft = makeSimulation(type, id, std::chrono::system_clock::time_point(std::chrono::seconds(seconds)));
	draft.id = "event-" + std::to_string(id);
	draft.channelId = std::move(channel);
	if (type == EventType::HypeTrain) std::get<HypeTrainEvent>(draft.payload).stage = stage;
	return engine.process(std::move(draft)).event;
}
void enqueue(ExecutionQueue &queue, EventPtr event)
{ require(bool(queue.enqueue(std::move(event), clockStart)), "enqueue failed"); }
}
int main()
{
	try {
		ExecutionQueue q;
		require(q.enqueue(fixture(1)).status == EnqueueStatus::Stopped, "accepted before start");
		q.start(); q.start();
		require(q.enqueue(nullptr).status == EnqueueStatus::InvalidEvent, "accepted null event");
		const auto follow = fixture(1);
		const auto first = q.enqueue(follow, clockStart);
		require(bool(first) && q.size() == 1, "first enqueue");
		require(q.enqueue(follow, clockStart).status == EnqueueStatus::Duplicate, "pending duplicate");
		enqueue(q, fixture(2, EventType::Raid));
		enqueue(q, fixture(3));
		const auto batch = q.takeBatch();
		require(batch.size() == 3 && typeOf(batch[0].event->data().payload) == EventType::Raid,
			"priority did not select raid first");
		require(batch[1].event.get() == follow.get() && batch[2].event->data().id == "event-3", "FIFO/sharing failed");
		require(q.enqueue(follow, clockStart + std::chrono::minutes(9)).status == EnqueueStatus::Duplicate, "completed duplicate");
		require(bool(q.enqueue(follow, clockStart + std::chrono::minutes(10))), "dedup expiry");
		require(q.cancelAll() == 1 && q.cancelAll() == 0, "cancel all");
		q.stop(); q.start();
		for (int i = 1; i <= 20; ++i) enqueue(q, fixture(i, EventType::Raid));
		enqueue(q, fixture(21, EventType::Chat));
		enqueue(q, fixture(22));
		bool chatDelivered = false, followDelivered = false;
		for (int i = 0; i < 7; ++i) {
			const auto one = q.takeBatch(1);
			chatDelivered |= typeOf(one[0].event->data().payload) == EventType::Chat;
			followDelivered |= typeOf(one[0].event->data().payload) == EventType::Follow;
		}
		require(chatDelivered && followDelivered, "lower priorities starved across batches");
		q.stop(); q.start();
		const auto progress = q.enqueue(fixture(1, EventType::HypeTrain), clockStart);
		const auto newer = fixture(2, EventType::HypeTrain, "channel", HypeTrainStage::Progress, 1002);
		const auto merged = q.enqueue(newer, clockStart);
		require(merged.status == EnqueueStatus::Coalesced && merged.jobId == progress.jobId && q.size() == 1, "progress not coalesced");
		require(q.enqueue(fixture(3, EventType::HypeTrain), clockStart).status == EnqueueStatus::Coalesced, "stale progress not consumed");
		require(q.enqueue(fixture(1, EventType::HypeTrain), clockStart).status == EnqueueStatus::Duplicate, "coalesced ID forgotten");
		require(q.takeBatch()[0].event.get() == newer.get(), "stale progress replaced newest or copied payload");
		q.stop(); q.start();
		enqueue(q, fixture(1, EventType::HypeTrain));
		enqueue(q, fixture(2, EventType::HypeTrain, "channel", HypeTrainStage::End));
		enqueue(q, fixture(3, EventType::HypeTrain, "channel", HypeTrainStage::Begin));
		enqueue(q, fixture(4, EventType::HypeTrain));
		enqueue(q, fixture(5, EventType::HypeTrain, "other"));
		require(q.size() == 5, "coalescing crossed boundary/channel");
		const auto stages = q.takeBatch();
		for (int i = 0; i < 5; ++i) require(stages[i].event->data().id == "event-" + std::to_string(i+1), "Hype boundary reordered");
		q.stop(); q.start();
		for (int i=1;i<=2;++i) {
			EventEngine engine;engine.start();auto draft=makeSimulation(EventType::HypeTrain,i,std::chrono::system_clock::now());
			std::get<HypeTrainEvent>(draft.payload).trainId="train-"+std::to_string(i);
			enqueue(q,engine.process(std::move(draft)).event);
		}
		require(q.size()==2,"Progress from separate trains was merged");
		for (auto type : {EventType::Follow, EventType::Sub, EventType::Cheer, EventType::Gift, EventType::Raid, EventType::Chat}) {
			q.stop(); q.start();
			enqueue(q, fixture(1, type)); enqueue(q, fixture(2, type));
			require(q.size() == 2, "individual event lost to coalescing");
		}
		ExecutionQueue small({2, 2, 1, std::chrono::seconds(10)}); small.start();
		const auto token = small.enqueue(fixture(1), clockStart).jobId;
		enqueue(small, fixture(2));
		require(small.enqueue(fixture(3), clockStart).status == EnqueueStatus::Full && small.size() == 2, "capacity not enforced");
		require(small.takeBatch(0).empty() && small.size() == 2, "zero batch mutated queue");
		require(small.takeBatch(99).size() == 1 && !small.cancel(token), "batch cap or delivered cancellation");
		require(bool(small.enqueue(fixture(3), clockStart)), "full rejection incorrectly deduplicated retry");
		require(small.recentIdCount() == 2, "history cap");
		require(small.enqueue(fixture(2), clockStart + std::chrono::seconds(11)).status == EnqueueStatus::Duplicate, "pending duplicate after TTL");
		small.cancelAll();
		const auto cancelled = small.enqueue(fixture(4), clockStart + std::chrono::seconds(12));
		require(small.cancel(cancelled.jobId) && !small.cancel(cancelled.jobId), "cancel idempotency");
		require(small.enqueue(fixture(4), clockStart).status == EnqueueStatus::Duplicate, "cancel/backward clock loses dedup");
		small.stop(); small.stop(); small.start();
		const auto fresh = small.enqueue(fixture(4), clockStart);
		require(bool(fresh) && fresh.jobId != cancelled.jobId && !small.cancel(cancelled.jobId), "stale cancellation after restart");
		std::weak_ptr<const Event> weak;
		{ auto item = fixture(88); weak = item; small.cancelAll(); enqueue(small, item); }
		require(!weak.expired(), "queue did not retain event");
		small.stop(); require(weak.expired() && small.size() == 0 && small.recentIdCount() == 0, "stop retains work");
		require(small.takeBatch().empty(), "stopped batch not empty");
		// Same ID in different channels must not collide.
		q.stop(); q.start(); enqueue(q, fixture(1, EventType::Follow, "a")); enqueue(q, fixture(1, EventType::Follow, "b"));
		require(q.size() == 2, "dedup crosses channel");
		ExecutionQueue bounded({2, 2, 2, std::chrono::seconds(10)}); bounded.start();
		const auto mergedToken = bounded.enqueue(fixture(1, EventType::HypeTrain), clockStart).jobId;
		enqueue(bounded, fixture(2));
		require(bounded.enqueue(fixture(3, EventType::HypeTrain), clockStart).status == EnqueueStatus::Coalesced,
			"full queue should still coalesce pending progress");
		// The Follow ID is still pending even after enough merged IDs evict it from recent history.
		for (int i = 4; i < 100; ++i) enqueue(bounded, fixture(i, EventType::HypeTrain));
		require(bounded.size() == 2 && bounded.recentIdCount() == 2, "coalescing grows memory unbounded");
		require(bounded.enqueue(fixture(2), clockStart).status == EnqueueStatus::Duplicate, "evicted pending ID duplicated");
		require(bounded.cancel(mergedToken) && bounded.size() == 1, "coalesced token cannot cancel replacement");
		require(bounded.enqueue(fixture(99, EventType::HypeTrain), clockStart).status == EnqueueStatus::Duplicate,
			"cancelled replacement duplicate");
		// Completed history is bounded: after eviction, an old ID may be accepted again.
		bounded.stop(); bounded.start();
		for (int i = 1; i <= 3; ++i) { enqueue(bounded, fixture(i)); bounded.takeBatch(); }
		require(bool(bounded.enqueue(fixture(1), clockStart)), "history eviction not honored");
		bool badLimits = false;
		try { ExecutionQueue invalid({0, 2, 2, std::chrono::seconds(10)}); }
		catch (const std::invalid_argument &) { badLimits = true; }
		require(badLimits, "invalid capacity accepted");
		std::cout << "PASS: priorities, fairness, FIFO, bounded batches/history, dedup TTL, coalescing, cancellation, lifecycle\n";
		q.stop(); q.start(); enqueue(q, fixture(1)); enqueue(q, fixture(2, EventType::Raid));
		const auto prepared = q.prepareBatch();
		require(q.size() == 2 && prepared.items().size() == 2, "prepare removed pending work");
		require(q.prepareBatch().items()[0].jobId == prepared.items()[0].jobId, "prepare advances priority turns");
		require(q.commitBatch(prepared) && q.size() == 0 && !q.commitBatch(prepared), "batch replay applied twice");
		enqueue(q, fixture(3)); const auto stale = q.prepareBatch(); enqueue(q, fixture(4));
		require(!q.commitBatch(stale) && q.size() == 2, "stale transaction removed jobs");
		ExecutionQueue foreign; foreign.start(); enqueue(foreign, fixture(9));
		require(!foreign.commitBatch(q.prepareBatch()), "foreign transaction accepted");
		const auto cancelledBatch = q.prepareBatch(); q.cancelAll();
		require(!q.commitBatch(cancelledBatch), "cancel did not invalidate transaction");
		return 0;
	} catch (const std::exception &error) { std::cerr << error.what() << '\n'; return 1; }
}
