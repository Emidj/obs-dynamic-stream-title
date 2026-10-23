// SPDX-License-Identifier: GPL-2.0-or-later
#include "twitch/protocol.hpp"
#include "core/plugin-core.hpp"
#include "adapters/json-daily-store.hpp"
#include "sources/supporter-model.hpp"
#include <QCoreApplication>
#include <QDateTime>
#include <QTemporaryDir>
#include <iostream>
#include <stdexcept>
using namespace oto;
namespace {
void require(bool ok,const char *why){if(!ok)throw std::runtime_error(why);}
struct Config:IConfigStore{ConfigLoadResult load()override{return {ConfigStatus::Loaded,{false}};}bool save(Settings)override{return true;}};
struct Log:ILogger{void write(LogLevel,std::string_view)const noexcept override{}};
QJsonObject message(QString type,QString id,QJsonObject event) {
	event.insert("broadcaster_user_id","123");
 if(type.startsWith("channel.hype_train.")) {
  event.insert("id","train-fixture");event.insert("started_at","2026-10-04T08:00:00Z");event.insert("top_contributions",QJsonArray{});
  if(type.endsWith(".end"))event.insert("ended_at","2026-10-04T08:04:00Z");
  else {event.insert("expires_at","2026-10-04T08:05:00Z");event.insert("progress",event.value("total"));}
 }
	return {{"metadata",QJsonObject{{"message_type","notification"},{"message_id",id},{"message_timestamp",QDateTime::currentDateTimeUtc().toString("yyyy-MM-ddTHH:mm:ss.zzz")+"123456Z"}}},
		{"payload",QJsonObject{{"subscription",QJsonObject{{"type",type},{"version",type=="channel.follow" || type.startsWith("channel.hype_train.")?"2":"1"}}},{"event",event}}}};
}
}
int main(int argc,char **argv){QCoreApplication app(argc,argv);try{
	QTemporaryDir dir;require(dir.isValid(),"Temp directory");
	QJsonObject cheerChat{{"chatter_user_id","77"},{"chatter_user_name","Viewer"},{"message_id","chat-cheer-fixture"},{"message",QJsonObject{{"text","Cheer100"},{"fragments",QJsonArray{QJsonObject{{"type","cheermote"},{"text","Cheer100"},{"cheermote",QJsonObject{{"prefix","cheer"},{"bits",100},{"tier",100}}}}}}}}};
	const auto cheerDraft=twitch::notification(message("channel.chat.message","chat-cheer",cheerChat),"123");require(bool(cheerDraft),"Cheermote notification rejected");const auto &cheerFragment=std::get<events::ChatEvent>(cheerDraft->payload).fragments.at(0);require(cheerFragment.cheerPrefix=="cheer" && cheerFragment.cheerBits==100 && cheerFragment.cheerTier==100 && cheerFragment.emoteId.empty(),"Cheermote metadata lost");events::EventEngine cheerEngine;cheerEngine.start();require(bool(cheerEngine.process(*cheerDraft)),"Cheermote normalization failed");auto invalidCheer=*cheerDraft;std::get<events::ChatEvent>(invalidCheer.payload).fragments[0].cheerBits=0;require(!cheerEngine.process(invalidCheer),"Zero Bits cheermote accepted");cheerEngine.stop();
	PluginCore core(std::make_unique<Config>(),std::make_unique<Log>(),std::make_unique<JsonDailyStore>(dir.filePath("daily.json")));core.start();
	core.simulateEvent(events::EventType::Cheer);
	QJsonObject viewer{{"user_id","456"},{"user_name",QString::fromUtf8("Álex_東京")},{"tier","1000"}};
	auto submit=[&](QString type,QString id,QJsonObject e){auto d=twitch::notification(message(type,id,e),"123");require(bool(d),"Protocol rejected fixture");return core.processEvent(std::move(*d));};
	viewer.insert("is_gift",true);require(bool(submit("channel.subscribe","gift-recipient",viewer)),"Recipient rejected");
	viewer.insert("is_anonymous",false);viewer.insert("total",5);require(bool(submit("channel.subscription.gift","gift-batch",viewer)),"Gift batch rejected");
	require(submit("channel.subscription.gift","gift-batch",viewer).queue.status==execution::EnqueueStatus::Duplicate,"Duplicate counted");
	viewer.insert("is_gift",false);require(bool(submit("channel.subscribe","paid",viewer)),"Paid sub");
	viewer.insert("cumulative_months",14);require(bool(submit("channel.subscription.message","resub-chat",viewer)),"Resub message");
	viewer.insert("bits",100);require(bool(submit("channel.cheer","cheer",viewer)),"Cheer");
	require(!twitch::notification(message("channel.cheer","wrong",viewer),"999"),"Wrong broadcaster accepted");
	const auto updated=core.processPendingEventsFor(events::EventOrigin::Twitch,"123");
	require(updated.processed==5 && core.pendingEventCount()==1,"Live drain consumed simulation");
	const auto live=updated.snapshot->channels.at({events::EventOrigin::Twitch,"123"});
	require(live->daily.subs==6 && live->daily.bits==100,"Gift double counting or resub overcount");
	core.selectChannel({events::EventOrigin::Twitch,"123"});
	sources::BarLabels labels{{"Follow","Sub","Cheer","Gift","Bits","Subs"},"Empty","Anonymous"};
	const auto values=sources::supporterValues(updated.snapshot,state::localDay(std::chrono::system_clock::now()),labels,{},core.selectedChannel());
	require(values[4]=="100"&&values[5]=="6","Live source routing");
	JsonDailyStore reopen(dir.filePath("daily.json"));require(reopen.load().at({events::EventOrigin::Twitch,"123"}).subs==6,"Live persistence");
	viewer.insert("bits",0);auto zero=twitch::notification(message("channel.cheer","zero",viewer),"123");require(zero && !core.processEvent(*zero),"Engine validation bypassed");
	QJsonObject chat{{"chatter_user_id","456"},{"chatter_user_name","Viewer"},{"message",QJsonObject{{"text","hello"}}}};
	require(bool(submit("channel.chat.message","chat",chat)),"Chat mapping");
	QJsonObject hype{{"level",2},{"total",40}};require(bool(submit("channel.hype_train.end","end",hype)),"Hype end missing goal rejected");
	require(core.cancelPendingEventsFor(events::EventOrigin::Simulation,"sim-channel")==1 && core.pendingEventCount()==2,"Simulation cancellation touched live events");
	require(twitch::subscriptions("123","session").size()==13,"Subscription inventory");
	// Cover every subscribed notification, including the non-visible shared-state types.
	PluginCore complete(std::make_unique<Config>(),std::make_unique<Log>(),std::make_unique<JsonDailyStore>(dir.filePath("all-types.json")));complete.start();
	QJsonObject sub{{"user_id","456"},{"user_name","Viewer"},{"tier","1000"},{"is_gift",false}};
	QJsonObject gift{{"is_anonymous",true},{"total",2},{"tier","1000"}};
	QJsonObject cheer{{"is_anonymous",true},{"bits",7}};
	QJsonObject raid{{"to_broadcaster_user_id","123"},{"from_broadcaster_user_id","789"},{"from_broadcaster_user_name","Raider"},{"viewers",3}};
	QJsonObject progress{{"level",2},{"total",40},{"goal",100}};
	sub.insert("cumulative_months",14);
	const std::vector<std::pair<QString,QJsonObject>> fixtures{
		{"channel.follow",sub},{"channel.subscribe",sub},{"channel.subscription.gift",gift},{"channel.subscription.message",sub},
		{"channel.cheer",cheer},{"channel.raid",raid},{"channel.chat.message",chat},
		{"channel.hype_train.begin",progress},{"channel.hype_train.progress",progress},{"channel.hype_train.end",hype}};
	for(const auto &[type,event]:fixtures){const auto draft=twitch::notification(message(type,type,event),"123");require(bool(draft),"Subscribed notification mapping missing");
		require(bool(complete.processEvent(*draft)),"Subscribed notification rejected by the engine");require(complete.processEvent(*draft).queue.status==execution::EnqueueStatus::Duplicate,"Subscribed notification duplicate accepted");}
	const auto all=complete.processPendingEventsFor(events::EventOrigin::Twitch,"123");require(all.processed==10,"Subscribed notifications not consumed exactly once");
	const auto channel=all.snapshot->channels.at({events::EventOrigin::Twitch,"123"});require(channel->daily.bits==7 && channel->daily.subs==3,"Anonymous gift/cheer or resub counts incorrect");
	complete.selectChannel({events::EventOrigin::Twitch,"123"});const auto displayed=sources::supporterValues(all.snapshot,state::localDay(std::chrono::system_clock::now()),labels,{},complete.selectedChannel());
	require(displayed[0]=="Viewer" && displayed[2].contains("Anonymous") && displayed[3].contains("Anonymous") && displayed[4]=="7" && displayed[5]=="3","Subscribed notifications not reflected in the bar");
	require(!twitch::notification(message("channel.hype_train.unknown","unknown-stage",progress),"123"),"Unknown Hype Train stage accepted as End");
	auto wrongVersion=message("channel.follow","bad-version",sub);auto payload=wrongVersion.value("payload").toObject();auto subscription=payload.value("subscription").toObject();subscription.insert("version","1");payload.insert("subscription",subscription);wrongVersion.insert("payload",payload);
	require(!twitch::notification(wrongVersion,"123"),"Wrong notification version accepted");
	complete.stop();PluginCore restored(std::make_unique<Config>(),std::make_unique<Log>(),std::make_unique<JsonDailyStore>(dir.filePath("all-types.json")));restored.start();restored.selectChannel({events::EventOrigin::Twitch,"123"});
	const auto restoredValues=sources::supporterValues(restored.sharedSnapshot(),state::localDay(std::chrono::system_clock::now()),labels,{},restored.selectedChannel());require(restoredValues==displayed,"Latest users or non-zero totals lost after core restart");require(restored.sharedSnapshot()->channels.at({events::EventOrigin::Twitch,"123"})->sessionSubs==0 && restored.pendingEventCount()==0,"Restored activity replayed or added session totals");restored.stop();
	core.stop();std::cout<<"PASS: Twitch origin, scope isolation, nanosecond timestamps, gift dedup, daily restart, chat and hype end\n";return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
