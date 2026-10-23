// SPDX-License-Identifier: GPL-2.0-or-later
#include "events/event-engine.hpp"
#include "events/simulation.hpp"
#include <iostream>
#include <stdexcept>
#include <type_traits>

using namespace oto::events;
namespace {
void require(bool condition, const char *message) { if (!condition) throw std::runtime_error(message); }
const auto now = std::chrono::system_clock::time_point(std::chrono::seconds(1000));
EventDraft fixture(EventType type = EventType::Follow) { return makeSimulation(type, 1, now); }
void rejected(EventEngine &engine, EventDraft draft, EventError error)
{
	const auto result = engine.process(std::move(draft));
	require(!result.event && result.error == error, "wrong rejection result");
}
}
int main()
{
	static_assert(std::is_const_v<EventPtr::element_type>);
	try {
		EventEngine engine;
		rejected(engine, fixture(), EventError::Stopped);
		engine.start(); engine.start();
		for (const auto type : {EventType::Follow, EventType::Sub, EventType::Cheer, EventType::Gift,
				       EventType::Raid, EventType::Chat, EventType::HypeTrain}) {
			const auto result = engine.process(fixture(type));
			require(result.event && result.error == EventError::None, "valid simulation rejected");
			require(typeOf(result.event->data().payload) == type, "payload type mismatch");
			require(result.event->data().origin == EventOrigin::Simulation, "simulation origin lost");
			EventPtr consumerA = result.event, consumerB = result.event;
			require(consumerA.get() == consumerB.get(), "payload copied per consumer");
		}
		auto draft = fixture();
		draft.id = "  event-1 ";
		std::get<FollowEvent>(draft.payload).follower = {" user-1 ", "  "};
		const auto normalized = engine.process(draft);
		require(normalized.event->data().id == "event-1", "id not normalized");
		require(std::get<FollowEvent>(normalized.event->data().payload).follower.displayName == "user-1", "name fallback failed");
		draft.id = "changed-by-producer";
		require(normalized.event->data().id == "event-1", "producer mutated accepted event");
		// Duplicates remain accepted here: deduplication belongs exclusively to Phase 4.
		require(bool(engine.process(fixture())) && bool(engine.process(fixture())), "premature deduplication");
		draft = fixture(); draft.id.clear(); rejected(engine, draft, EventError::InvalidId);
		draft = fixture(); draft.channelId = "bad channel"; rejected(engine, draft, EventError::InvalidId);
		draft = fixture(); draft.id.assign(129, 'a'); rejected(engine, draft, EventError::InvalidId);
		draft = fixture(EventType::Chat); draft.origin = EventOrigin::Twitch; draft.id = "opaque+message/id==";
		const auto opaque = engine.process(draft);
		require(opaque && opaque.event->data().id == draft.id, "Opaque Twitch ID rejected or changed");
		draft.origin = EventOrigin::Simulation; rejected(engine, draft, EventError::InvalidId);
		draft.origin = EventOrigin::Twitch; draft.channelId = "channel+invalid"; rejected(engine, draft, EventError::InvalidId);
		for (const auto &id : {std::string(), std::string(129, 'a'), std::string("bad\nID"), std::string("bad\0ID", 6)}) {
			draft = fixture(); draft.origin = EventOrigin::Twitch; draft.id = id; rejected(engine, draft, EventError::InvalidId);
		}
		draft = fixture(); draft.occurredAt = {}; rejected(engine, draft, EventError::InvalidTime);
		draft = fixture(); draft.origin = static_cast<EventOrigin>(99); rejected(engine, draft, EventError::InvalidOrigin);
		draft = fixture(EventType::Cheer); std::get<CheerEvent>(draft.payload).bits = 0; rejected(engine, draft, EventError::InvalidPayload);
		draft = fixture(EventType::Gift); std::get<GiftEvent>(draft.payload).tier = SubscriptionTier::Prime; rejected(engine, draft, EventError::InvalidPayload);
		draft = fixture(EventType::Raid); std::get<RaidEvent>(draft.payload).viewers = 0; rejected(engine, draft, EventError::InvalidPayload);
		draft = fixture(EventType::Sub); std::get<SubEvent>(draft.payload).totalMonths = 0; rejected(engine, draft, EventError::InvalidPayload);
		draft = fixture(EventType::HypeTrain); std::get<HypeTrainEvent>(draft.payload).goal = 0; rejected(engine, draft, EventError::InvalidPayload);
		for (const auto &text : {std::string("   "), std::string(4097, 'a'), std::string("\xc0\xaf"),
			std::string("\xed\xa0\x80"), std::string("\xf4\x90\x80\x80"), std::string("\xf0\x9f"), std::string("a\0b", 3)}) {
			draft = fixture(EventType::Chat); std::get<ChatEvent>(draft.payload).message = text;
			rejected(engine, draft, EventError::InvalidPayload);
		}
		draft = fixture(EventType::Cheer); std::get<CheerEvent>(draft.payload).user.reset();
		require(bool(engine.process(draft)), "anonymous cheer rejected");
		draft = fixture(EventType::Gift); std::get<GiftEvent>(draft.payload).gifter.reset();
		require(bool(engine.process(draft)), "anonymous gift rejected");
		draft = fixture(EventType::Chat); std::get<ChatEvent>(draft.payload).message = "  Español 日本語 🎵\n\t ";
		require(std::get<ChatEvent>(engine.process(draft).event->data().payload).message == std::get<ChatEvent>(draft.payload).message,
			"chat content changed");
		engine.stop(); engine.stop();
		rejected(engine, fixture(), EventError::Stopped);
		require(normalized.event->data().id == "event-1", "shared event invalid after engine stop");
		engine.start(); require(bool(engine.process(fixture())), "restart failed");
		require(makeSimulation(EventType::Follow, 1, now).id != makeSimulation(EventType::Follow, 2, now).id,
			"simulation ids collide within session");
		std::cout << "PASS: seven event types, normalization, validation, UTF-8, sharing, anonymous events, lifecycle\n";
		return 0;
	} catch (const std::exception &error) {
		std::cerr << error.what() << '\n'; return 1;
	}
}
