// SPDX-License-Identifier: GPL-2.0-or-later
#include "core/plugin-core.hpp"
#include "sources/supporter-model.hpp"
#include "visual/visual-engine.hpp"
#include <QGuiApplication>
#include <iostream>
#include <stdexcept>
using namespace oto;
namespace {
void require(bool value, const char *why) { if (!value) throw std::runtime_error(why); }
struct Config : IConfigStore {
	ConfigLoadResult load() override { return {ConfigStatus::Loaded,{false}}; }
	bool save(Settings) override { return true; }
};
struct Logger : ILogger { void write(LogLevel,std::string_view) const noexcept override {} };
struct Daily : IDailyStore {
	state::DailyBook book; int writes = 0;
	state::DailyBook load() override { return book; }
	bool save(const state::DailyBook &next) override { book=next; ++writes; return true; }
};
}
int main(int argc,char **argv)
{
	QGuiApplication app(argc,argv);
	try {
		auto daily=std::make_unique<Daily>(); auto *disk=daily.get();
		PluginCore core(std::make_unique<Config>(),std::make_unique<Logger>(),std::move(daily)); core.start();
		core.simulateEvent(events::EventType::Cheer); core.processPendingEvents();
		const auto original=core.sharedSnapshot(); const auto book=disk->book; const auto writes=disk->writes;
		core.simulateEvent(events::EventType::Sub); // TEST must not process this unrelated manual pending event.
		int notifications=0; core.setStateChanged([&] { ++notifications; });
		const auto normal=core.createSourcePreview(false);
		const auto longText=core.createSourcePreview(true);
		require(notifications==2 && core.pendingEventCount()==1,"Preview bypassed batch notification or drained unrelated queue");
		require(disk->book==book && disk->writes==writes,"Preview altered persisted daily totals");
		const auto current=core.sharedSnapshot();
		require(current->channels.at({events::EventOrigin::Simulation,"sim-channel"})==original->channels.begin()->second,"Preview mutated channel data");
		const auto today=state::localDay(std::chrono::system_clock::now());
		sources::BarLabels labels{{"Follow","Sub","Cheer","Gift","Bits","Subs"},"Empty","Anonymous"};
		const auto a=sources::supporterValues(current,today,labels,normal);
		const auto b=sources::supporterValues(current,today,labels,longText);
		require(a[0]=="TestViewer" && a[4]=="100" && a[5]=="6","Normal preview sample wrong");
		require(sources::supporterValues(current,0,labels,normal)[4]=="100","Preview fixture changed with calendar day");
		require(b[0].contains(QString::fromUtf8("東京")) && b[0].size()>80 && a[0]!=b[0],"Long Unicode sample or source isolation failed");
		visual::VisualEngine visual;
		const auto frame=visual.prepare(sources::supporterScene({},labels,b));
		bool elided=false; for (const auto &metric:frame->textMetrics) elided=elided || metric.elided;
		require(elided,"LONG TEXT did not exercise ellipsis");
		const auto again=visual.prepare(sources::supporterScene({},labels,b));
		require(again==frame,"Preview did not reuse shared visual cache");
		core.releaseSourcePreview(normal);
		require(core.sharedSnapshot()->channels.count({events::EventOrigin::Preview,normal})==0 &&
			core.sharedSnapshot()->channels.count({events::EventOrigin::Preview,longText})==1,"End test affected another preview");
		core.releaseSourcePreview(longText);
		core.processPendingEvents();
		require(core.sharedSnapshot()->channels.at({events::EventOrigin::Simulation,"sim-channel"})->daily.subs==1,"Normal pending event lost");
		const auto beforeFull=core.sharedSnapshot();
		for(int i=0;i<254;++i) require(bool(core.simulateEvent(events::EventType::Follow)),"Queue fill failed");
		bool rejected=false; try { core.createSourcePreview(false); } catch (...) { rejected=true; }
		require(rejected && core.pendingEventCount()==254 && core.sharedSnapshot()==beforeFull,"Partial preview admission failed to roll back");
		core.cancelPendingEvents();
		std::vector<std::string> channels;
		for(int i=0;i<14;++i) channels.push_back(core.createSourcePreview(i%2));
		rejected=false; try { core.createSourcePreview(false); } catch (...) { rejected=true; }
		require(rejected && core.pendingEventCount()==0,"Preview capacity is not bounded");
		for(const auto &channel:channels) core.releaseSourcePreview(channel);
		require(core.sharedSnapshot()->channels.size()==1,"Ended previews retained state channels");
		core.createSourcePreview(true); core.stop(); core.start();
		require(core.sharedSnapshot()->channels.size()==1 && state::dailyBook(core.sharedSnapshot())==disk->book,"Preview persisted across restart");
		const state::ChannelKey live{events::EventOrigin::Twitch,"123"};core.selectChannel(live);
		const auto beforePanel=core.sharedSnapshot();const auto panelWrites=disk->writes;
		core.simulateEvent(events::EventType::Follow);
		for(const auto type:{events::EventType::Follow,events::EventType::Sub,events::EventType::Cheer,events::EventType::Gift,events::EventType::Raid,events::EventType::Chat,events::EventType::HypeTrain,events::EventType::Shoutout})require(bool(core.simulatePanelEvent(type)),"Panel event admission failed");
		require(core.selectedChannel()==live && core.displayChannel().origin==events::EventOrigin::Preview && core.pendingEventCount()==1,"Panel drained live route or manual backlog");
		const auto panelKey=core.displayChannel();const auto panel=core.sharedSnapshot()->channels.at(panelKey);
		require(panel->daily.bits==100 && panel->daily.subs==6 && panel->activity.size()==5 && panel->chat.size()==1 && panel->hypeTrain && panel->hypeTrain->expiresAt>std::chrono::system_clock::now(),"Panel sources lack shared data or running Hype clock");
		require(state::dailyBook(core.sharedSnapshot())==state::dailyBook(beforePanel) && state::latestBook(core.sharedSnapshot())==state::latestBook(beforePanel) && state::activityBook(core.sharedSnapshot())==state::activityBook(beforePanel) && disk->writes==panelWrites,"Panel persisted synthetic events");
		const auto privateTest=core.createSourcePreview(false);core.endPanelTest();
		require(core.displayChannel()==live && !core.panelTestChannel() && core.sharedSnapshot()->channels.count(panelKey)==0 && core.sharedSnapshot()->channels.count({events::EventOrigin::Preview,privateTest})==1 && core.pendingEventCount()==1,"End panel TEST removed private TEST or unrelated pending event");
		core.releaseSourcePreview(privateTest);core.cancelPendingEvents();core.simulatePanelEvent(events::EventType::Follow);core.stop();core.start();require(!core.panelTestChannel(),"Panel TEST survived restart");
		std::cout << "PASS: source isolation, central queue delivery, pending preservation, daily persistence unchanged, Unicode ellipsis/cache, partial admission rollback, capacity and cleanup/restart\n";
		return 0;
	} catch(const std::exception &error) { std::cerr<<error.what()<<'\n'; return 1; }
}
