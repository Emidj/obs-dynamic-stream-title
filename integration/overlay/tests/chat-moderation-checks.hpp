// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include "sources/chat-model.hpp"
#include "core/plugin-core.hpp"
#include "twitch/protocol.hpp"
#include <QJsonObject>
#include <stdexcept>
inline void checkChatModeration(const std::shared_ptr<oto::PluginCore> &core,oto::visual::VisualEngine &visual){
	using namespace oto;const auto check=[](bool ok,const char *why){if(!ok)throw std::runtime_error(why);};core->start();
	const state::ChannelKey key{events::EventOrigin::Twitch,"123"};const auto time=std::chrono::system_clock::now();int sequence=0;
	const auto put=[&](events::EventPayload payload,int seconds=0,state::ChannelKey destination=state::ChannelKey{events::EventOrigin::Twitch,"123"}){const auto result=core->processEvent({"moderation-notification-"+std::to_string(++sequence),destination.channelId,time+std::chrono::seconds(seconds),destination.origin,std::move(payload)});check(bool(result),"Moderation fixture admission");return result;};
	const auto message=[](std::string id,std::string user){events::ChatEvent result{{user,user},"Visible message with emotes"};result.messageId=std::move(id);return result;};
	const auto flush=[&]{core->processPendingEvents();};const auto channelState=[&]{return core->sharedSnapshot()->channels.at(key);};
	put(message("chat-id-A","alice"));put(message("chat-id-B","bob"));flush();const auto original=core->sharedSnapshot();
	put(events::ChatModerationEvent{events::ChatModerationAction::DeleteMessage,"chat-id-A",{}},1);flush();check(channelState()->chat.size()==1 && std::get<events::ChatEvent>(channelState()->chat[0].event->data().payload).messageId=="chat-id-B","Delete confused notification ID and message ID");check(original->channels.at(key)->chat.size()==2,"Published snapshot mutated");
	put(events::ChatModerationEvent{events::ChatModerationAction::ClearUser,{},"bob"},2);put(message("late-bob","bob"),1);put(message("new-bob","bob"),3);flush();check(channelState()->chat.size()==1 && std::get<events::ChatEvent>(channelState()->chat[0].event->data().payload).messageId=="new-bob","Timeout resurrects late message or blocks future message");
	put(events::ChatModerationEvent{events::ChatModerationAction::ClearUser,{},"bob"},1);flush();check(channelState()->chat.size()==1,"Older timeout regressed watermark");
	put(events::ChatModerationEvent{events::ChatModerationAction::DeleteMessage,"not-yet-delivered",{}},4);put(message("not-yet-delivered","alice"),3);flush();check(channelState()->chat.size()==1,"Early delete did not protect queued message");
	put(events::ChatModerationEvent{},5);put(message("old-after-clear","alice"),4);put(message("new-after-clear","alice"),6);flush();check(channelState()->chat.size()==1,"Clear failed for late messages or rejected new chat");
	put(events::ChatModerationEvent{},3);flush();check(channelState()->chat.size()==1,"Old clear removed future messages");
	put(message("new-after-clear","other-user"),6,{events::EventOrigin::Twitch,"456"});put(message("new-after-clear","other-user"),6,{events::EventOrigin::Preview,"preview-isolated"});put(events::ChatModerationEvent{},7);flush();check(channelState()->chat.empty() && !channelState()->latest[state::slot(events::EventType::Chat)].event,"Clear retained latest removed content");check(core->sharedSnapshot()->channels.at({events::EventOrigin::Twitch,"456"})->chat.size()==1 && core->sharedSnapshot()->channels.at({events::EventOrigin::Preview,"preview-isolated"})->chat.size()==1,"Moderation crossed channel/origin");
	check(channelState()->activity.empty() && channelState()->daily.bits==0 && channelState()->daily.subs==0 && core->presentations().pending()==0,"Moderation affected activity/totals/presentation queue");
	// Delete a card already retiring, then refresh its media. It must never
	// reappear during a normal departure, even outside the bounded view.
	put(message("retiring","alice"),8);flush();sources::ChatSettings settings;settings.layout=sources::ChatLayout::Horizontal;settings.width=1920;settings.height=100;settings.messages=1;const auto mono=visual::MotionClock::now();
	auto frame=sources::prepareChatFrame(visual,settings,sources::chatView(core->sharedSnapshot(),key,settings),{},mono);
	put(message("remaining","bob"),9);flush();frame=sources::prepareChatFrame(visual,settings,sources::chatView(core->sharedSnapshot(),key,settings),frame,mono+std::chrono::milliseconds(100));check(frame->rows.size()==2 && frame->rows.front().retiring,"Normal horizontal departure changed");
	const auto survivor=frame->rows.back();put(events::ChatModerationEvent{events::ChatModerationAction::DeleteMessage,"retiring",{}},10);flush();auto next=sources::prepareChatFrame(visual,settings,sources::chatView(core->sharedSnapshot(),key,settings),frame,mono+std::chrono::milliseconds(120));check(next->rows.size()==1 && next->rows[0].event==survivor.event && next->rows[0].graphic==survivor.graphic && next->rows[0].entered==survivor.entered,"Deleted retiring content persists or survivor reraster/reentry");
	const auto renders=visual.stats().renders;check(sources::prepareChatFrame(visual,settings,next->view,next,mono+std::chrono::seconds(1))==next && visual.stats().renders==renders,"Moderation reraster on idle");
	put(events::ChatModerationEvent{},11);flush();for(const bool horizontal:{false,true}){settings.layout=horizontal?sources::ChatLayout::Horizontal:sources::ChatLayout::Vertical;check(sources::prepareChatFrame(visual,settings,sources::chatView(core->sharedSnapshot(),key,settings),next,mono)->rows.empty(),"Clear leaves visible content");}
	for(int i=0;i<300;++i){put(events::ChatModerationEvent{events::ChatModerationAction::DeleteMessage,"missing-"+std::to_string(i),{}},12+i);put(events::ChatModerationEvent{events::ChatModerationAction::ClearUser,{},"user-"+std::to_string(i)},12+i);flush();}
	check(channelState()->chatModeration->messages.size()==256 && channelState()->chatModeration->users.size()==256,"Moderation cache unbounded");
	for(const auto bad:{events::ChatModerationEvent{events::ChatModerationAction::DeleteMessage,{},{}},events::ChatModerationEvent{events::ChatModerationAction::ClearUser,{},{}},events::ChatModerationEvent{events::ChatModerationAction::ClearAll,"unexpected",{}},events::ChatModerationEvent{events::ChatModerationAction(99),{},{}},events::ChatModerationEvent{events::ChatModerationAction::DeleteMessage,std::string(129,'x'),{}}})check(!core->processEvent({"invalid-moderation","123",time,events::EventOrigin::Twitch,bad}),"Invalid moderation accepted");
	const auto subs=twitch::subscriptions("123","session");check(subs.size()==13,"Moderation subscription inventory");for(const auto &entry:subs){const auto sub=entry.toObject();if(sub["type"].toString().startsWith("channel.chat."))check(sub["version"]=="1" && sub["condition"].toObject()["user_id"]=="123","Wrong moderation subscription condition/version");}
	for(const auto &[type,event]:std::vector<std::pair<QString,QJsonObject>>{{"channel.chat.message_delete",{{"message_id","real-chat-id"}}},{"channel.chat.clear_user_messages",{{"target_user_id","alice"}}},{"channel.chat.clear",{}}}){
		auto body=event;body["broadcaster_user_id"]="123";QJsonObject envelope{{"metadata",QJsonObject{{"message_type","notification"},{"message_id","different-transport-id"},{"message_timestamp","2026-10-07T10:00:00Z"}}},{"payload",QJsonObject{{"subscription",QJsonObject{{"type",type},{"version","1"}}},{"event",body}}}};
		const auto parsed=twitch::notification(envelope,"123");check(parsed && std::holds_alternative<events::ChatModerationEvent>(parsed->payload) && !twitch::notification(envelope,"456"),"Moderation protocol/account mapping");
	}
	core->stop();core->start();check(core->sharedSnapshot()->channels.empty(),"Moderation/chat persisted across restart");core->stop();
}
