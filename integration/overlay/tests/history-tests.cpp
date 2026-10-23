// SPDX-License-Identifier: GPL-2.0-or-later
#include "core/plugin-core.hpp"
#include "adapters/json-daily-store.hpp"
#include "sources/event-list-model.hpp"
#include "sources/alert-model.hpp"
#include "sources/supporter-frame.hpp"
#include "events/simulation.hpp"
#include <QGuiApplication>
#include <QTemporaryDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <iostream>
#include <stdexcept>
using namespace oto;
namespace {
void require(bool ok,const char *why){if(!ok)throw std::runtime_error(why);}
struct Config:IConfigStore{ConfigLoadResult load()override{return {ConfigStatus::Loaded,{false}};}bool save(Settings)override{return true;}};
struct Log:ILogger{void write(LogLevel,std::string_view)const noexcept override{}};
std::unique_ptr<PluginCore> open(const QString &path){auto core=std::make_unique<PluginCore>(std::make_unique<Config>(),std::make_unique<Log>(),std::make_unique<JsonDailyStore>(path));core->start();return core;}
QByteArray read(const QString &path){QFile file(path);require(file.open(QIODevice::ReadOnly),"Read history fixture");return file.readAll();}
void write(const QString &path,const QByteArray &bytes){QFile file(path);require(file.open(QIODevice::WriteOnly),"Write history fixture");require(file.write(bytes)==bytes.size(),"Write complete fixture");}
}
int main(int argc,char **argv){QGuiApplication app(argc,argv);try{
	QTemporaryDir dir;require(dir.isValid(),"Temporary directory");const auto path=dir.filePath("history.json");
	const state::ChannelKey channel{events::EventOrigin::Twitch,"123"};const auto now=std::chrono::system_clock::now();
	auto core=open(path);int sequence=100;events::EventDraft last;
	for(auto type:{events::EventType::Follow,events::EventType::Sub,events::EventType::Cheer,events::EventType::Gift,events::EventType::Raid}){
		auto draft=events::makeSimulation(type,++sequence,now-std::chrono::seconds(sequence));draft.origin=channel.origin;draft.channelId=channel.channelId;
		if(type==events::EventType::Gift)std::get<events::GiftEvent>(draft.payload).gifter.reset();
		if(type==events::EventType::Cheer)std::get<events::CheerEvent>(draft.payload).user.reset();
		last=draft;require(bool(core->processEvent(draft)),"Submit activity");
	}
	core->processPendingEvents();const auto totals=state::dailyBook(core->sharedSnapshot());const auto baseline=read(path);
	const auto preview=core->createSourcePreview(false,true);core->releaseSourcePreview(preview);require(read(path)==baseline,"TEST persisted");core->stop();
	core=open(path);const auto restored=core->sharedSnapshot()->channels.at(channel);
	require(core->selectedChannel()==channel && restored->activity.size()==5 && state::dailyBook(core->sharedSnapshot())==totals,"Real history or startup channel not restored");
	require(restored->sessionBits==0 && restored->sessionSubs==0 && restored->activityDelivery==0 && core->pendingEventCount()==0,"Restoration counted or replayed history");
	require(core->processEvent(last).queue.status==execution::EnqueueStatus::Duplicate,"Restored EventSub duplicate admitted");
	sources::ListSettings settings;sources::ListLabels labels{"Anonymous","%1 Bits","%1 Subs","%1 viewers","Sub x1","%1 months"};
	visual::VisualEngine visual;const auto clock=visual::MotionClock::now();
	auto view=sources::eventListView(core->sharedSnapshot(),settings,labels,channel);require(view.items.size()==5 && view.items[2].name=="Anonymous" && view.items[3].name=="Anonymous","Anonymous history lost");
	auto frame=sources::prepareEventListFrame(visual,settings,labels,view,{},clock);for(const auto &row:frame->rows)require(row.entered==visual::MotionClock::time_point{},"Startup replayed entry animation");
	sources::AlertCursor cursor;cursor.synchronize(core->sharedSnapshot(),channel,true);require(!cursor.next(core->sharedSnapshot(),{true,true,true,true,true}),"Restored history fired alerts");
	auto history=events::makeSimulation(events::EventType::Follow,++sequence,now);history.origin=channel.origin;history.channelId=channel.channelId;
	core->restoreChannelHistory(channel,{history});const auto imported=core->sharedSnapshot();
	require(state::dailyBook(imported)==totals && imported->channels.at(channel)->activity.size()==6 && !cursor.next(imported,{true,true,true,true,true}),"Backfill changed totals or fired alerts");
	view=sources::eventListView(imported,settings,labels,channel);frame=sources::prepareEventListFrame(visual,settings,labels,view,frame,clock);for(const auto &row:frame->rows)require(row.entered==visual::MotionClock::time_point{} && row.moved==visual::MotionClock::time_point{},"Backfill animated old list");
	const auto saved=read(path);core->restoreChannelHistory(channel,{history});require(core->sharedSnapshot()==imported && read(path)==saved,"Repeated backfill changed history");
	auto liveDuplicate=history;liveDuplicate.id="live-follow-id";require(core->processEvent(liveDuplicate).queue.status==execution::EnqueueStatus::Duplicate,"Backfill/live overlap duplicated follower");
	for(int n=0;n<80;++n){auto draft=events::makeSimulation(events::EventType::Follow,++sequence,now+std::chrono::seconds(n+1));draft.origin=channel.origin;draft.channelId=channel.channelId;require(bool(core->processEvent(draft)),"Bounded activity submission");core->processPendingEvents();}
	const auto full=core->sharedSnapshot();require(full->channels.at(channel)->activity.size()==64,"Live history unbounded");
	auto late=history;late.id="late-follow";late.occurredAt=now-std::chrono::hours(48);require(bool(core->processEvent(late)),"Late activity rejected");core->processPendingEvents();
	require(core->sharedSnapshot()->channels.at(channel)->activity.back().event->data().id=="late-follow","Late activity lost");
	core->stop();core=open(path);require(core->sharedSnapshot()->channels.at(channel)->activity.size()==64 && core->sharedSnapshot()->channels.at(channel)->activity.back().event->data().id=="late-follow","Repeated Follow/late activity was not persisted");
	const auto before=core->sharedSnapshot();write(path,"external edit");bool failed=false;try{core->restoreChannelHistory(channel,{history});}catch(...){failed=true;}
	require(failed && core->sharedSnapshot()==before && read(path)=="external edit","Failed historical save partially published or overwrote file");core->stop();
	// Schema 2 can recover its actual four last events without guessing timestamps.
	auto old=QJsonDocument::fromJson(baseline).object();old.insert("schemaVersion",2);auto rows=old.value("channels").toArray();auto row=rows[0].toObject();row.remove("activity");rows[0]=row;old.insert("channels",rows);const auto legacy=dir.filePath("legacy.json");write(legacy,QJsonDocument(old).toJson());
	core=open(legacy);require(core->sharedSnapshot()->channels.at(channel)->activity.size()==4 && state::dailyBook(core->sharedSnapshot())==totals,"Schema 2 recovery lost real last fields/counters");core->stop();
	// Both sources recover actual records when old last-field slots are empty.
	auto missing=QJsonDocument::fromJson(baseline).object();auto missingRows=missing.value("channels").toArray();auto missingRow=missingRows[0].toObject();missingRow.insert("latest",QJsonArray{QJsonValue(QJsonValue::Null),QJsonValue(QJsonValue::Null),QJsonValue(QJsonValue::Null),QJsonValue(QJsonValue::Null)});missingRows[0]=missingRow;missing.insert("channels",missingRows);
	const auto repairPath=dir.filePath("missing-last-fields.json");write(repairPath,QJsonDocument(missing).toJson());core=open(repairPath);
	sources::BarLabels barLabels{{"Follow","Sub","Cheer","Gift","Bits","Subs"},"Empty","Anonymous"};
	const auto repaired=sources::supporterValues(core->sharedSnapshot(),state::localDay(now),barLabels,{},channel);
	require(repaired[0]=="DemoViewer" && repaired[1]=="DemoViewer" && repaired[2].contains("Anonymous") && repaired[3].contains("Anonymous"),"Supporter last fields not repaired from real shared history");
	sources::BarSettings barSettings;barSettings.motion=visual::Motion::StyleChange;
	const auto bar=sources::prepareSupporterFrame(visual,barSettings,barLabels,repaired,{},clock,{},{},sources::supporterContext(core->sharedSnapshot(),{},channel));
	for(const auto &layer:bar->layers)require(layer.started==visual::MotionClock::time_point{} && !layer.previousValue,"Recovered supporter history animated on load");
	require(state::dailyBook(core->sharedSnapshot())==totals,"Supporter repair recalculated daily totals");core->stop();
	// Malformed/future/over-capacity histories stay untouched and cannot be saved.
	for(int scenario=0;scenario<4;++scenario){auto bad=QJsonDocument::fromJson(baseline).object();auto channels=bad.value("channels").toArray();auto badRow=channels[0].toObject();auto entries=badRow.value("activity").toArray();
		if(scenario==0)entries.append(entries[0]);else if(scenario==1){auto entry=entries[0].toObject();entry.insert("type",5);entries[0]=entry;}else if(scenario==2)while(entries.size()<65)entries.append(entries[0]);else bad.insert("schemaVersion",4);
		badRow.insert("activity",entries);channels[0]=badRow;bad.insert("channels",channels);const auto bytes=QJsonDocument(bad).toJson();const auto damaged=dir.filePath(QString::number(scenario)+".json");write(damaged,bytes);
		JsonDailyStore store(damaged);bool rejected=false;try{store.load();}catch(...){rejected=true;}require(rejected && !store.save(totals) && read(damaged)==bytes,"Invalid history not protected");
	}
	settings.types.fill(false);view=sources::eventListView(before,settings,labels,channel);frame=sources::prepareEventListFrame(visual,settings,labels,view,{},clock);
	require(frame->rows.empty() && frame->emptyState && frame->emptyState->image.pixelColor(300,32).alpha()>0,"Empty source remains invisible");const auto renders=visual.stats().renders;
	require(sources::prepareEventListFrame(visual,settings,labels,view,frame,clock+std::chrono::hours(1))==frame && visual.stats().renders==renders,"Empty state renders continuously");
	std::cout<<"PASS: real history restart, quiet follows import, no totals/alert replay, 64-entry bound, repeated/late events, TEST isolation, persisted dedup, v2 migration, atomic failure, damaged-file protection, visible cached empty state\n";return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
