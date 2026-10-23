// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include "sources/chat-model.hpp"
#include "twitch/protocol.hpp"
#include "events/event-engine.hpp"
#include <QBuffer>
#include <QJsonArray>
#include <QJsonObject>
#include <stdexcept>
inline QByteArray badgeFixtureCatalog(QString url="https://static-cdn.jtvnw.net/badges/v1/fixture/3",QString set="subscriber",QString version="0"){
	return QJsonDocument(QJsonObject{{"data",QJsonArray{QJsonObject{{"set_id",set},{"versions",QJsonArray{QJsonObject{{"id",version},{"image_url_4x",url}}}}}}}}).toJson();
}
inline QByteArray badgeFixturePng(QColor color){QImage image(72,72,QImage::Format_RGBA8888_Premultiplied);image.fill(color);QByteArray bytes;QBuffer buffer(&bytes);buffer.open(QIODevice::WriteOnly);image.save(&buffer,"PNG");return bytes;}
inline void checkChatBadges(std::shared_ptr<oto::visual::VisualEngine> visual){
	using namespace oto;
	const auto check=[](bool ok,const char *why){if(!ok)throw std::runtime_error(why);};
	const QString global="https://static-cdn.jtvnw.net/badges/v1/global/3",custom="https://static-cdn.jtvnw.net/badges/v1/custom/3";
	check(chat::publicBadgeUrl(custom) && !chat::publicEmoteUrl(custom),"Badge and emote URL validation conflated");
	for(const auto bad:{"https://static-cdn.jtvnw.net.evil/badges/v1/a/3","file:///badges/v1/a/3","https://static-cdn.jtvnw.net/badges/v1/a/3?token=x","https://user@static-cdn.jtvnw.net/badges/v1/a/3","https://static-cdn.jtvnw.net/badges/v1/a/3#x"})check(!chat::publicBadgeUrl(bad),"Unsafe badge URL accepted");
	check(!chat::parseBadgeCatalog("broken") && !chat::parseBadgeCatalog("{}") && chat::parseBadgeCatalog("{\"data\":[]}")->empty(),"Malformed/empty badge catalog confused");
	check(chat::parseBadgeCatalog(badgeFixtureCatalog("https://evil.example/a"))->empty(),"Untrusted badge image accepted");
	struct Pending{QString url;std::function<void(twitch::HttpReply)> done;};std::vector<Pending> requests;
	chat::EmoteCache cache(visual,[&](QString url,std::function<void(twitch::HttpReply)> done){requests.push_back({url,std::move(done)});});
	cache.ensureBadges("123");cache.ensureBadges("123");check(requests.size()==2,"Clones duplicate badge catalogs");
	requests[0].done({200,badgeFixtureCatalog(global),false});requests[1].done({200,badgeFixtureCatalog(custom),false});
	const std::vector<events::ChatBadge> ids{{"subscriber","0","12"},{"missing","1",{}}};auto urls=cache.badgeUrls(ids);check(urls.size()==2 && urls[0]==custom && urls[1].isEmpty(),"Channel badges do not override global or unknown badge invented");
	cache.getBadge(custom);cache.getBadge(custom);check(requests.size()==3,"Clones download same badge twice");requests[2].done({200,badgeFixturePng(QColor(0,255,0)),false});
	const auto image=cache.getBadge(custom);check(image && cache.getBadge(custom)==image && image->ends.size()==1,"Badge media not immutable/shared/static");
	cache.ensureBadges("456");check(cache.badgeUrls(ids)[0]==global && !cache.ingestBadges("123",badgeFixtureCatalog(custom)),"Old channel badge leaks after switch");
	requests[3].done({200,"{\"data\":[]}",false});cache.ensureBadges("456");check(requests.size()==4,"Empty channel catalog fetched again");
	cache.getBadge(global);check(requests.size()==5,"Global image not queued");requests[4].done({404,{},false});cache.getBadge(global);check(requests.size()==5,"Failed badge download has no backoff");
	check(cache.ingest(global,badgeFixturePng(QColor(0,255,0))),"Shared badge ingestion failed");
	events::EventEngine eventEngine;eventEngine.start();events::ChatEvent message{{"456","Long_Álex_東京_Name"},"Full message with emotes"};message.badges=ids;
	const auto now=std::chrono::system_clock::now();auto event=eventEngine.process({"badge-layout","456",now,events::EventOrigin::Twitch,message});check(bool(event),"Valid badge event rejected");
	for(const bool horizontal:{false,true}){sources::ChatSettings settings;settings.width=320;settings.height=100;settings.layout=horizontal?sources::ChatLayout::Horizontal:sources::ChatLayout::Vertical;settings.nameSize=128;settings.messageSize=128;
		sources::ChatView view{{events::EventOrigin::Twitch,"456"},1,{{event.event,{},1}}};const auto start=visual::MotionClock::now();
		const auto frame=sources::prepareChatFrame(*visual,settings,view,{},start,false,&cache);const auto &row=frame->rows.back();check(row.badges.size()==2 && row.badges[0].media && !row.badges[1].media,"Multiple badges lost");
		for(const auto &badge:row.badges){check(QRectF(QPointF{},row.graphic->image.size()).contains(badge.box),"Badge exceeds cell");check(badge.box.right()<row.bodyBox.left(),"Badge overlaps message body");}
		const auto count=visual->stats().renders;check(sources::prepareChatFrame(*visual,settings,view,frame,start+std::chrono::seconds(1),false,&cache)==frame && visual->stats().renders==count,"Idle badge reraster");
	}
	message.badges.assign(32,{"subscriber","0",{}});auto full=eventEngine.process({"badge-many","456",now,events::EventOrigin::Twitch,message});check(bool(full),"Bounded badge list rejected");
	sources::ChatSettings tiny;tiny.width=320;tiny.height=100;const auto all=sources::prepareChatFrame(*visual,tiny,{{events::EventOrigin::Twitch,"456"},1,{{full.event,{},1}}},{},visual::MotionClock::now(),false,&cache);check(all->rows.back().badges.size()==32,"Badges discarded to fit");for(const auto &badge:all->rows.back().badges)check(badge.box.right()<all->rows.back().bodyBox.left(),"Many badges invade body");
	message.badges.push_back({"subscriber","0",{}});check(!eventEngine.process({"too-many","456",now,events::EventOrigin::Twitch,message}),"Unbounded badge event accepted");message.badges={{"../subscriber","0",{}}};check(!eventEngine.process({"unsafe-id","456",now,events::EventOrigin::Twitch,message}),"Unsafe badge ID accepted");
	QJsonObject payload{{"broadcaster_user_id","123"},{"chatter_user_id","456"},{"chatter_user_name","Mod"},{"source_broadcaster_user_id","789"},{"badges",QJsonArray{QJsonObject{{"set_id","moderator"},{"id","1"},{"info",""}},QJsonObject{{"set_id","subscriber"},{"id","12"},{"info","36"}}}},{"source_badges",QJsonArray{QJsonObject{{"set_id","subscriber"},{"id","999"}}}},{"message",QJsonObject{{"text","Hello"}}}};
	QJsonObject envelope{{"metadata",QJsonObject{{"message_type","notification"},{"message_id","badge-protocol"},{"message_timestamp","2026-10-07T10:00:00Z"}}},{"payload",QJsonObject{{"subscription",QJsonObject{{"type","channel.chat.message"},{"version","1"}}},{"event",payload}}}};
	const auto parsed=twitch::notification(envelope,"123");check(bool(parsed),"Badge protocol rejected");const auto &chat=std::get<events::ChatEvent>(parsed->payload);check(chat.badges.size()==2 && chat.badges[1].version=="12" && chat.badges[1].info=="36" && !chat.canShoutout,"Badge version/order/info or Shared Chat permissions lost");
	cache.stop();cache.ensureBadges("123");check(!cache.getBadge(custom),"Stopped badge cache remains active");
}
