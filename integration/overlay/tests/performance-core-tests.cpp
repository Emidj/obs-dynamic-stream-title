// SPDX-License-Identifier: GPL-2.0-or-later
#include "core/plugin-core.hpp"
#include "events/simulation.hpp"
#include <iostream>
#include <chrono>
#include <stdexcept>
using namespace oto;
namespace {
void require(bool ok,const char *why){if(!ok)throw std::runtime_error(why);}
struct Config:IConfigStore{ConfigLoadResult load()override{return {ConfigStatus::Loaded,{false}};}bool save(Settings)override{return true;}};
struct Log:ILogger{void write(LogLevel,std::string_view)const noexcept override{}};
events::EventDraft draft(uint64_t n){auto d=events::makeSimulation(events::EventType(n%6),n,std::chrono::system_clock::now());if(n%6==5)d.payload=events::ShoutoutEvent{{"123","Target"},"target","Music",125000};return d;}
}
int main(){try{PluginCore core(std::make_unique<Config>(),std::make_unique<Log>());core.start();auto &queue=core.presentations();execution::PresentationConsumer a=0,b=0;execution::PresentationTypes types{};types.fill(true);const state::ChannelKey key{events::EventOrigin::Simulation,"sim-channel"};queue.subscribe(a,key,types);queue.subscribe(b,key,types);
const auto began=std::chrono::steady_clock::now();uint64_t admitted=0,delivered=0,played=0;
for(uint64_t n=1;n<=50000;++n){require(bool(core.processEvent(draft(n))),"Pressure admission");++admitted;if(n%32==0 || n==50000){while(core.pendingEventCount()){auto batch=core.processPendingEvents();delivered+=batch.processed;}while(const auto job=queue.next(a)){const auto second=queue.next(b);require(second && job->event==second->event,"Pressure: shared turn");queue.finish(a);require(!queue.next(a),"Pressure: overlap");queue.finish(b);++played;}}}
require(admitted==50000 && delivered==admitted && played==admitted && queue.pending()==0,"Pressure: loss/duplicates");
const auto snapshot=core.sharedSnapshot();require(snapshot->channels.at(key)->activity.size()<=64 && snapshot->channels.at(key)->shoutouts.size()<=16,"Unbounded shared history");
// Accepted playback deliberately retains a burst beyond the display history.
for(uint64_t n=50001;n<=60000;++n){require(bool(core.processEvent(draft(n))),"Burst admission");if(n%32==0)core.processPendingEvents();}while(core.pendingEventCount())core.processPendingEvents();require(queue.pending()==10000,"Burst lost playback");queue.pause(a);queue.pause(b);require(!queue.next(a) && queue.pending()==10000,"Pause lost commands");queue.subscribe(a,key,types);queue.subscribe(b,key,types);size_t count=0;while(const auto job=queue.next(a)){require(queue.next(b)->event==job->event,"Burst shared turn");queue.finish(a);queue.finish(b);++count;}require(count==10000 && queue.pending()==0,"Burst drain loss");
// Execution admission has explicit backpressure: already accepted work survives.
for(uint64_t n=60001;n<=60256;++n)require(bool(core.processEvent(draft(n))),"Capacity admission");const auto full=core.processEvent(draft(60257));require(full.queue.status==execution::EnqueueStatus::Full && core.pendingEventCount()==256,"Capacity must reject explicitly");while(core.pendingEventCount())core.processPendingEvents();count=0;while(queue.next(a)){queue.next(b);queue.finish(a);queue.finish(b);++count;}require(count==256,"Capacity lost accepted work");
for(int n=0;n<100;++n){core.stop();core.start();require(core.pendingEventCount()==0 && core.presentations().pending()==0 && core.sharedSnapshot()->channels.empty(),"Lifecycle retained transient state");}core.stop();
const double seconds=std::chrono::duration<double>(std::chrono::steady_clock::now()-began).count();std::cout<<"PASS 60256 accepted mixed events, 10000 deferred turns, 2 consumers, pause/resume, explicit 256 backpressure, 100 restart cycles; elapsed_s="<<seconds<<'\n';return 0;}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
