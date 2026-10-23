// SPDX-License-Identifier: GPL-2.0-or-later
#include "sources/alert-model.hpp"
#include "core/plugin-core.hpp"
#include "visual/asset-manager.hpp"
#include <QGuiApplication>
#include <QFileInfo>
#include <QDir>
#include <QBuffer>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <limits>
#include <iostream>
#include <stdexcept>
using namespace oto;
namespace {
void require(bool ok,const char *why){if(!ok)throw std::runtime_error(why);}
struct Config:IConfigStore{ConfigLoadResult load()override{return {ConfigStatus::Loaded,{false}};}bool save(Settings)override{return true;}};
struct Log:ILogger{void write(LogLevel,std::string_view)const noexcept override{}};
struct Daily:IDailyStore{state::DailyBook book;int writes=0;state::DailyBook load()override{return book;}bool save(const state::DailyBook &next)override{book=next;++writes;return true;}};
}
int main(int argc,char **argv){QGuiApplication app(argc,argv);try{
	auto disk=std::make_unique<Daily>();auto *storage=disk.get();PluginCore core(std::make_unique<Config>(),std::make_unique<Log>(),std::move(disk));core.start();
	const state::ChannelKey channel{events::EventOrigin::Simulation,"sim-channel"};sources::AlertCursor a,b;
	core.simulateEvent(events::EventType::Follow);core.processPendingEvents();a.synchronize(core.sharedSnapshot(),channel);b.synchronize(core.sharedSnapshot(),channel);
	require(!a.next(core.sharedSnapshot(),{true,true,true,true,true}),"New source replayed old events");
	const auto first=core.simulateEvent(events::EventType::Sub).event;auto late=first->data();late.id="late-follower";late.occurredAt-=std::chrono::hours(1);late.payload=events::FollowEvent{{"7","LateUser"}};const auto second=core.processEvent(late).event;
	core.processPendingEvents();const auto one=a.next(core.sharedSnapshot(),{true,true,true,true,true});require(one && (one->event==first || one->event==second),"Same-batch events lost");
	const auto two=a.next(core.sharedSnapshot(),{true,true,true,true,true});require(two && two->event!=one->event && (two->event==first || two->event==second) && two->delivery>one->delivery,"Delivery order skipped or collapsed same-batch/late events");
	require(b.next(core.sharedSnapshot(),{true,true,true,true,true})->event==one->event,"Independent cursor missed shared event");
	require(!a.next(core.sharedSnapshot(),{true,true,true,true,true}),"Event replayed after completion");
	a.reset();a.synchronize(core.sharedSnapshot(),channel);require(!a.next(core.sharedSnapshot(),{true,true,true,true,true}),"Hidden source replayed events on show");
	core.simulateEvent(events::EventType::Gift);core.processPendingEvents();require(!a.next(core.sharedSnapshot(),{true,true,true,false,true}),"Disabled type rendered");require(!a.next(core.sharedSnapshot(),{true,true,true,true,true}),"Enabling filter replayed consumed events");
	for(int n=0;n<80;++n)core.simulateEvent(events::EventType::Follow),core.processPendingEvents();require(core.sharedSnapshot()->channels.at(channel)->activity.size()==64,"Shared activity lost its bound");
	a.reset();a.synchronize(core.sharedSnapshot(),channel);late.id="late-at-capacity";const auto delayed=core.processEvent(late).event;core.processPendingEvents();const auto retained=a.next(core.sharedSnapshot(),{true,true,true,true,true});require(retained && retained->event==delayed && core.sharedSnapshot()->channels.at(channel)->activity.size()==64,"Late timestamp discarded a newly delivered alert at capacity");
	const auto registry=presets::Registry::fromFile(OTO_PRESET_FILE);const auto *style=registry.find("neon-dj");require(style && style->alertStyle,"Neon alert metadata missing");
	visual::AssetManager assets(QDir(QFileInfo(OTO_PRESET_FILE).absolutePath()).filePath("assets"));const auto atlas=assets.load(style->alertStyle->atlas);require(atlas->image.width()<=1024 && atlas->image.height()<=1024,"Atlas residency exceeds budget");
	visual::VisualEngine engine;constexpr events::EventType types[]={events::EventType::Follow,events::EventType::Sub,events::EventType::Gift,events::EventType::Gift,events::EventType::Raid,events::EventType::Cheer};
	const auto book=storage->book;const auto writes=storage->writes;core.simulateEvent(events::EventType::Sub);const auto pending=core.pendingEventCount();
	for(int variant=0;variant<6;++variant){const auto preview=core.createAlertPreview(types[variant],variant==3,false,{"123","Viewer"});const auto &entry=core.sharedSnapshot()->channels.at({events::EventOrigin::Preview,preview})->activity.front();require(sources::alertVariant(entry.event->data().payload)==variant,"Alert type/pack mapping wrong");
		sources::AlertSettings settings;const auto clip=assets.load(style->alertStyle->iconAnimations[variant]);const auto frame=sources::prepareAlertFrame(engine,settings,*style->alertStyle,entry,atlas,{},visual::MotionClock::now(),clip);require(frame->event==entry.event && frame->displayName==entry.displayName && frame->name && frame->portrait && frame->particles<=12 && frame->animatedEmblem,"Prepared alert did not retain shared references/animation");
		require(frame->emblem->image.size()==QSize(1024,768),"Animated icon sheet changed dimensions");
		for(int n=0;n<48;++n){const auto cell=sources::alertIconCell((n+.5)*1000/24);require(cell==QRect((n%8)*128,(n/8)*128,128,128),"Clip frame selection/row ordering failed");require(clip->image.rect().contains(cell),"Clip frame left texture");}
		require(sources::alertIconCell(2000)==sources::alertIconCell(0) && sources::alertIconCell(80000)==sources::alertIconCell(0),"Long duration clip wrap failed");
		for(const auto &metric:frame->heading->textMetrics)require(!metric.overflow && !metric.elided,"Headline doesn't fit");core.releaseSourcePreview(preview);
	}
	require(storage->book==book && storage->writes==writes && core.pendingEventCount()==pending,"TEST changed storage or drained unrelated events");
	const auto *cyber=registry.find("cyberpunk");require(cyber && cyber->alertStyle,"Cyberpunk alert metadata missing");
	const auto &cyberStyle=*cyber->alertStyle;require(cyberStyle.energyPattern==1 && cyberStyle.atlas.isEmpty(),"Cyberpunk didn't use independent angular/clip profile");
	visual::AssetManager cyberAssets(QDir(QFileInfo(OTO_PRESET_FILE).absolutePath()).filePath("assets"));
	for(int variant=0;variant<6;++variant){
		const auto key=core.createAlertPreview(types[variant],variant==3,false,{"123","ShortName"});const auto &item=core.sharedSnapshot()->channels.at({events::EventOrigin::Preview,key})->activity.front();
		sources::AlertSettings chosen;chosen.presetId="cyberpunk";chosen.nameFamily="Consolas";chosen.labelFamily="Arial";
		const auto clip=cyberAssets.load(cyberStyle.iconAnimations[variant]);const auto prepared=sources::prepareAlertFrame(engine,chosen,cyberStyle,item,{}, {},visual::MotionClock::now(),clip);
		require(prepared->animatedEmblem && prepared->energyPattern==1 && prepared->settings.nameFamily==chosen.nameFamily && prepared->settings.labelFamily==chosen.labelFamily,"Cyberpunk failed shared clip/font preparation");
		require(prepared->badge->image.width()==prepared->nameViewport+40 && prepared->badge->image.width()<=480,"Cyberpunk lost automatic plate padding/bound");
		for(const auto &metric:prepared->heading->textMetrics)require(!metric.overflow && !metric.elided,"Cyberpunk heading doesn't fit diagonal band");
		require(clip->image.copy(sources::alertIconCell(0))!=clip->image.copy(sources::alertIconCell(500)),"Cyberpunk icon remained static");
		core.releaseSourcePreview(key);
	}
	require(cyberAssets.stats().loads==6 && cyberAssets.stats().bytes<=8*1024*1024 && storage->book==book && storage->writes==writes,"Cyberpunk eagerly loaded unrelated resources or changed daily state");
	QFile metadata(OTO_PRESET_FILE);require(metadata.open(QIODevice::ReadOnly),"Alert metadata fixture unavailable");auto object=QJsonDocument::fromJson(metadata.readAll()).object();auto definitions=object["presets"].toArray();
	for(int n=0;n<definitions.size();++n)if(definitions[n].toObject()["id"]=="cyberpunk"){
		const auto original=definitions[n].toObject();for(const auto change:{QJsonObject{{"energyPattern",10}},QJsonObject{{"iconAnimations",QJsonArray{}}},QJsonObject{{"headlineColor","red"}}}){
			auto altered=original,styleObject=original["alertStyle"].toObject();for(auto i=change.begin();i!=change.end();++i)styleObject[i.key()]=i.value();altered["alertStyle"]=styleObject;definitions[n]=altered;object["presets"]=definitions;
			bool rejected=false;try{presets::Registry::fromJson(QJsonDocument(object).toJson());}catch(const std::invalid_argument &){rejected=true;}require(rejected,"Invalid angular profile/clip list/color accepted");
		}definitions[n]=original;
	}
	const auto *crt=registry.find("retro-crt");require(crt && crt->alertStyle && crt->alertStyle->energyPattern==2,"CRT profile missing");
	visual::AssetManager crtAssets(QDir(QFileInfo(OTO_PRESET_FILE).absolutePath()).filePath("assets"));
	QImage photo(32,32,QImage::Format_RGBA8888_Premultiplied);photo.fill(QColor(200,90,50));auto crtPortrait=std::make_shared<visual::ImageData>(visual::ImageData{photo});
	for(int variant=0;variant<6;++variant){
		const auto key=core.createAlertPreview(types[variant],variant==3,false,{"123","Viewer"});const auto &item=core.sharedSnapshot()->channels.at({events::EventOrigin::Preview,key})->activity.front();
		sources::AlertSettings selected;selected.presetId="retro-crt";selected.nameFamily="Arial";selected.labelFamily="Consolas";
		const auto clip=crtAssets.load(crt->alertStyle->iconAnimations[variant]);const auto prepared=sources::prepareAlertFrame(engine,selected,*crt->alertStyle,item,{},crtPortrait,visual::MotionClock::now(),clip);
		require(prepared->energyPattern==2 && prepared->animatedEmblem && prepared->profileImage==crtPortrait,"CRT lost shared image/clip profile");
		require(prepared->portrait->image.pixelColor(0,0).alpha()==255,"CRT portrait still circular instead of a television picture");
		require(prepared->heading->image.size()==QSize(540,110) && prepared->badge->image.width()==prepared->nameViewport+40,"CRT layout exceeds tube or loses centred plate");
		for(const auto &metric:prepared->heading->textMetrics)require(!metric.overflow && !metric.elided,"CRT title doesn't fit tube");
		require(clip->image.copy(sources::alertIconCell(0))!=clip->image.copy(sources::alertIconCell(500)),"CRT pixel icon remained static");core.releaseSourcePreview(key);
	}
	require(crtAssets.stats().loads==6 && crtAssets.stats().bytes<=8*1024*1024 && storage->book==book && storage->writes==writes && core.pendingEventCount()==pending,"CRT loads unrelated resources or changes central TEST state");
	const auto *space=registry.find("space");require(space && space->alertStyle && space->alertStyle->energyPattern==3 && space->alertStyle->atlas.isEmpty(),"Space shared orbital profile missing");
	visual::AssetManager spaceAssets(QDir(QFileInfo(OTO_PRESET_FILE).absolutePath()).filePath("assets"));
	for(int variant=0;variant<6;++variant){
		const auto key=core.createAlertPreview(types[variant],variant==3,false,{"123","Astronaut"});const auto &item=core.sharedSnapshot()->channels.at({events::EventOrigin::Preview,key})->activity.front();
		sources::AlertSettings selected;selected.presetId="space";selected.nameFamily="Arial";selected.labelFamily="Consolas";
		const auto clip=spaceAssets.load(space->alertStyle->iconAnimations[variant]);const auto prepared=sources::prepareAlertFrame(engine,selected,*space->alertStyle,item,{},crtPortrait,visual::MotionClock::now(),clip);
		require(prepared->energyPattern==3 && prepared->animatedEmblem && prepared->profileImage==crtPortrait,"Space lost shared photo/animated icon");
		require(prepared->portrait->image.pixelColor(0,0).alpha()==0 && prepared->portrait->image.pixelColor(145,145).alpha()==255,"Space portrait isn't centred and circular");
		require(prepared->settings.nameFamily==selected.nameFamily && prepared->settings.labelFamily==selected.labelFamily && prepared->badge->image.width()==prepared->nameViewport+40,"Space lost independent fonts or automatic name plate");
		for(const auto &metric:prepared->heading->textMetrics)require(!metric.overflow && !metric.elided,"Space title doesn't fit armour panel");
		require(clip->image.copy(sources::alertIconCell(0))!=clip->image.copy(sources::alertIconCell(500)),"Space icon remained static");core.releaseSourcePreview(key);
	}
	require(spaceAssets.stats().loads==6 && spaceAssets.stats().bytes<=8*1024*1024 && storage->book==book && storage->writes==writes && core.pendingEventCount()==pending,"Space loads unrelated resources or changes TEST state");
	const auto *horror=registry.find("horror");require(horror && horror->alertStyle && horror->alertStyle->energyPattern==4 && horror->alertStyle->atlas.isEmpty(),"Horror fractured profile missing");
	visual::AssetManager horrorAssets(QDir(QFileInfo(OTO_PRESET_FILE).absolutePath()).filePath("assets"));
	for(int variant=0;variant<6;++variant){
		const auto key=core.createAlertPreview(types[variant],variant==3,false,{"123","DarkViewer"});const auto &item=core.sharedSnapshot()->channels.at({events::EventOrigin::Preview,key})->activity.front();
		sources::AlertSettings selected;selected.presetId="horror";selected.nameFamily="Arial";selected.labelFamily="Consolas";
		const auto clip=horrorAssets.load(horror->alertStyle->iconAnimations[variant]);const auto prepared=sources::prepareAlertFrame(engine,selected,*horror->alertStyle,item,{},crtPortrait,visual::MotionClock::now(),clip);
		require(prepared->energyPattern==4 && prepared->animatedEmblem && prepared->profileImage==crtPortrait,"Horror lost shared photo/animated icon");
		require(prepared->portrait->image.pixelColor(0,0).alpha()==0 && prepared->portrait->image.pixelColor(145,145).alpha()==255,"Horror portrait isn't centred and circular");
		require(prepared->settings.nameFamily==selected.nameFamily && prepared->settings.labelFamily==selected.labelFamily && prepared->badge->image.width()==prepared->nameViewport+40,"Horror lost fonts or automatic name plate");
		for(const auto &metric:prepared->heading->textMetrics)require(!metric.overflow && !metric.elided,"Horror title exceeds fractured plate");
		require(clip->image.copy(sources::alertIconCell(0))!=clip->image.copy(sources::alertIconCell(500)),"Horror icon remained static");core.releaseSourcePreview(key);
	}
	require(horrorAssets.stats().loads==6 && horrorAssets.stats().bytes<=8*1024*1024 && storage->book==book && storage->writes==writes && core.pendingEventCount()==pending,"Horror eagerly loads other resources or changes TEST state");
	const auto reserveKey=core.createAlertPreview(events::EventType::Follow,false,false,{"123","Viewer"});
	const auto &reserveItem=core.sharedSnapshot()->channels.at({events::EventOrigin::Preview,reserveKey})->activity.front();
	const auto reserve=sources::prepareAlertFrame(engine,{},*horror->alertStyle,reserveItem,{}, {},visual::MotionClock::now(),horrorAssets.load(horror->alertStyle->iconAnimations[0]));
	require(reserve->portrait->image.pixelColor(145,15).red()>reserve->portrait->image.pixelColor(145,15).blue(),"Horror fallback retained unrelated blue background");core.releaseSourcePreview(reserveKey);
	const auto *love=registry.find("love");require(love && love->alertStyle && love->alertStyle->energyPattern==5 && love->alertStyle->atlas.isEmpty(),"Love shared luminous profile missing");
	visual::AssetManager loveAssets(QDir(QFileInfo(OTO_PRESET_FILE).absolutePath()).filePath("assets"));
	for(int variant=0;variant<6;++variant){
		const auto key=core.createAlertPreview(types[variant],variant==3,false,{"123",variant==0?"DJ":"A_Long_Community_Name_1234567890123456789"});const auto &item=core.sharedSnapshot()->channels.at({events::EventOrigin::Preview,key})->activity.front();
		sources::AlertSettings selected;selected.presetId="love";selected.nameFamily="Arial";selected.labelFamily="Consolas";
		const auto clip=loveAssets.load(love->alertStyle->iconAnimations[variant]);const auto prepared=sources::prepareAlertFrame(engine,selected,*love->alertStyle,item,{},crtPortrait,visual::MotionClock::now(),clip);
		require(prepared->energyPattern==5 && prepared->animatedEmblem && prepared->profileImage==crtPortrait,"Love lost cached photo or active clip");
		require(prepared->portrait->image.pixelColor(0,0).alpha()==0 && prepared->portrait->image.pixelColor(145,145).alpha()==255,"Love photo must remain circular and centred");
		require(prepared->settings.nameFamily==selected.nameFamily && prepared->settings.labelFamily==selected.labelFamily,"Love lost independent FONT roles");
		require(prepared->badge->image.width()==prepared->nameViewport+40 && prepared->badge->image.width()<=480,"Love name plate lost automatic symmetric padding/bound");
		require(variant==0?prepared->scrollDistance==0:prepared->scrollDistance>0,"Love lost short-name centring or long-name ping-pong");
		for(const auto &metric:prepared->heading->textMetrics)require(!metric.overflow && !metric.elided,"Love title exceeds the glass panel");
		if(prepared->detail)for(const auto &metric:prepared->detail->textMetrics)require(!metric.overflow,"Love amount exceeds its text box");
		require(clip->image.copy(sources::alertIconCell(0))!=clip->image.copy(sources::alertIconCell(500)),"Love icon has no component animation");core.releaseSourcePreview(key);
	}
	require(loveAssets.stats().loads==6 && loveAssets.stats().bytes<=8*1024*1024 && storage->book==book && storage->writes==writes && core.pendingEventCount()==pending,"Love loads unrelated resources or changes TEST/daily state");
	const auto *graffiti=registry.find("graffiti");require(graffiti && graffiti->alertStyle && graffiti->alertStyle->energyPattern==6 && graffiti->alertStyle->atlas.isEmpty(),"Graffiti shared marker paint profile missing");
	visual::AssetManager graffitiAssets(QDir(QFileInfo(OTO_PRESET_FILE).absolutePath()).filePath("assets"));
	for(int variant=0;variant<6;++variant){
		const auto key=core.createAlertPreview(types[variant],variant==3,false,{"123",variant==0?"DJ":"A_Long_Community_Name_1234567890123456789"});const auto &item=core.sharedSnapshot()->channels.at({events::EventOrigin::Preview,key})->activity.front();
		sources::AlertSettings selected;selected.presetId="graffiti";selected.nameFamily="Arial";selected.labelFamily="Consolas";
		const auto clip=graffitiAssets.load(graffiti->alertStyle->iconAnimations[variant]);const auto prepared=sources::prepareAlertFrame(engine,selected,*graffiti->alertStyle,item,{},crtPortrait,visual::MotionClock::now(),clip);
		require(prepared->energyPattern==6 && prepared->animatedEmblem && prepared->profileImage==crtPortrait,"Graffiti lost cached photo or active clip");
		require(prepared->portrait->image.pixelColor(0,0).alpha()==0 && prepared->portrait->image.pixelColor(145,145).alpha()==255,"Graffiti photo must remain circular and centred");
		require(prepared->settings.nameFamily==selected.nameFamily && prepared->settings.labelFamily==selected.labelFamily,"Graffiti lost independent FONT roles");
		require(prepared->badge->image.width()==prepared->nameViewport+40 && prepared->badge->image.width()<=480,"Graffiti name plate lost automatic symmetric padding/bound");
		require(variant==0?prepared->scrollDistance==0:prepared->scrollDistance>0,"Graffiti lost short-name centring or long-name ping-pong");
		for(const auto &metric:prepared->heading->textMetrics)require(!metric.overflow && !metric.elided,"Graffiti title exceeds the paint label");
		if(prepared->detail)for(const auto &metric:prepared->detail->textMetrics)require(!metric.overflow,"Graffiti amount exceeds its text box");
		require(clip->image.copy(sources::alertIconCell(0))!=clip->image.copy(sources::alertIconCell(500)),"Graffiti icon has no component animation");core.releaseSourcePreview(key);
	}
	require(graffitiAssets.stats().loads==6 && graffitiAssets.stats().bytes<=8*1024*1024 && storage->book==book && storage->writes==writes && core.pendingEventCount()==pending,"Graffiti loads unrelated resources or changes TEST/daily state");
	const auto *minimal=registry.find("minimal");require(minimal && minimal->alertStyle && minimal->alertStyle->energyPattern==7 && minimal->alertStyle->atlas.isEmpty(),"Minimal shared silver line profile missing");
	visual::AssetManager minimalAssets(QDir(QFileInfo(OTO_PRESET_FILE).absolutePath()).filePath("assets"));
	for(int variant=0;variant<6;++variant){
		const auto key=core.createAlertPreview(types[variant],variant==3,false,{"123",variant==0?"DJ":"A_Long_Community_Name_1234567890123456789"});const auto &item=core.sharedSnapshot()->channels.at({events::EventOrigin::Preview,key})->activity.front();
		sources::AlertSettings selected;selected.presetId="minimal";selected.nameFamily="Arial";selected.labelFamily="Consolas";
		const auto clip=minimalAssets.load(minimal->alertStyle->iconAnimations[variant]);const auto prepared=sources::prepareAlertFrame(engine,selected,*minimal->alertStyle,item,{},crtPortrait,visual::MotionClock::now(),clip);
		require(prepared->energyPattern==7 && prepared->animatedEmblem && prepared->profileImage==crtPortrait,"Minimal lost cached photo or active clip");
		require(prepared->portrait->image.pixelColor(0,0).alpha()==0 && prepared->portrait->image.pixelColor(145,145).alpha()==255,"Minimal photo must remain circular and centred");
		require(prepared->settings.nameFamily==selected.nameFamily && prepared->settings.labelFamily==selected.labelFamily,"Minimal lost independent FONT roles");
		require(prepared->badge->image.width()==prepared->nameViewport+40 && prepared->badge->image.width()<=480,"Minimal name plate lost automatic symmetric padding/bound");
		require(variant==0?prepared->scrollDistance==0:prepared->scrollDistance>0,"Minimal lost short-name centring or long-name ping-pong");
		for(const auto &metric:prepared->heading->textMetrics)require(!metric.overflow && !metric.elided,"Minimal title exceeds the minimal layout");
		if(prepared->detail)for(const auto &metric:prepared->detail->textMetrics)require(!metric.overflow,"Minimal amount exceeds its text box");
		require(clip->image.copy(sources::alertIconCell(0))!=clip->image.copy(sources::alertIconCell(500)),"Minimal icon has no component animation");core.releaseSourcePreview(key);
	}
	require(minimalAssets.stats().loads==6 && minimalAssets.stats().bytes<=8*1024*1024 && storage->book==book && storage->writes==writes && core.pendingEventCount()==pending,"Minimal loads unrelated resources or changes TEST/daily state");
	const auto *urban=registry.find("urban-city");require(urban && urban->alertStyle && urban->alertStyle->energyPattern==8 && urban->alertStyle->atlas.isEmpty(),"Urban City shared bronze machinery profile missing");
	visual::AssetManager urbanAssets(QDir(QFileInfo(OTO_PRESET_FILE).absolutePath()).filePath("assets"));
	for(int variant=0;variant<6;++variant){
		const auto key=core.createAlertPreview(types[variant],variant==3,false,{"123",variant==0?"DJ":"A_Long_Community_Name_1234567890123456789"});const auto &item=core.sharedSnapshot()->channels.at({events::EventOrigin::Preview,key})->activity.front();
		sources::AlertSettings selected;selected.presetId="urban-city";selected.nameFamily="Arial";selected.labelFamily="Consolas";
		const auto clip=urbanAssets.load(urban->alertStyle->iconAnimations[variant]);const auto prepared=sources::prepareAlertFrame(engine,selected,*urban->alertStyle,item,{},crtPortrait,visual::MotionClock::now(),clip);
		require(prepared->energyPattern==8 && prepared->animatedEmblem && prepared->profileImage==crtPortrait,"Urban City lost cached photo or active clip");
		require(prepared->portrait->image.pixelColor(0,0).alpha()==0 && prepared->portrait->image.pixelColor(145,145).alpha()==255,"Urban City photo must remain circular and centred");
		require(prepared->settings.nameFamily==selected.nameFamily && prepared->settings.labelFamily==selected.labelFamily,"Urban City lost independent FONT roles");
		require(prepared->badge->image.width()==prepared->nameViewport+40 && prepared->badge->image.width()<=480,"Urban City name plate lost automatic symmetric padding/bound");
		require(variant==0?prepared->scrollDistance==0:prepared->scrollDistance>0,"Urban City lost short-name centring or long-name ping-pong");
		for(const auto &metric:prepared->heading->textMetrics)require(!metric.overflow && !metric.elided,"Urban City title exceeds the industrial layout");
		if(prepared->detail)for(const auto &metric:prepared->detail->textMetrics)require(!metric.overflow,"Urban City amount exceeds its text box");
		require(clip->image.copy(sources::alertIconCell(0))!=clip->image.copy(sources::alertIconCell(500)),"Urban City icon has no component animation");core.releaseSourcePreview(key);
	}
	require(urbanAssets.stats().loads==6 && urbanAssets.stats().bytes<=8*1024*1024 && storage->book==book && storage->writes==writes && core.pendingEventCount()==pending,"Urban City loads unrelated resources or changes TEST/daily state");
	const auto *hologram=registry.find("futuristic-hologram");require(hologram && hologram->alertStyle && hologram->alertStyle->energyPattern==9 && hologram->alertStyle->atlas.isEmpty(),"Futuristic Hologram shared holographic projection profile missing");
	visual::AssetManager hologramAssets(QDir(QFileInfo(OTO_PRESET_FILE).absolutePath()).filePath("assets"));
	for(int variant=0;variant<6;++variant){
		const auto key=core.createAlertPreview(types[variant],variant==3,false,{"123",variant==0?"DJ":"A_Long_Community_Name_1234567890123456789"});const auto &item=core.sharedSnapshot()->channels.at({events::EventOrigin::Preview,key})->activity.front();
		sources::AlertSettings selected;selected.presetId="futuristic-hologram";selected.nameFamily="Arial";selected.labelFamily="Consolas";
		const auto clip=hologramAssets.load(hologram->alertStyle->iconAnimations[variant]);const auto prepared=sources::prepareAlertFrame(engine,selected,*hologram->alertStyle,item,{},crtPortrait,visual::MotionClock::now(),clip);
		require(prepared->energyPattern==9 && prepared->animatedEmblem && prepared->profileImage==crtPortrait,"Futuristic Hologram lost cached photo or active clip");
		require(prepared->portrait->image.pixelColor(0,0).alpha()==0 && prepared->portrait->image.pixelColor(145,145).alpha()==255,"Futuristic Hologram photo must remain circular and centred");
		require(prepared->settings.nameFamily==selected.nameFamily && prepared->settings.labelFamily==selected.labelFamily,"Futuristic Hologram lost independent FONT roles");
		require(prepared->badge->image.width()==prepared->nameViewport+40 && prepared->badge->image.width()<=480,"Futuristic Hologram name plate lost automatic symmetric padding/bound");
		require(variant==0?prepared->scrollDistance==0:prepared->scrollDistance>0,"Futuristic Hologram lost short-name centring or long-name ping-pong");
		for(const auto &metric:prepared->heading->textMetrics)require(!metric.overflow && !metric.elided,"Futuristic Hologram title exceeds the holographic layout");
		if(prepared->detail)for(const auto &metric:prepared->detail->textMetrics)require(!metric.overflow,"Futuristic Hologram amount exceeds its text box");
		require(clip->image.copy(sources::alertIconCell(0))!=clip->image.copy(sources::alertIconCell(500)),"Futuristic Hologram icon has no component animation");core.releaseSourcePreview(key);
	}
	require(hologramAssets.stats().loads==6 && hologramAssets.stats().bytes<=8*1024*1024 && storage->book==book && storage->writes==writes && core.pendingEventCount()==pending,"Futuristic Hologram loads unrelated resources or changes TEST/daily state");
	int previousWidth=0;
	for(const auto &userName:{std::string("DJ"),std::string("MediumViewer"),std::string("A_Much_Longer_Community_Name_123456789")}){
		const auto key=core.createAlertPreview(events::EventType::Follow,false,false,{"123",userName});const auto &item=core.sharedSnapshot()->channels.at({events::EventOrigin::Preview,key})->activity.front();
		sources::AlertSettings layout;const auto sized=sources::prepareAlertFrame(engine,layout,*style->alertStyle,item,atlas,{},visual::MotionClock::now());
		const int width=sized->badge->image.width();require(width>=192 && width<=480 && width>previousWidth,"Name plate doesn't follow measured name width/bounds");previousWidth=width;
		require(sized->nameViewport+40==width,"Name plate padding isn't symmetric");
		if(sized->scrollDistance==0){
			require(sized->name->image.width()==sized->nameViewport,"Short name isn't sized to its centred viewport");
			int left=sized->nameViewport,right=-1;
			for(int y=0;y<sized->name->image.height();++y)for(int x=0;x<sized->nameViewport;++x)if(sized->name->image.pixelColor(x,y).alpha()>64){left=std::min(left,x);right=std::max(right,x);}
			require(right>=left && std::abs((left+right)*.5-(sized->nameViewport-1)*.5)<=3,"Rendered name glyphs aren't centred inside automatic plate");
		}
		core.releaseSourcePreview(key);
	}
	const auto preview=core.createAlertPreview(events::EventType::Follow,false,true);const auto &entry=core.sharedSnapshot()->channels.at({events::EventOrigin::Preview,preview})->activity.front();sources::AlertSettings settings;const auto longFrame=sources::prepareAlertFrame(engine,settings,*style->alertStyle,entry,atlas,{},visual::MotionClock::now());require(longFrame->scrollDistance>0 && longFrame->name->image.width()<=2048,"Long Unicode name lost bounded ping-pong");core.releaseSourcePreview(preview);
	for(const auto times:{sources::AlertTimes{},sources::AlertTimes{100,0,100},sources::AlertTimes{10000,60000,10000}}){require(!sources::sampleAlert(0,times).finished && sources::sampleAlert(0,times).portrait==0,"Entrance must begin transparent");const auto visible=sources::sampleAlert(times.entry*.9,times);require(visible.title>.1f && visible.portrait>.9f,"Staggered reveal absent");const auto end=sources::sampleAlert(times.total(),times);require(end.finished && end.portrait==0 && end.burst==0 && end.name==0,"Exact end is not fully transparent");}
	require(sources::sampleAlert(std::numeric_limits<double>::quiet_NaN(),{}).finished,"Invalid timeline accepted");
	for(const auto times:{sources::AlertTimes{},sources::AlertTimes{100,0,100},sources::AlertTimes{1500,3000,10000}}){
		for(double progress:{.25,.50,.75,.95}){const auto entry=sources::sampleAlert(times.entry*(1-progress),times),exit=sources::sampleAlert(times.entry+times.hold+times.exit*progress,times);
			for(const auto pair:{std::pair{entry.portrait,exit.portrait},std::pair{entry.burst,exit.burst},std::pair{entry.title,exit.title},std::pair{entry.name,exit.name},std::pair{entry.effects,exit.effects},std::pair{entry.scale,exit.scale},std::pair{entry.offsetX,exit.offsetX}})require(std::abs(pair.first-pair.second)<.0001f,"Exit no longer mirrors entrance with independent duration");
		}
		const auto before=sources::sampleAlert(times.entry+times.hold-.001,times),after=sources::sampleAlert(times.entry+times.hold,times);
		require(std::abs(before.visualMs-after.visualMs)<.002 && std::abs(before.burst-after.burst)<.001f && std::abs(before.effects-after.effects)<.001f && std::abs(before.scale-after.scale)<.001f,"Hold/exit discontinuity");
		require(sources::sampleAlert(times.entry+times.hold+times.exit*.5,times).visualMs<after.visualMs,"Exit clock did not reverse from current pose");
	}
	require(sources::sampleAlert(6000-.000001,{}).effects>=0,"Exit sine roundoff exceeded GPU range");
	for(double ms=0;ms<6000;ms+=1000.0/120){const auto sample=sources::sampleAlert(ms,{});for(float value:{sample.portrait,sample.burst,sample.title,sample.name,sample.effects,sample.exitProgress})require(std::isfinite(value) && value>=0 && value<=1,"Continuous alert timeline exceeds GPU parameter range");}
	QImage pic(32,32,QImage::Format_RGBA8888_Premultiplied);pic.fill(QColor(240,100,50));QByteArray bytes;QBuffer buffer(&bytes);buffer.open(QIODevice::WriteOnly);pic.save(&buffer,"PNG");const auto image=assets.ingestProfileImage("123",bytes);require(assets.profileImage("123")==image && assets.stats().bytes<=8*1024*1024,"Profile cache not shared/bounded");
	bool rejected=false;try{assets.ingestProfileImage("123",QByteArray("not an image"));}catch(...){rejected=true;}require(rejected,"Malformed profile decoded");
	visual::Scene face{{100,100},{visual::ImageNode{QRect(0,0,100,100),image,{},0,{},100,true}}};const auto round=engine.prepare(face);require(round->image.pixelColor(0,0).alpha()==0 && round->image.pixelColor(50,50).alpha()==255,"Circular portrait doesn't preserve transparency");
	std::cout<<"PASS: shared delivery cursors, same-batch/late events, no replay, bounded activity, six variants, isolated TEST, phase timings, Unicode ping-pong, portrait cache/circle\n";return 0;
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}}
