// SPDX-License-Identifier: GPL-2.0-or-later
#include "sources/alert-source.hpp"
#include "sources/shoutout-source.hpp"
#include "events/simulation.hpp"
#include <obs-module.h>
#include <QGuiApplication>
#include <QTemporaryDir>
#include <QFileInfo>
#include <QDir>
#include <iostream>
#include <stdexcept>
const char *obs_module_text(const char *key){return key;}
using namespace oto;
namespace {
void require(bool ok,const char *why){if(!ok)throw std::runtime_error(why);}
struct Config:IConfigStore{ConfigLoadResult load()override{return {ConfigStatus::Loaded,{false}};}bool save(Settings)override{return true;}};
struct Log:ILogger{void write(LogLevel,std::string_view)const noexcept override{}};
void settle(){for(int n=0;n<4;++n)QCoreApplication::processEvents();}
void age(const std::shared_ptr<sources::AlertInstance> &source,int milliseconds){auto frame=std::make_shared<sources::AlertFrame>(*std::atomic_load(&source->frame));frame->started=visual::MotionClock::now()-std::chrono::milliseconds(milliseconds);std::atomic_store(&source->frame,sources::AlertFramePtr(frame));}
void age(const std::shared_ptr<sources::ShoutoutInstance> &source,int milliseconds){auto frame=std::make_shared<sources::ShoutoutFrame>(*std::atomic_load(&source->frame));frame->started=visual::MotionClock::now()-std::chrono::milliseconds(milliseconds);std::atomic_store(&source->frame,sources::ShoutoutFramePtr(frame));}
}
int main(int argc,char **argv){QGuiApplication app(argc,argv);try{
	QTemporaryDir dir;require(obs_startup("en-US",nullptr,nullptr),"OBS startup failed");
	auto core=std::make_shared<PluginCore>(std::make_unique<Config>(),std::make_unique<Log>());core->start();
	auto service=std::make_shared<twitch::Service>(core,dir.filePath("session.bin"));auto visual=std::make_shared<visual::VisualEngine>();
	auto registry=std::make_shared<presets::Registry>(presets::Registry::fromFile(OTO_PRESET_FILE));auto assets=std::make_shared<visual::AssetManager>(QDir(QFileInfo(OTO_PRESET_FILE).absolutePath()).filePath("assets"));
	auto alerts=std::make_shared<sources::AlertHub>(core,visual,nullptr,registry,assets,service);auto shoutouts=std::make_shared<sources::ShoutoutHub>(core,visual,nullptr,registry,assets,service);
	core->setStateChanged([&]{alerts->refresh();shoutouts->refresh();});core->presentations().setChanged([&]{QMetaObject::invokeMethod(alerts.get(),[&]{alerts->refresh();shoutouts->refresh();},Qt::QueuedConnection);});
	auto a=std::make_shared<sources::AlertInstance>();auto b=std::make_shared<sources::AlertInstance>();auto s=std::make_shared<sources::ShoutoutInstance>();a->visible=b->visible=s->visible=true;a->settings.times={1000,6000,3000};b->settings.times={1000,9000,3000};s->settings.times={1000,6000,3000};
	alerts->request(a);alerts->request(b);shoutouts->request(s);settle();
	const auto submit=[&](events::EventType type,int id){auto draft=events::makeSimulation(type,id,std::chrono::system_clock::now());draft.id="sim-"+std::to_string(id);if(type==events::EventType::Shoutout)draft.payload=events::ShoutoutEvent{{"123","Target"},"target","Games",125};const auto result=core->processEvent(std::move(draft));require(bool(result),"Source fixture admission failed");core->processPendingEvents();return result.event;};
	const auto first=submit(events::EventType::Follow,1);const auto second=submit(events::EventType::Shoutout,2);submit(events::EventType::Follow,3);
	require(std::atomic_load(&a->frame)->event==first && std::atomic_load(&b->frame)->event==first && !std::atomic_load(&s->frame),"Production hubs overlap alert and SO");
	age(a,9000);age(b,12000);alerts->refresh();settle();require(!std::atomic_load(&s->frame),"Shoutout began during alert exit");
	age(a,10001);alerts->refresh();settle();require(!std::atomic_load(&a->frame) && !std::atomic_load(&s->frame),"Short instance bypassed longer alert");
	age(b,13001);alerts->refresh();settle();require(std::atomic_load(&s->frame)->event.event==second && !std::atomic_load(&a->frame),"SO wasn't woken by completed alert");
	age(s,9000);shoutouts->refresh();settle();require(!std::atomic_load(&a->frame),"Alert began during SO exit");age(s,10001);shoutouts->refresh();settle();require(std::atomic_load(&a->frame) && !std::atomic_load(&s->frame),"Alert wasn't woken by SO completion");
	// Preferences retain active clock and backlog, including more than 16 cards.
	const auto started=std::atomic_load(&a->frame)->started;a->preferences=true;alerts->refresh();require(std::atomic_load(&a->frame)->started==started,"Properties restarted live timeline");
	for(int n=4;n<34;++n)submit(events::EventType::Shoutout,n);require(core->presentations().pending()==31,"Production backlog truncated to SO history");
	age(a,10001);age(b,13001);alerts->refresh();settle();
	for(int n=4;n<34;++n){const auto frame=std::atomic_load(&s->frame);require(frame && frame->event.event->data().id=="sim-"+std::to_string(n),"Production hub lost/reordered a queued card");age(s,10001);shoutouts->refresh();settle();}
	require(core->presentations().pending()==0 && !std::atomic_load(&s->frame),"Queue didn't become transparent/idle");
	submit(events::EventType::Shoutout,34);s->visible=false;shoutouts->refresh();require(core->presentations().pending()==1 && !std::atomic_load(&s->frame),"Hide discarded accepted SO");
	submit(events::EventType::Follow,35);require(bool(std::atomic_load(&a->frame)),"Hidden SO blocked alerts");s->visible=true;shoutouts->refresh();require(!std::atomic_load(&s->frame),"Resume overlapped active alert");age(a,10001);age(b,13001);alerts->refresh();settle();require(std::atomic_load(&s->frame)->event.event->data().id=="sim-34","Accepted SO did not resume after hide");age(s,10001);shoutouts->refresh();settle();
	// Panel TEST joins playback without replacing the live consumer.
	const auto live=submit(events::EventType::Shoutout,36);const auto liveStart=std::atomic_load(&s->frame)->started;
	core->simulatePanelEvent(events::EventType::Follow);core->simulatePanelEvent(events::EventType::Shoutout);settle();
	require(std::atomic_load(&s->frame)->event.event==live && std::atomic_load(&s->frame)->started==liveStart && core->presentations().pending()==3,"Panel TEST interrupted live SO or lost backlog");
	age(s,10001);shoutouts->refresh();settle();require(std::atomic_load(&a->frame) && std::atomic_load(&a->frame)->event->data().origin==events::EventOrigin::Preview,"Panel Follow never reached production alerts");
	age(a,10001);age(b,13001);alerts->refresh();settle();require(std::atomic_load(&s->frame) && std::atomic_load(&s->frame)->event.event->data().origin==events::EventOrigin::Preview,"Panel SO stalled after Follow completion");
	age(s,10001);shoutouts->refresh();settle();require(core->presentations().pending()==0 && !std::atomic_load(&s->frame),"Panel presentations did not finish");
	core->simulatePanelEvent(events::EventType::Cheer);submit(events::EventType::Follow,37);core->endPanelTest();settle();
	require(std::atomic_load(&a->frame) && std::atomic_load(&a->frame)->event->data().id=="sim-37" && core->presentations().pending()==1,"Ending panel TEST lost next live alert");
	age(a,10001);age(b,13001);alerts->refresh();settle();
	core->simulatePanelEvent(events::EventType::Shoutout);s->visible=false;shoutouts->refresh();core->endPanelTest();settle();require(core->presentations().pending()==0,"Hidden source retained ended TEST");s->visible=true;shoutouts->refresh();settle();require(!std::atomic_load(&s->frame),"Ended TEST resurfaced");
	core->setStateChanged({});core->presentations().setChanged({});alerts->stop();shoutouts->stop();service->stop();core->stop();obs_shutdown();
	std::cout<<"Production hubs: mixed turns, full exit, two durations, 30 queued SOs, unchanged settings and idle passed\n";return 0;
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}}
