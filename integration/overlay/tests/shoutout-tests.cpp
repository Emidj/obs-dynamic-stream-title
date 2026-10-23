// SPDX-License-Identifier: GPL-2.0-or-later
#include "sources/shoutout-model.hpp"
#include "core/plugin-core.hpp"
#include "twitch/protocol.hpp"
#include "visual/asset-manager.hpp"
#include <QGuiApplication>
#include <QJsonArray>
#include <QDir>
#include <QPainter>
#include <QFileInfo>
#include <iostream>
#include <stdexcept>
using namespace oto;
namespace {
void require(bool ok,const char *why){if(!ok)throw std::runtime_error(why);}
struct Config:IConfigStore{ConfigLoadResult load()override{return {ConfigStatus::Loaded,{false}};}bool save(Settings)override{return true;}};
struct Log:ILogger{void write(LogLevel,std::string_view)const noexcept override{}};
struct Daily:IDailyStore{state::DailyBook book;int writes=0;state::DailyBook load()override{return book;}bool save(const state::DailyBook &next)override{book=next;++writes;return true;}};
QJsonObject message(QString actor,QJsonArray badges={},QString source={}){
	QJsonObject event{{"broadcaster_user_id","123"},{"chatter_user_id",actor},{"chatter_user_name","Actor"},{"badges",badges},{"message",QJsonObject{{"text","!so @DJEMIMAS"}}}};if(!source.isEmpty())event["source_broadcaster_user_id"]=source;
	return {{"metadata",QJsonObject{{"message_type","notification"},{"message_id","chat-123"},{"message_timestamp","2026-10-04T10:00:00Z"}}},{"payload",QJsonObject{{"subscription",QJsonObject{{"type","channel.chat.message"},{"version","1"}}},{"event",event}}}};
}
}
int main(int argc,char **argv){QGuiApplication app(argc,argv);try{
	const auto target=[](QJsonObject msg){auto event=twitch::notification(msg,"123");return event?twitch::shoutoutTarget(std::get<events::ChatEvent>(event->payload)):QString{};};
	require(target(message("123"))=="djemimas","Broadcaster !so @target rejected");
	for(const auto badge:{"moderator","lead_moderator"})require(target(message("456",{QJsonObject{{"set_id",badge}}}))=="djemimas","Moderator denied");
	require(target(message("456")).isEmpty() && target(message("456",{QJsonObject{{"set_id","vip"}}})).isEmpty(),"Viewer/VIP gained command permission");
	require(target(message("456",{QJsonObject{{"set_id","moderator"}}},"999")).isEmpty(),"Shared chat moderator from another channel accepted");
	for(const auto invalid:{"!so djemimas","!so @@djemimas","!social @djemimas","hello !so @djemimas","!so @djemimas extra","!so @https://bad","!so @","!so @this_login_is_longer_than_25_chars"})require(twitch::shoutoutTarget({{"123","Owner"},invalid,true}).isEmpty(),"Malformed command accepted");
	require(twitch::shoutoutTarget({{"123","Owner"}," !SO\t@DJEMIMAS ",true})=="djemimas","Mention parsing/case normalization wrong");
	require(twitch::shoutoutMessage("djemimas").contains("https://www.twitch.tv/djemimas") && twitch::shoutoutMessage("djemimas").contains("@djemimas") && twitch::shoutoutMessage("bad\nname").isEmpty(),"Chat link validation failed");
	auto disk=std::make_unique<Daily>();auto *storage=disk.get();PluginCore core(std::make_unique<Config>(),std::make_unique<Log>(),std::move(disk));core.start();
	const state::ChannelKey channel{events::EventOrigin::Simulation,"sim-channel"};core.simulateEvent(events::EventType::Follow);core.processPendingEvents();const auto book=storage->book;const auto writes=storage->writes;
	sources::ShoutoutCursor a,b;a.synchronize(core.sharedSnapshot(),channel);b.synchronize(core.sharedSnapshot(),channel);
	core.simulateEvent(events::EventType::Shoutout);core.simulateEvent(events::EventType::Shoutout);core.processPendingEvents();const auto first=a.next(core.sharedSnapshot());const auto second=a.next(core.sharedSnapshot());require(first && second && first->delivery<second->delivery && !a.next(core.sharedSnapshot()),"Same-batch shoutouts lost/replayed");require(b.next(core.sharedSnapshot())->event==first->event,"Instances don't share events");
	require(storage->book==book && storage->writes==writes && core.sharedSnapshot()->channels.at(channel)->activity.size()==1,"Shoutout changed daily/history");
	a.reset();a.synchronize(core.sharedSnapshot(),channel);require(!a.next(core.sharedSnapshot()),"Show/restart replayed old card");
	for(int n=0;n<20;++n)core.simulateEvent(events::EventType::Shoutout);core.processPendingEvents();require(core.sharedSnapshot()->channels.at(channel)->shoutouts.size()==16,"Transient history unbounded");
	visual::VisualEngine visual;for(bool longText:{false,true}){const auto key=core.createShoutoutPreview(longText,{"456","DemoViewer"},"demoviewer");const auto &entry=core.sharedSnapshot()->channels.at({events::EventOrigin::Preview,key})->shoutouts.back();sources::ShoutoutSettings settings;settings.nameFamily="Consolas";settings.labelFamily="Arial";const auto frame=sources::prepareShoutoutFrame(visual,settings,{},entry,{},visual::MotionClock::now());require(frame->base->image.size()==QSize(800,800) && frame->name && frame->event.event==entry.event,"Card preparation failed");require(longText?frame->scrollDistance>0:frame->scrollDistance==0,"Name scrolling contract failed");core.releaseSourcePreview(key);}
	require(storage->book==book && storage->writes==writes,"TEST wrote daily totals");
	for(const auto times:{sources::AlertTimes{700,6000,700},sources::AlertTimes{100,0,100},sources::AlertTimes{10000,60000,10000}}){require(sources::sampleShoutout(0,times).portrait==0 && sources::sampleShoutout(times.total(),times).finished,"Timeline endpoints wrong");const auto hold=sources::sampleShoutout(times.entry,times);require(hold.opacity==1 && hold.portrait==1 && hold.data==1 && hold.dataScale==1,"Hold missing");for(int n=1;n<=10;++n){const auto entry=sources::sampleShoutout(times.entry*n/10.0,times);const auto exit=sources::sampleShoutout(times.total()-times.exit*n/10.0,times);require(std::abs(entry.portrait-exit.portrait)<.0001f && std::abs(entry.dataScale-exit.dataScale)<.0001f,"Exit not reverse entrance");}}
	const auto registry=presets::Registry::fromFile(OTO_PRESET_FILE);visual::AssetManager assets(QFileInfo(OTO_PRESET_FILE).absolutePath()+"/assets");
	const QString output=argc>1?QString::fromLocal8Bit(argv[1]):QString{};if(!output.isEmpty())QDir().mkpath(output);
	for(const auto &preset:registry.entries()){
		require(bool(preset.shoutoutStyle),"Factory card style missing");const auto &style=*preset.shoutoutStyle;require(style.assetBox.contains(style.nameBox) && !style.nameBox.intersects(style.categoryBox) && !style.nameBox.intersects(style.followersBox),"Name row collides with details");
		sources::ShoutoutSettings settings;settings.preset=preset.id;
		sources::ShoutoutFramePtr shortFrame;
		int largest=0;for(const auto &name:{QString("DJ"),QString("DJ_PEPITO"),QString("A_longer_channel_display_name"),QString("THE_LONG_CHANNEL_DISPLAY_NAME_FOR_PING_PONG_0123456789")}){
			const auto key=core.createShoutoutPreview(false,{"456",name.toStdString()},"viewer","Just Chatting",125000);const auto entry=core.sharedSnapshot()->channels.at({events::EventOrigin::Preview,key})->shoutouts.back();const auto frame=sources::prepareShoutoutFrame(visual,settings,style,entry,{},visual::MotionClock::now(),assets.load(style.atlas));
			require(frame->namePixelSize>=8 && frame->namePixelSize<=style.nameBox.height()-4,"Adaptive font exceeds name height");if(name=="DJ"){largest=frame->namePixelSize;require(frame->scrollDistance==0,"Short name scrolls unnecessarily");}else require(frame->namePixelSize<=largest,"Longer name got larger");
			if(name=="DJ_PEPITO")shortFrame=frame;
			if(name.startsWith("THE_LONG")){require(frame->scrollDistance>0,"Very long name doesn't scroll at minimum size");const auto end=visual::samplePingPong(100000,frame->scrollDistance);require(end>=0 && end<=frame->scrollDistance,"Ping-pong escaped viewport");}
			core.releaseSourcePreview(key);
		}
		if(!output.isEmpty()) {QImage image=shortFrame->base->image;QPainter painter(&image);painter.drawImage(style.portraitBox.topLeft(),shortFrame->portrait->image);painter.drawImage(QPoint{},shortFrame->base->image);painter.drawImage(QPoint{},shortFrame->details->image);painter.setClipRect(style.nameBox);painter.drawImage(style.nameBox.topLeft(),shortFrame->name->image);painter.end();require(image.save(output+"/"+preset.id+".png"),"Can't save composition preview");}
	}
	core.stop();core.start();require(core.sharedSnapshot()->channels.at(channel)->shoutouts.empty(),"Shoutout persisted/replayed");
	std::cout<<"Shoutout permissions, mandatory mention, shared queue/cursors, isolated preview and reverse timeline passed\n";return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
