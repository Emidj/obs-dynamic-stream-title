#include "localization.hpp"
// SPDX-License-Identifier: GPL-2.0-or-later
#include "sources/hype-source.hpp"
#include "twitch/protocol.hpp"
#include "sources/hype-motion.hpp"
#include <QApplication>
#include <QTemporaryDir>
#include <QJsonArray>
#include <obs-module.h>
#include <iostream>
#include <limits>
using namespace oto;
const char *obs_module_text(const char *key){return key;}
namespace {
void require(bool ok,const char *why){if(!ok)throw std::runtime_error(why);}
struct Config:IConfigStore{ConfigLoadResult load()override{return {ConfigStatus::Loaded,{false}};}bool save(Settings)override{return true;}};
struct Log:ILogger{void write(LogLevel,std::string_view)const noexcept override{}};
void settle(){for(int n=0;n<8;++n)QCoreApplication::processEvents();}
}
int main(int argc,char **argv){QApplication app(argc,argv);try {
	require(obs_startup("en-US",nullptr,nullptr),"OBS startup");
	QTemporaryDir dir;auto core=std::make_shared<PluginCore>(std::make_unique<Config>(),std::make_unique<Log>());core->start();
	const auto wall=std::chrono::system_clock::now();const state::ChannelKey key{events::EventOrigin::Twitch,"123"};
	const auto send=[&](events::HypeTrainEvent event,int id,int seconds=0){const auto r=core->processEvent({"hype-"+std::to_string(id),key.channelId,wall+std::chrono::seconds(seconds),key.origin,std::move(event)});require(bool(r),"Hype admission");core->processPendingEventsFor(key.origin,key.channelId);};
	events::HypeTrainEvent event{events::HypeTrainStage::Progress,2,12500,10000};event.progress=4500;event.trainId="train-a";event.startedAt=wall-std::chrono::minutes(3);event.expiresAt=wall+std::chrono::seconds(60);event.contributors={{{"a","Álex_東京"},1200}};
	send(event,1);auto view=sources::hypeView(core->sharedSnapshot(),key,wall);
	require(view.visible && view.ratio==.45 && view.remaining==60 && view.total==12500 && view.progress==4500,"Progress used total or countdown wrong");
	// Progress may precede Begin; an older Begin must never regress the snapshot.
	auto begin=event;begin.stage=events::HypeTrainStage::Begin;begin.level=1;begin.progress=20;send(begin,2,-1);
	require(sources::hypeView(core->sharedSnapshot(),key,wall).level==2,"Late Begin regressed level");
	auto end=event;end.stage=events::HypeTrainStage::End;end.goal=0;end.progress.reset();end.expiresAt={};send(end,3,1);
	view=sources::hypeView(core->sharedSnapshot(),key,wall+std::chrono::seconds(1));require(view.ended && view.visible && view.goal==10000 && view.progress==4500,"End lost known progress");
	send(event,4,2);require(sources::hypeView(core->sharedSnapshot(),key,wall+std::chrono::seconds(2)).ended,"Closed train resurrected");
	require(sources::hypeView(core->sharedSnapshot(),key,wall+std::chrono::milliseconds(7999)).visible,"End skipped THANK YOU hold");
	require(!sources::hypeView(core->sharedSnapshot(),key,wall+std::chrono::seconds(8)).visible,"End did not suspend");
	event.trainId="train-b";send(event,5,3);view=sources::hypeView(core->sharedSnapshot(),key,wall+std::chrono::seconds(61));require(view.visible && view.waiting && !view.ended && view.remaining==0,"Expiry fabricated End");
	require(!sources::hypeView(core->sharedSnapshot(),key,wall+std::chrono::seconds(76)).visible,"Lost End never became idle");
	const auto channel=core->sharedSnapshot()->channels.at(key);require(channel->daily.bits==0 && channel->daily.subs==0 && channel->activity.empty(),"Hype modified supporter data");
	visual::VisualEngine visual;const auto mono=visual::MotionClock::now();view=sources::hypeView(core->sharedSnapshot(),key,wall);
	sources::HypeSettings settings;sources::HypeLabels labels;auto f=sources::prepareHypeFrame(visual,settings,labels,view,{},mono);
	const auto renders=visual.stats().renders;require(sources::prepareHypeFrame(visual,settings,labels,view,f,mono)==f && visual.stats().renders==renders,"Idle reraster");
	view.remaining=59;auto tick=sources::prepareHypeFrame(visual,settings,labels,view,f,mono+std::chrono::seconds(1));
	require(tick->base==f->base && tick->fill==f->fill && tick->labels==f->labels && tick->numbers==f->numbers && tick->names[0].frame==f->names[0].frame && visual.stats().renders==renders+1,"Countdown rebuilt static layers");
	view.ratio=.7;view.progress=7000;auto changed=sources::prepareHypeFrame(visual,settings,labels,view,tick,mono+std::chrono::seconds(2));
	require(changed->clockStart==f->clockStart && tick->clockStart==f->clockStart,"Updates restart particles or name scrolling");
	require(sources::hypeFill(*changed,mono+std::chrono::seconds(2))==.45 && sources::hypeFill(*changed,mono+std::chrono::seconds(3))==.7,"Progress jumps rather than interpolates");
	view.level=3;view.progress=100;view.ratio=.01;auto up=sources::prepareHypeFrame(visual,settings,labels,view,changed,mono+std::chrono::seconds(3));require(up->fromRatio==0,"Level up filled backwards");
	require(up->celebrated==mono+std::chrono::seconds(3),"Level celebration missing");
	const auto impact=sources::sampleHypeMotion(*up,mono+std::chrono::milliseconds(3300));
	require(impact.levelFlash>.8f && impact.levelScale>1.2f && impact.levelProgress>0,"Level-up lacks visible impact and number pulse");
	require(sources::sampleHypeMotion(*up,mono+std::chrono::seconds(3)).levelFlash==0 && sources::sampleHypeMotion(*up,mono+std::chrono::milliseconds(5600)).levelProgress<0,"Celebration boundaries are discontinuous or never settle");
	view.remaining=58;auto upTick=sources::prepareHypeFrame(visual,settings,labels,view,up,mono+std::chrono::milliseconds(3400));
	require(upTick->celebrated==up->celebrated,"Countdown restarted level celebration");
	auto bootstrap=sources::prepareHypeFrame(visual,settings,labels,view,{},mono);
	require(bootstrap->celebrated==visual::MotionClock::time_point{},"High-level bootstrap celebrates old activity");
	view.level=2;auto correction=sources::prepareHypeFrame(visual,settings,labels,view,bootstrap,mono+std::chrono::seconds(4));
	require(correction->celebrated==visual::MotionClock::time_point{},"Backwards correction celebrates");
	view.level=4;view.trainId="different-train";auto nextTrain=sources::prepareHypeFrame(visual,settings,labels,view,up,mono+std::chrono::seconds(4));
	require(nextTrain->celebrated==visual::MotionClock::time_point{},"New train replays a level celebration");
	view.trainId=up->view.trainId;view.ended=true;auto interrupted=sources::prepareHypeFrame(visual,settings,labels,view,up,mono+std::chrono::milliseconds(3500));
	require(interrupted->celebrated==visual::MotionClock::time_point{} && sources::sampleHypeMotion(*interrupted,mono+std::chrono::milliseconds(3600)).levelFlash==0,"End retains level-up energy");view.ended=false;view.level=3;
	auto entrance=sources::prepareHypeFrame(visual,settings,labels,view,{},mono,true);
	const auto pose=[&](int ms){return sources::sampleHypeMotion(*entrance,mono+std::chrono::milliseconds(ms));};
	require(pose(350).title==1 && pose(350).titleScale>1 && pose(350).outline==0 && pose(350).bar==0 && pose(350).details==0,"Title entrance is not independent");
	require(pose(1100).outline>0 && pose(1100).bar==0 && pose(1100).details==0,"Contour stage missing");
	require(pose(1700).bar>0 && pose(1700).details==0,"Bar/level stage missing");
	require(pose(2500).details==1,"Remaining elements stage missing");
	view.ended=true;auto closing=sources::prepareHypeFrame(visual,settings,labels,view,entrance,mono+std::chrono::seconds(4));
	const auto ending=[&](int ms){return sources::sampleHypeMotion(*closing,mono+std::chrono::seconds(4)+std::chrono::milliseconds(ms));};
	require(closing->title==entrance->title && closing->timer==entrance->timer && closing->thanks && ending(0).gather==0 && ending(0).morph==0,"End changes title or drops countdown before collecting");
	require(ending(500).gather>0 && ending(500).gather<1 && ending(500).morph==0,"Collect step missing");
	require(ending(1100).gather==1 && ending(1100).morph>0 && ending(1100).morph<1,"Morph step missing");
	require(ending(1500).morph==1 && ending(5499).leave==0 && !ending(5499).finished,"THANK YOU hold not respected");
	require(ending(6000).leave>0 && !ending(6000).finished && ending(7000).finished,"Exit duration not respected");
	view.ended=false;
	for(const auto size:{QSize(420,120),QSize(1920,400)}){settings.width=size.width();settings.height=size.height();settings.numberSize=settings.labelSize=128;auto bounds=sources::prepareHypeFrame(visual,settings,labels,view,{},mono);require(QRect(QPoint(),size).contains(bounds->bar),"Bar outside source");for(const auto &n:bounds->names)if(n.frame)require(QRect(QPoint(),size).contains(n.box),"Contributor outside source");}
	// Native adapter: current-level progress, nanosecond deadline, malformed fields.
	QJsonObject json{{"id","native-train"},{"level",2},{"total",12000},{"progress",3000},{"goal",10000},{"started_at","2026-10-04T09:00:00Z"},{"expires_at","2026-10-04T09:05:00.123456789Z"},{"top_contributions",QJsonArray{QJsonObject{{"user_id","a"},{"user_name","Viewer"},{"type","bits"},{"total",200}},QJsonObject{{"user_id","a"},{"user_name","Viewer"},{"type","subscription"},{"total",500}}}}};
	const auto mapped=twitch::hypeTrain(json,events::HypeTrainStage::Progress);require(mapped && mapped->progress==3000 && mapped->total==12000 && mapped->contributors.size()==1 && mapped->contributors[0].points==700 && mapped->contributors[0].methods==3,"Bits/subscription contributions not combined");
	require(mapped->contributors[0].bits==200 && mapped->contributors[0].subscriptionPoints==500 && !mapped->contributors[0].subscriptions,"API points fabricated a subscription count");
	require(sources::hypeContributionLabels(mapped->contributors[0])==std::array<QString,2>{i18n::qtext("Unit.Bits").arg("200"),i18n::qtext("Unit.Subs").arg(QString::fromUtf8("—"))},"Mixed amounts still show combined points");
	json.insert("expires_at","invalid");require(!twitch::hypeTrain(json,events::HypeTrainStage::Progress),"Malformed deadline accepted");
	{
		auto counts=std::make_shared<PluginCore>(std::make_unique<Config>(),std::make_unique<Log>());counts->start();
		const auto ingest=[&](std::string id,events::EventPayload payload,int seconds){const auto r=counts->processEvent({id,key.channelId,wall+std::chrono::seconds(seconds),key.origin,std::move(payload)});require(bool(r),"Contribution admission");counts->processPendingEventsFor(key.origin,key.channelId);};
		const events::User donor{"a","Viewer"};
		ingest("gift-t3",events::GiftEvent{donor,1,events::SubscriptionTier::Tier3},1);
		ingest("gift-t1",events::GiftEvent{donor,2,events::SubscriptionTier::Tier1},1);
		ingest("recipient",events::SubEvent{donor,events::SubscriptionTier::Tier1,1,false},1);
		ingest("resub-message",events::SubEvent{donor,events::SubscriptionTier::Tier3,20,false},1);
		require(!counts->processEvent({"gift-t3",key.channelId,wall+std::chrono::seconds(1),key.origin,events::GiftEvent{donor,1,events::SubscriptionTier::Tier3}}),"Duplicate gift accepted");
		auto train=*mapped;train.trainId="counts-train";train.startedAt=wall;train.expiresAt=wall+std::chrono::seconds(60);train.contributors[0].points=3700;train.contributors[0].subscriptionPoints=3500;
		ingest("counts-begin",train,1);
		const auto contribution=[&](){return counts->sharedSnapshot()->channels.at(key)->hypeTrain->contributors[0];};
		require(contribution().subscriptions==3 && sources::hypeContributionLabels(contribution())==std::array<QString,2>{i18n::qtext("Unit.Bits").arg("200"),i18n::qtext("Unit.Subs").arg("3")},"Tier 3 or gift pack counted as Tier 1 points");
		ingest("sub-t2",events::SubEvent{donor,events::SubscriptionTier::Tier2},2);
		require(!contribution().subscriptions,"Unmatched summaries claim an exact count");
		train.contributors[0].subscriptionPoints=4500;train.contributors[0].points=4700;ingest("counts-progress",train,2);
		require(contribution().subscriptions==4,"Subscription before Hype update lost");
		train.contributors[0].subscriptionPoints=5000;train.contributors[0].points=5200;ingest("counts-before-gift",train,3);
		require(!contribution().subscriptions,"Missing gift invents quantity");
		ingest("gift-late",events::GiftEvent{donor,1},3);require(contribution().subscriptions==5,"Late gift does not repair the displayed count");
		// The event list's 64-row retention must not erase train quantities.
		for(int n=0;n<80;++n)ingest("burst-sub-"+std::to_string(n),events::SubEvent{donor},3);
		train.contributors[0].subscriptionPoints=45000;train.contributors[0].points=45200;ingest("counts-burst",train,3);
		require(contribution().subscriptions==85,"Activity display retention loses train contributions");
		train.stage=events::HypeTrainStage::End;ingest("counts-end",train,4);ingest("post-train",events::SubEvent{donor},5);
		require(contribution().subscriptions==85,"Subscription after End alters finished train");
		train.stage=events::HypeTrainStage::Begin;train.trainId="bootstrap";train.startedAt=wall+std::chrono::seconds(6);train.contributors[0].subscriptionPoints=1000;ingest("counts-bootstrap",train,6);
		require(!contribution().subscriptions,"Mid-train bootstrap converts ambiguous points into subs");counts->stop();
	}
	// The actual production hub: independent TEST channels and suspension.
	core->selectChannel(key);auto service=std::make_shared<twitch::Service>(core,dir.filePath("session.bin"));auto engine=std::make_shared<visual::VisualEngine>();auto renderer=std::make_shared<ObsVisualRenderer>();
	auto hub=std::make_shared<sources::HypeHub>(core,engine,renderer,service);auto a=std::make_shared<sources::HypeInstance>(),b=std::make_shared<sources::HypeInstance>();a->visible=b->visible=true;hub->request(a);hub->request(b);settle();require(hub->timerActive(),"Active countdown timer missing");
	core->setStateChanged([&]{hub->refresh();});const auto normal=std::atomic_load(&b->frame);
	require(hub->preview(a,0),"TEST start");require(a->previewChannel!=b->previewChannel && std::atomic_load(&b->frame)==normal,"TEST affected second meter");require(hub->preview(a,1) && std::atomic_load(&a->frame)->view.progress==4150,"TEST progress");require(hub->preview(a,2) && std::atomic_load(&a->frame)->view.level==4,"TEST level");require(hub->preview(a,3) && std::atomic_load(&a->frame)->view.ended,"TEST end");require(hub->preview(a,4) && a->previewChannel.empty(),"TEST cleanup");
	require(hub->preview(a,0),"Automatic TEST start");const auto initialTest=std::atomic_load(&a->frame);
	for(int n=0;n<4;++n)require(hub->preview(a,1),"Automatic TEST progress");
	const auto almost=std::atomic_load(&a->frame);require(almost->view.level==3 && almost->view.progress==4900 && almost->celebrated==visual::MotionClock::time_point{},"TEST advances before goal");
	require(hub->preview(a,1),"Automatic TEST level");const auto automatic=std::atomic_load(&a->frame);
	require(automatic->view.level==4 && automatic->view.goal==5500 && automatic->view.progress==150 && automatic->view.total==10150,"TEST fails automatic level or loses overflow points");
	require(automatic->celebrated!=visual::MotionClock::time_point{} && automatic->appeared==initialTest->appeared && automatic->clockStart==initialTest->clockStart,"Automatic TEST omits celebration or replays entrance");
	for(int n=0;n<22;++n)require(hub->preview(a,1),"Next automatic TEST progress");
	const auto nextLevel=std::atomic_load(&a->frame);require(nextLevel->view.level==5 && nextLevel->view.goal==6000 && nextLevel->view.progress==150,"TEST only advances once");
	// Set an exact boundary through the same normalized preview event path.
	auto exact=*core->sharedSnapshot()->channels.at({events::EventOrigin::Preview,a->previewChannel})->hypeTrain;exact.progress=5750;
	require(bool(core->processEvent({"hype-exact-boundary",a->previewChannel,std::chrono::system_clock::now(),events::EventOrigin::Preview,exact})),"Exact TEST fixture");core->processPendingEventsFor(events::EventOrigin::Preview,a->previewChannel);
	require(hub->preview(a,1),"Exact automatic TEST level");const auto exactLevel=std::atomic_load(&a->frame);
	require(exactLevel->view.level==6 && exactLevel->view.progress==0 && exactLevel->view.goal==6500,"Exact 100 percent stays stuck or fabricates carry");
	require(std::atomic_load(&b->frame)==normal && core->sharedSnapshot()->channels.at(key)->daily.bits==0 && core->sharedSnapshot()->channels.at(key)->daily.subs==0,"Automatic TEST alters another source or real counters");
	require(hub->preview(a,4),"Automatic TEST cleanup");
	require(hub->preview(a,5) && std::atomic_load(&a->frame)->names[0].distance>0,"LONG TEXT does not scroll");
	require(std::atomic_load(&b->frame)==normal,"LONG TEXT affected second meter");require(hub->preview(a,4),"LONG cleanup");
	a->visible=b->visible=false;hub->refresh();require(!hub->timerActive(),"Hidden meters keep timer alive");
	core->setStateChanged({});hub->stop();service->stop();core->stop();renderer.reset();obs_shutdown();
	std::cout<<"PASS: Hype protocol/current-level progress, out-of-order closure, deadlines/idle, selective layers, bounds and production private previews\n";return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
