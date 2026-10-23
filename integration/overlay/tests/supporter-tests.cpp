// SPDX-License-Identifier: GPL-2.0-or-later
#include "sources/supporter-model.hpp"
#include "visual/visual-engine.hpp"
#include "adapters/json-daily-store.hpp"
#include "core/plugin-core.hpp"
#include "events/simulation.hpp"
#include <QGuiApplication>
#include <QDateTime>
#include <QTimeZone>
#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QTemporaryDir>
#include <iostream>
#include <limits>
#include <stdexcept>
using namespace oto;
namespace {
void require(bool ok, const char *why) { if (!ok) throw std::runtime_error(why); }
struct SettingsStore : IConfigStore {
	ConfigLoadResult load() override { return {ConfigStatus::Loaded,{false}}; }
	bool save(Settings) override { return true; }
};
struct Sink : ILogger { void write(LogLevel,std::string_view) const noexcept override {} };
struct DailyMemory : IDailyStore {
	state::DailyBook book; bool fail = false; int saves = 0;
	state::DailyBook load() override { return book; }
	bool save(const state::DailyBook &next) override { ++saves; if (fail) return false; book = next; return true; }
};
events::EventPtr event(events::EventType type, int sequence, std::chrono::system_clock::time_point when)
{
	events::EventEngine engine; engine.start(); return engine.process(events::makeSimulation(type,sequence,when)).event;
}
}
int main(int argc, char **argv)
{
	QGuiApplication app(argc,argv);
	try {
		const auto now = std::chrono::system_clock::now();
		const int today = state::localDay(now);
		const auto tomorrowQt = QDateTime(QDate::currentDate().addDays(1),QTime(12,0),QTimeZone::systemTimeZone());
		const auto tomorrow = std::chrono::system_clock::time_point(std::chrono::milliseconds(tomorrowQt.toMSecsSinceEpoch()));
		const int tomorrowDay = state::localDay(tomorrow);
		state::SharedState shared; state::TextCache cache; shared.start();
		const auto initial = shared.prepare({event(events::EventType::Follow,1,now),event(events::EventType::Sub,2,now),
			event(events::EventType::Cheer,3,now),event(events::EventType::Gift,4,now)},cache,now);
		shared.publish(initial);
		sources::BarLabels labels{{"Follow","Sub","Cheer","Gift","Bits","Subs"},"Empty","Anonymous"};
		const auto values = sources::supporterValues(initial,today,labels);
		require(values[0] == "DemoViewer" && values[1] == "DemoViewer" && values[2] == QString::fromUtf8("DemoViewer · 100") &&
			values[3] == QString::fromUtf8("DemoViewer ×5") && values[4] == "100" && values[5] == "6", "Six fields or daily aggregation failed");
		const auto nextDayValues = sources::supporterValues(initial,tomorrowDay,labels);
		require(nextDayValues[4] == "0" && nextDayValues[5] == "0" && nextDayValues[0] == "DemoViewer", "Midnight view rollover failed");
		const auto next = shared.prepare({event(events::EventType::Cheer,5,tomorrow),event(events::EventType::Sub,6,now)},cache,tomorrow);
		require(next->channels.begin()->second->daily.bits == 100 && next->channels.begin()->second->daily.subs == 0,
			"Late event contaminated today's counters");
		require(initial->channels.begin()->second->daily.bits == 100 && initial->channels.begin()->second->daily.subs == 6, "Old snapshot mutated");
		visual::VisualEngine visual; sources::BarSettings settings;
		const auto a = visual.prepare(sources::supporterScene(settings,labels,values));
		const auto b = visual.prepare(sources::supporterScene(settings,labels,values));
		require(a == b && visual.stats().renders == 1, "Identical sources did not share renderer results");
		settings.fields[0] = false; settings.width = 800;
		const auto independent = visual.prepare(sources::supporterScene(settings,labels,values));
		require(independent != a && a->image.width() == 1080 && independent->textMetrics.size() == 10, "Independent source options failed");
		settings.fields.fill(false);
		require(sources::supporterScene(settings,labels,values).nodes.empty(), "All fields hidden should be transparent");
		QTemporaryDir directory; require(directory.isValid(),"Temporary directory failed");
		const auto path = directory.filePath("daily.json");
		JsonDailyStore disk(path); require(disk.load().empty(),"Missing daily file");
		require(disk.save(state::dailyBook(initial)),"Daily save failed");
		JsonDailyStore reopen(path); const auto restored = reopen.load();
		require(restored == state::dailyBook(initial),"Daily restart round trip");
		state::SharedState resumed; resumed.start(restored);
		const auto restoredValues = sources::supporterValues(resumed.snapshot(),today,labels);
		require(restoredValues[0] == "Empty" && restoredValues[4] == "100" && restoredValues[5] == "6", "Restore mixed daily and session data");
		auto huge = restored; huge.begin()->second.bits = std::numeric_limits<uint64_t>::max(); require(reopen.save(huge),"Large counter save");
		JsonDailyStore precise(path); require(precise.load() == huge,"64-bit integer precision lost");
		QFile changed(path); require(changed.open(QIODevice::WriteOnly),"External edit failed"); changed.write("{broken"); changed.close();
		require(!precise.save(restored),"External edit overwritten");
		JsonDailyStore invalid(path); bool rejected = false; try { invalid.load(); } catch (...) { rejected = true; }
		require(rejected && !invalid.save(restored),"Malformed file not protected");
		const auto activityPath=directory.filePath("activity.json");
		PluginCore original(std::make_unique<SettingsStore>(),std::make_unique<Sink>(),std::make_unique<JsonDailyStore>(activityPath));original.start();
		for(const auto type:{events::EventType::Follow,events::EventType::Sub,events::EventType::Cheer,events::EventType::Gift})original.simulateEvent(type);
		original.processPendingEvents();const auto savedValues=sources::supporterValues(original.sharedSnapshot(),today,labels);original.stop();
		PluginCore recovered(std::make_unique<SettingsStore>(),std::make_unique<Sink>(),std::make_unique<JsonDailyStore>(activityPath));recovered.start();
		require(sources::supporterValues(recovered.sharedSnapshot(),today,labels)==savedValues,"Latest names/amounts did not survive restart");
		require(recovered.sharedSnapshot()->channels.begin()->second->sessionSubs==0 && recovered.pendingEventCount()==0,"Restoration replayed events");
		const auto midnightValues=sources::supporterValues(recovered.sharedSnapshot(),tomorrowDay,labels);require(midnightValues[0]==savedValues[0] && midnightValues[1]==savedValues[1] && midnightValues[4]=="0" && midnightValues[5]=="0","Midnight erased last users or retained daily totals");
		QFile activityFile(activityPath);require(activityFile.open(QIODevice::ReadOnly),"Activity read");const auto activityBytes=activityFile.readAll();activityFile.close();
		const auto preview=recovered.createSourcePreview(true);recovered.releaseSourcePreview(preview);require(activityFile.open(QIODevice::ReadOnly),"Preview persistence read");require(activityFile.readAll()==activityBytes,"Preview overwrote last users");activityFile.close();recovered.stop();
		auto corrupt=QJsonDocument::fromJson(activityBytes).object();auto channels=corrupt.value("channels").toArray();auto row=channels[0].toObject();auto latest=row.value("latest").toArray();auto follow=latest[0].toObject();auto actor=follow.value("user").toObject();actor.insert("id","");follow.insert("user",actor);latest[0]=follow;row.insert("latest",latest);channels[0]=row;corrupt.insert("channels",channels);
		require(activityFile.open(QIODevice::WriteOnly),"Corruption fixture write");const auto corruptBytes=QJsonDocument(corrupt).toJson();activityFile.write(corruptBytes);activityFile.close();
		{PluginCore damaged(std::make_unique<SettingsStore>(),std::make_unique<Sink>(),std::make_unique<JsonDailyStore>(activityPath));bool refused=false;try{damaged.start();}catch(...){refused=true;}require(refused,"Corrupt last user accepted");}
		require(activityFile.open(QIODevice::ReadOnly),"Corruption preservation read");require(activityFile.readAll()==corruptBytes,"Corrupt activity file overwritten");activityFile.close();
		const auto legacyPath=directory.filePath("legacy.json");auto legacy=QJsonDocument::fromJson(activityBytes).object();legacy.insert("schemaVersion",1);auto oldRows=legacy.value("channels").toArray();for(int i=0;i<oldRows.size();++i){auto old=oldRows[i].toObject();old.remove("latest");oldRows[i]=old;}legacy.insert("channels",oldRows);
		QFile legacyFile(legacyPath);require(legacyFile.open(QIODevice::WriteOnly),"Legacy write");legacyFile.write(QJsonDocument(legacy).toJson());legacyFile.close();
		PluginCore migrated(std::make_unique<SettingsStore>(),std::make_unique<Sink>(),std::make_unique<JsonDailyStore>(legacyPath));migrated.start();require(sources::supporterValues(migrated.sharedSnapshot(),today,labels)[5]=="6","Legacy counts not loaded");migrated.simulateEvent(events::EventType::Follow);migrated.processPendingEvents();migrated.stop();
		require(legacyFile.open(QIODevice::ReadOnly),"Migration read");const auto migratedJson=QJsonDocument::fromJson(legacyFile.readAll()).object();require(migratedJson.value("schemaVersion").toInt()==3 && migratedJson.value("channels").toArray()[0].toObject().value("subs").toString()=="6","Migration lost daily counts");legacyFile.close();
		auto storage = std::make_unique<DailyMemory>(); auto *store = storage.get();
		PluginCore core(std::make_unique<SettingsStore>(),std::make_unique<Sink>(),std::move(storage)); core.start();
		int notifications = 0; core.setStateChanged([&] { ++notifications; });
		core.simulateEvent(events::EventType::Cheer); const auto before = core.sharedSnapshot(); store->fail = true;
		bool failed = false; try { core.processPendingEvents(); } catch (...) { failed = true; }
		require(failed && core.pendingEventCount() == 1 && core.sharedSnapshot() == before && notifications == 0,"Failed persistence consumed batch");
		store->fail = false; core.processPendingEvents();
		require(core.pendingEventCount() == 0 && notifications == 1 && store->book.begin()->second.bits == 100,"Daily transaction retry failed");
		core.simulateEvent(events::EventType::Follow); const auto saves = store->saves; core.processPendingEvents();
		require(store->saves == saves+1 && notifications == 2,"Latest Follow was not saved with unchanged totals");
		core.stop(); core.start();
		require(core.sharedSnapshot()->channels.begin()->second->daily.bits == 100,"Core restart did not restore totals");
		std::cout << "PASS: six fields, shared frames, independent settings, local-day rollover, late events, daily persistence and failure rollback\n";
		return 0;
	} catch (const std::exception &error) { std::cerr << error.what() << '\n'; return 1; }
}
