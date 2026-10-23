// SPDX-License-Identifier: GPL-2.0-or-later
#include "core/plugin-core.hpp"
#include "events/simulation.hpp"
#include <iostream>
#include <stdexcept>
using namespace oto;
namespace {
void require(bool ok,const char *why){if(!ok)throw std::runtime_error(why);}
struct Config:IConfigStore{ConfigLoadResult load()override{return {ConfigStatus::Loaded,{false}};}bool save(Settings)override{return true;}};
struct Log:ILogger{void write(LogLevel,std::string_view)const noexcept override{}};
struct Daily:IDailyStore{bool fail=false;state::DailyBook load()override{return {};}bool save(const state::DailyBook &)override{return !fail;}};
events::EventDraft draft(events::EventType type,int id){auto item=events::makeSimulation(type,id,std::chrono::system_clock::now());if(type==events::EventType::Shoutout)item.payload=events::ShoutoutEvent{{"123","Target"},"target","Games",123};return item;}
}
int main(){try{
	auto daily=std::make_unique<Daily>();auto *storage=daily.get();PluginCore core(std::make_unique<Config>(),std::make_unique<Log>(),std::move(daily));core.start();
	auto &queue=core.presentations();execution::PresentationConsumer a=0,b=0,s=0;execution::PresentationTypes alerts{},shoutouts{};for(int n=0;n<5;++n)alerts[n]=true;shoutouts[state::slot(events::EventType::Shoutout)]=true;
	const state::ChannelKey channel{events::EventOrigin::Simulation,"sim-channel"};queue.subscribe(a,channel,alerts);queue.subscribe(b,channel,alerts);queue.subscribe(s,channel,shoutouts);
	std::vector<events::EventPtr> expected;
	for(int n=1;n<=120;++n){const auto result=core.processEvent(draft(n%2?events::EventType::Follow:events::EventType::Shoutout,n));require(bool(result),"Burst admission failed");}
	while(core.pendingEventCount()){const auto batch=core.processPendingEvents();expected.insert(expected.end(),batch.delivered.begin(),batch.delivered.end());}
	require(queue.pending()==120,"Playback truncated to visible history capacity");
	int notifications=0;queue.setChanged([&]{++notifications;});
	for(const auto &event:expected){const bool so=events::typeOf(event->data().payload)==events::EventType::Shoutout;
		const auto first=queue.next(so?s:a);require(first && first->event==event,"Central delivery order changed");require(!queue.next(so?a:s),"Different source acquired overlapping turn");
		if(!so){require(queue.next(b)->event==event,"Shared instances did not receive same event");queue.finish(a);require(!queue.next(a) && queue.next(b)->event==event,"Short instance released other instance's hold/exit");queue.finish(b);}else queue.finish(s);
	}
	require(queue.pending()==0 && notifications==120,"Completion did not wake next source exactly once");
	core.processEvent(draft(events::EventType::Shoutout,123));core.processPendingEvents();queue.next(s);queue.pause(s);
	core.processEvent(draft(events::EventType::Follow,124));core.processPendingEvents();require(queue.next(a) && !queue.next(s),"Hidden source blocked other alerts");
	queue.subscribe(s,channel,shoutouts);require(!queue.next(s),"Resuming hidden source interrupted active alert");queue.finish(a);queue.finish(b);require(queue.next(s) && queue.pending()==1,"Accepted hidden SO was discarded");queue.finish(s);require(queue.pending()==0,"Resumed SO wasn't completed");
	// Persistence failure/retry is transactional: neither duplicate nor early playback.
	storage->fail=true;require(bool(core.processEvent(draft(events::EventType::Follow,121))),"Retry admission failed");bool failed=false;try{core.processPendingEvents();}catch(...){failed=true;}require(failed && queue.pending()==0 && core.pendingEventCount()==1,"Failed persistence leaked a playback job");storage->fail=false;core.processPendingEvents();require(queue.pending()==1,"Retry duplicated/lost presentation");
	const auto queued=queue.next(a)->event;
	const auto preview=core.createShoutoutPreview(false);require(queue.pending()==1 && queue.next(a)->event==queued,"Private TEST replaced a real queued event");core.releaseSourcePreview(preview);
	queue.remove(a);require(queue.pending()==1 && queue.next(b)->event==queued,"Removing one instance discarded other participant");queue.remove(b);require(queue.pending()==0,"Removed final participant blocked queue");
	queue.subscribe(a,channel,alerts);require(bool(core.processEvent(draft(events::EventType::Shoutout,122))),"Channel admission failed");core.processPendingEvents();queue.subscribe(s,{events::EventOrigin::Simulation,"other"},shoutouts);require(queue.pending()==0,"Channel change kept a stale presentation");
	core.stop();core.start();require(queue.pending()==0,"Restart replayed transient presentations");core.stop();
	std::cout<<"120 mixed events retained; shared turns, participants, transactional retry, TEST isolation, removal/channel/restart passed\n";return 0;
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}}
