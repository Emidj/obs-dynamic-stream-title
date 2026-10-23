// SPDX-License-Identifier: GPL-2.0-or-later
#include "sources/chat-source.hpp"
#include "twitch/protocol.hpp"
#include "chat-fixtures.hpp"
#include "chat-badge-checks.hpp"
#include "chat-moderation-checks.hpp"
#include "chat-layer-checks.hpp"
#include "chat-catalog-checks.hpp"
#include "chat-effect-checks.hpp"
#include "chat-bttv-checks.hpp"
#include "chat-mood-checks.hpp"
#include "chat-wide-checks.hpp"
#include "chat-ffz-cursed-checks.hpp"
#include "chat-ffz-full-checks.hpp"
#include "chat-accessory-checks.hpp"
#include "chat-personal-checks.hpp"
#include "chat-cheer-checks.hpp"
#include <QApplication>
#include <QBuffer>
#include <QFileInfo>
#include <QDir>
#include <QPainter>
#include <obs-module.h>
#include <iostream>
using namespace oto;
const char *obs_module_text(const char *key){return key;}
namespace {
void require(bool value,const char *message){if(!value)throw std::runtime_error(message);}
struct Config:IConfigStore{ConfigLoadResult load()override{return {ConfigStatus::Loaded,{false}};}bool save(Settings)override{return true;}};
struct Log:ILogger{void write(LogLevel,std::string_view)const noexcept override{}};
void settle(){for(int i=0;i<10;++i)QCoreApplication::processEvents();}
}
int main(int argc,char **argv){QApplication app(argc,argv);try{
	require(obs_startup("en-US",nullptr,nullptr),"OBS startup");
	auto core=std::make_shared<PluginCore>(std::make_unique<Config>(),std::make_unique<Log>());core->start();
	const auto now=std::chrono::system_clock::now();const state::ChannelKey key{events::EventOrigin::Twitch,"123"};
	const auto send=[&](int id,events::ChatEvent chat,int seconds=0){auto r=core->processEvent({"chat-test-"+std::to_string(id),key.channelId,now+std::chrono::seconds(seconds),key.origin,std::move(chat)});require(bool(r),"Chat admission");core->processPendingEventsFor(key.origin,key.channelId);};
	for(int i=0;i<80;++i)send(i,{{"user","Álex_東京"},i%2?"!so @someone":"Hello, world",false},i);
	const auto channel=core->sharedSnapshot()->channels.at(key);require(channel->chat.size()==64 && channel->activity.empty() && channel->daily.bits==0 && channel->daily.subs==0,"Unbounded chat or altered activity/counters");
	sources::ChatSettings s;s.messages=12;const auto view=sources::chatView(core->sharedSnapshot(),key,s);require(view.messages.size()==12,"View capacity");
	s.showCommands=false;require(sources::chatView(core->sharedSnapshot(),key,s).messages.size()==12,"Optional command filter");for(const auto &v:sources::chatView(core->sharedSnapshot(),key,s).messages)require(std::get<events::ChatEvent>(v.event->data().payload).message.front()!='!',"Command filter wrong");s.showCommands=true;
	require(sources::chatView(core->sharedSnapshot(),key,s,channel->delivered[size_t(events::EventType::Chat)]).messages.empty(),"Clear watermark");
	visual::VisualEngine engine;const auto mono=visual::MotionClock::now();auto frame=sources::prepareChatFrame(engine,s,view,{},mono);
	{
	 auto alignedCore=std::make_shared<PluginCore>(std::make_unique<Config>(),std::make_unique<Log>());alignedCore->start();
	 int index=0;for(const auto &name:{std::string("A"),std::string("marinaDJ"),std::string("very_long_streamer_username"),std::string("Álex_東京")}){
	  events::ChatEvent chat{{"align-user-"+std::to_string(index),name},"Left aligned message with all words preserved",false};
	  for(int n=0;n<index;++n)chat.badges.push_back({"subscriber",std::to_string(n),{}});
	  require(bool(alignedCore->processEvent({"align-"+std::to_string(index++),"alignment",now,events::EventOrigin::Twitch,std::move(chat)})),"Alignment event admission");
	 }
	 alignedCore->processPendingEvents();const auto definitions=presets::Registry::fromFile(OTO_PRESET_FILE);
	 for(const auto &definition:definitions.entries())for(const int width:{320,600,1920})for(const int nameSize:{12,48}){
	  sources::ChatSettings aligned;aligned.style=*definition.chatStyle;aligned.showFrame=false;aligned.width=width;aligned.height=1080;aligned.nameSize=nameSize;
	  const auto v=sources::chatView(alignedCore->sharedSnapshot(),{events::EventOrigin::Twitch,"alignment"},aligned);
	  const auto result=sources::prepareChatFrame(engine,aligned,v,{},mono);
	  require(result->rows.size()==4,"Alignment rows missing");const auto left=result->rows.front().bodyBox.left();
	  for(const auto &row:result->rows){require(row.bodyBox.left()==left,"Username or badges shift message column");require(row.bodyBox.right()<=row.graphic->image.width(),"Aligned body outside cell");for(const auto &badge:row.badges)require(badge.box.right()<left,"Badges overlap aligned message");}
	 }
	 alignedCore->stop();
	}
	const auto renders=engine.stats().renders;require(sources::prepareChatFrame(engine,s,view,frame,mono)==frame && engine.stats().renders==renders,"Idle reraster");
	for(const auto &row:frame->rows)require(QRectF(0,0,s.width,s.height).contains(QRectF(row.position,row.graphic->image.size())),"Vertical bounds");
	s.layout=sources::ChatLayout::Horizontal;s.width=1920;s.height=100;auto horizontal=sources::prepareChatFrame(engine,s,view,{},mono);require(horizontal->rows.size()>=3,"Horizontal did not pack multiple messages");
	for(const auto &row:horizontal->rows)require(row.position.y()>=0 && row.position.y()+row.graphic->image.height()<=s.height,"Horizontal bounds");
	s.layout=sources::ChatLayout::Vertical;s.width=320;s.height=700;
	const auto preview=core->createChatPreview(true);auto longView=sources::chatView(core->sharedSnapshot(),{events::EventOrigin::Preview,preview},s);auto longFrame=sources::prepareChatFrame(engine,s,longView,{},mono,true);require(longFrame->rows.back().lines>2 && longFrame->rows.back().bodySize.height()*longFrame->rows.back().bodyScale<=longFrame->rows.back().bodyBox.height(),"Long text incomplete");
	// Keep room for both complete messages in the movement scenario. At 320px
	// the long message can occupy the entire canvas when multiple badges reserve
	// their band, legitimately removing it once a newer message arrives.
	s.width=600;longFrame=sources::prepareChatFrame(engine,s,longView,{},mono,true);
	const auto old=longFrame;core->appendChatPreview(preview);longView=sources::chatView(core->sharedSnapshot(),{events::EventOrigin::Preview,preview},s);longFrame=sources::prepareChatFrame(engine,s,longView,old,mono+std::chrono::milliseconds(500));
	bool moved=false;for(const auto &row:longFrame->rows)for(const auto &before:old->rows)if(row.event==before.event){require(row.graphic==before.graphic,"Old message reraster");moved=moved || row.position!=before.position;require(row.entered==before.entered,"Old entry restarted");}require(moved,"Previous messages did not move");
	auto visual=std::make_shared<visual::VisualEngine>();auto media=std::make_shared<chat::EmoteCache>(visual);const QString url="https://static-cdn.jtvnw.net/emoticons/v2/fixture/animated/dark/2.0";
	checkChatBadges(visual);
	testing::checkChatLayers(visual);testing::checkCatalogRefresh(visual);testing::checkChatEffects(visual);testing::checkBTTVEffects(visual);testing::checkChatMood(visual);testing::checkChatWide(visual);
	testing::checkFFZCursed(visual);testing::checkFullFFZ(visual);testing::checkChatAccessories();testing::checkPersonalCatalog(visual);testing::checkCheermotes(visual);
	checkChatModeration(std::make_shared<PluginCore>(std::make_unique<Config>(),std::make_unique<Log>()),*visual);
	require(media->ingest(url,animatedChatFixture()),"GIF decode");const auto emote=media->get(url);require(emote && emote->ends.size()==2 && emote->frame(0)!=emote->frame(100) && emote->frame(200)==emote->frame(0),"Animated GIF timing/loop");
	require(!chat::publicEmoteUrl("file:///etc/passwd") && !chat::publicEmoteUrl("https://cdn.7tv.app.evil/emote/a/2x.webp") && !media->ingest(url,QByteArray("bad")),"Unsafe/invalid asset accepted");
	events::ChatEvent rich{{"user","MarinaDJ"},"Before Animated after"};rich.fragments={{"Before ",{},false},{"Animated","fixture",true},{" after",{},false}};send(81,rich,81);
	s.width=600;s.messages=8;auto richFrame=sources::prepareChatFrame(*visual,s,sources::chatView(core->sharedSnapshot(),key,s),{},mono,false,media.get());require(richFrame->rows.back().emotes.size()==1 && richFrame->rows.back().emotes.front().media==emote,"Twitch fragment not inline/shared");
	require(QRectF(QPointF{},richFrame->rows.back().bodySize).contains(richFrame->rows.back().emotes.front().box),"Emote leaves complete body");
	// More than 48 emotes and maxLines=1 previously lost almost the whole message.
	events::ChatEvent many{{"user","MarinaDJ"},""};for(int i=0;i<60;++i){if(i){many.message+=' ';many.fragments.push_back({" ",{},false});}many.message+="Dance";many.fragments.push_back({"Dance","fixture",true});}send(82,many,82);
	for(const bool h:{false,true}){
		auto limits=s;limits.layout=h?sources::ChatLayout::Horizontal:sources::ChatLayout::Vertical;limits.width=h?1920:600;limits.height=h?100:700;limits.messages=1;
		const auto complete=sources::prepareChatFrame(*visual,limits,sources::chatView(core->sharedSnapshot(),key,limits),{},mono,false,media.get());const auto &row=complete->rows.back();
		require(row.emotes.size()==60,"Emotes discarded outside viewport or at old 48 limit");
		for(const auto &emote:row.emotes)require(QRectF(QPointF{},row.bodyBox.size()).contains(QRectF(emote.box.topLeft()*row.bodyScale,emote.box.size()*row.bodyScale)),"Emote not completely visible at once");
		const auto end=mono+std::chrono::seconds(10);
		const auto count=visual->stats().renders;require(sources::prepareChatFrame(*visual,limits,complete->view,complete,end,false,media.get())==complete && visual->stats().renders==count,"Overflow reraster on idle");
		if(h){send(83,{{"user","Next"},"Hello",false},83);const auto next=sources::prepareChatFrame(*visual,limits,sources::chatView(core->sharedSnapshot(),key,limits),complete,mono+std::chrono::seconds(1),false,media.get());require(next->rows.size()==2 && next->rows.front().retiring,"History removal drops outgoing card");const auto &leaving=next->rows.front();require(sources::sampleChatRow(leaving,limits.entryMs,limits.layout,mono+std::chrono::seconds(1)).position==row.position,"Outgoing jumps at start");require(sources::sampleChatRow(leaving,limits.entryMs,limits.layout,mono+std::chrono::milliseconds(1350)).position.x()+leaving.graphic->image.width()<0,"Outgoing never exits");
			const auto refreshed=sources::prepareChatFrame(*visual,limits,next->view,next,mono+std::chrono::milliseconds(1100),false,nullptr);require(refreshed->rows.front().moved==leaving.moved,"Media refresh restarts departure");
			const auto empty=sources::prepareChatFrame(*visual,limits,{key,next->view.generation,{}},next,mono);require(empty->rows.empty(),"Clear leaves retiring messages");}
	}
	sources::ChatSettings tiny;tiny.layout=sources::ChatLayout::Horizontal;tiny.width=320;tiny.height=100;tiny.messages=1;tiny.messageSize=128;send(84,{{"user","TinyCanvas"},std::string(4096,'W')},84);
	const auto fitted=sources::prepareChatFrame(*visual,tiny,sources::chatView(core->sharedSnapshot(),key,tiny),{},mono);const auto &fittedRow=fitted->rows.back();require(fittedRow.bodyPixelSize==8 && fittedRow.bodyScale<1 && fittedRow.bodySize.height()*fittedRow.bodyScale<=fittedRow.bodyBox.height(),"Extreme text not fitted to canvas");
	const auto bttv=chat::parseCatalog(1,R"({"channelEmotes":[{"id":"abc123","code":"Dance","animated":true}],"sharedEmotes":[]})",true);require(bttv.size()==1 && bttv[0].url=="https://cdn.betterttv.net/emote/abc123/2x","BTTV catalog");
	const auto seven=chat::parseCatalog(0,R"({"emote_set":{"emotes":[{"name":"Dance","data":{"host":{"url":"//cdn.7tv.app/emote/abc","files":[{"name":"2x.webp"}]}}}]}})",true);require(seven.size()==1,"7TV catalog");
	const auto ffz=chat::parseCatalog(2,R"({"default_sets":[1],"sets":{"1":{"emoticons":[{"name":"Dance","urls":{"2":"//cdn.frankerfacez.com/emote/12/2"},"animated":{"2":"//cdn.frankerfacez.com/emote/12/animated/2"}}]},"2":{"emoticons":[{"name":"Hidden","urls":{"2":"//cdn.frankerfacez.com/emote/13/2"}}]}}})",false);require(ffz.size()==1 && ffz[0].url.contains("animated"),"FFZ default sets/animated");
	std::vector<std::function<void(twitch::HttpReply)>> downloads;chat::EmoteCache networkCache(visual,[&](QString,std::function<void(twitch::HttpReply)> done){downloads.push_back(std::move(done));});
	networkCache.get(url);networkCache.get(url);require(downloads.size()==1,"Clone downloaded same emote twice");downloads[0]({200,animatedChatFixture(),false});require(networkCache.get(url) && networkCache.revision()==1,"Download not published");
	const auto pngUrl=QString("https://cdn.betterttv.net/emote/missing/2x");networkCache.get(pngUrl);downloads[1]({404,{},false});networkCache.get(pngUrl);require(downloads.size()==2,"Failure retries continuously");networkCache.stop();
	auto largeSettings=s;largeSettings.messageSize=36;const auto large=sources::prepareChatFrame(*visual,largeSettings,sources::chatView(core->sharedSnapshot(),key,largeSettings),{},mono);require(large->rows.back().graphic->image.height()>richFrame->rows.back().graphic->image.height(),"Message font size ignored");
	auto transparent=s;transparent.backgroundOpacity=0;const auto noFill=sources::prepareChatFrame(*visual,transparent,sources::chatView(core->sharedSnapshot(),key,transparent),{},mono);const auto &image=noFill->rows.back().graphic->image;require(image.pixelColor(200,image.height()-5).alpha()==0,"Background opacity affects fill incorrectly");
	QJsonObject envelope{{"metadata",QJsonObject{{"message_type","notification"},{"message_id","chat-protocol"},{"message_timestamp","2026-10-04T10:00:00Z"}}},{"payload",QJsonObject{{"subscription",QJsonObject{{"type","channel.chat.message"},{"version","1"}}},{"event",QJsonObject{{"broadcaster_user_id","123"},{"chatter_user_id","456"},{"chatter_user_name","Mod"},{"color","#39E67A"},{"badges",QJsonArray{QJsonObject{{"set_id","moderator"}}}},{"message",QJsonObject{{"text","Dance"},{"fragments",QJsonArray{QJsonObject{{"type","emote"},{"text","Dance"},{"emote",QJsonObject{{"id","abc"},{"format",QJsonArray{"static","animated"}}}}}}}}}}}}}};
	const auto mapped=twitch::notification(envelope,"123");require(bool(mapped),"Chat protocol");const auto &parsed=std::get<events::ChatEvent>(mapped->payload);require(parsed.role==events::ChatRole::Moderator && parsed.canShoutout && parsed.color=="#39E67A" && parsed.fragments.size()==1 && parsed.fragments[0].animated,"Roles/color/animated fragments lost");
	auto renderer=std::make_shared<ObsVisualRenderer>();auto hub=std::make_shared<sources::ChatHub>(core,visual,renderer,media);auto a=std::make_shared<sources::ChatInstance>(),b=std::make_shared<sources::ChatInstance>();a->settings.thirdParty=b->settings.thirdParty=false;a->visible=b->visible=true;hub->request(a);hub->request(b);settle();require(hub->preview(a,false,false),"Private preview");require(!std::atomic_load(&a->frame)->rows.empty() && std::atomic_load(&b->frame)->rows.empty(),"Preview leaked to clone");const auto privateKey=a->previewChannel;hub->clear(a);require(std::atomic_load(&a->frame)->rows.empty(),"Local clear");require(hub->preview(a,false,false,true) && std::atomic_load(&a->frame)->rows.size()==1,"After clear delivery");require(hub->preview(a,false,true) && !core->sharedSnapshot()->channels.count({events::EventOrigin::Preview,privateKey}),"Preview cleanup");
	require(hub->preview(a,false,false),"Moderation private preview");const auto privateBefore=std::atomic_load(&a->frame);const auto deleted=privateBefore->view.messages.back().event;
	require(hub->moderatePreview(a,events::ChatModerationAction::DeleteMessage),"Private delete");const auto privateAfter=std::atomic_load(&a->frame);for(const auto &row:privateAfter->rows)require(row.event!=deleted,"Private deleted row retained");require(std::atomic_load(&b->frame)->rows.empty(),"Moderation leaked to clone");
	const auto silenced=std::get<events::ChatEvent>(privateAfter->view.messages.back().event->data().payload).author.id;require(hub->moderatePreview(a,events::ChatModerationAction::ClearUser),"Private user clear");for(const auto &row:std::atomic_load(&a->frame)->rows)require(std::get<events::ChatEvent>(row.event->data().payload).author.id!=silenced,"Private timeout retained message");
	require(hub->moderatePreview(a,events::ChatModerationAction::ClearAll) && std::atomic_load(&a->frame)->rows.empty(),"Private room clear");require(hub->preview(a,false,false,true) && std::atomic_load(&a->frame)->rows.size()==1,"New chat after moderation clear");require(!hub->moderatePreview(b,events::ChatModerationAction::ClearAll) && core->sharedSnapshot()->channels.at(key)->chat.size()==64,"Inactive TEST moderates real chat");require(hub->preview(a,false,true),"Finalize moderation TEST");
	a->settings.thirdParty=true;require(media->ingest(testing::layerBase,testing::effectPng()),"Private BTTV fixture");
	require(hub->preview(a,false,false,false,ChatPreviewEffects::BTTVTransforms),"Private BTTV TEST");const auto bttvFrame=std::atomic_load(&a->frame);
	require(bttvFrame->rows.size()==1 && bttvFrame->rows[0].emotes.size()==4,"Private BTTV TEST composition");const unsigned transforms[]={chat::FlipX,chat::FlipY,chat::TurnLeft,chat::TurnRight};for(size_t i=0;i<4;++i)require(bttvFrame->rows[0].emotes[i].effects==transforms[i],"Private BTTV TEST flags");
	require(std::atomic_load(&b->frame)->rows.empty() && core->sharedSnapshot()->channels.at(key)->chat.size()==64 && core->sharedSnapshot()->channels.at(key)->daily.bits==0,"BTTV TEST leaks to clone or real state");require(hub->preview(a,false,true),"Finalize BTTV TEST");a->settings.thirdParty=false;
	a->settings.thirdParty=true;require(hub->preview(a,false,false,false,ChatPreviewEffects::BTTVMood),"Private mood TEST");const auto mood=std::atomic_load(&a->frame);require(mood->rows.size()==1 && mood->rows[0].emotes.size()==4,"Private mood TEST composition");const unsigned moods[]={chat::Party,chat::Shake,chat::Cursed,chat::Party|chat::Shake};for(size_t i=0;i<4;++i)require(mood->rows[0].emotes[i].effects==moods[i],"Private mood TEST flags");require(std::atomic_load(&b->frame)->rows.empty() && core->sharedSnapshot()->channels.at(key)->chat.size()==64 && core->sharedSnapshot()->channels.at(key)->daily.subs==0,"Mood TEST leaks to real state");require(hub->preview(a,false,true),"Finalize mood TEST");a->settings.thirdParty=false;
    a->settings.thirdParty=true;require(hub->preview(a,false,false,false,ChatPreviewEffects::BTTVWide),"Private wide TEST");const auto wide=std::atomic_load(&a->frame);require(wide->rows.size()==1 && wide->rows[0].emotes.size()==4,"Private wide TEST composition");const unsigned widths[]={0,chat::Wide,chat::Wide|chat::Party|chat::Shake,chat::Wide|chat::TurnRight};for(size_t i=0;i<4;++i)require(wide->rows[0].emotes[i].effects==widths[i],"Private wide TEST flags");require(std::atomic_load(&b->frame)->rows.empty() && core->sharedSnapshot()->channels.at(key)->chat.size()==64 && core->sharedSnapshot()->channels.at(key)->daily.subs==0,"Wide TEST leaks to real state");require(hub->preview(a,false,true),"Finalize wide TEST");a->settings.thirdParty=false;
	const state::ChannelKey cloneKey{events::EventOrigin::Twitch,"789"};require(bool(core->processEvent({"clone-message",cloneKey.channelId,now,cloneKey.origin,events::ChatEvent{{"viewer","Viewer"},"Shared history"}})),"Clone history admission");core->processPendingEvents();core->selectChannel(cloneKey);hub->refresh();require(!std::atomic_load(&a->frame)->rows.empty() && !std::atomic_load(&b->frame)->rows.empty(),"Live clones missing history");b->visible=false;std::atomic_store(&b->frame,sources::ChatFramePtr{});
	require(bool(core->processEvent({"clone-clear",cloneKey.channelId,now+std::chrono::seconds(100),cloneKey.origin,events::ChatModerationEvent{}})),"Clone clear admission");core->processPendingEvents();hub->refresh();require(std::atomic_load(&a->frame)->rows.empty(),"Visible clone retained deleted content");b->visible=true;hub->refresh();require(std::atomic_load(&b->frame)->rows.empty(),"Hidden clone resurrected moderated history");core->selectChannel({events::EventOrigin::Simulation,"simulation"});hub->refresh();
	a->visible=false;std::atomic_store(&a->frame,sources::ChatFramePtr{});const auto idle=visual->stats().renders;hub->refresh();require(visual->stats().renders==idle,"Hidden source prepares graphics");
	const auto registry=presets::Registry::fromFile(OTO_PRESET_FILE);visual::AssetManager assets(QFileInfo(OTO_PRESET_FILE).dir().filePath("assets"));
	{
	 auto cursedMedia=testing::ffzCursedCache(visual);auto cursedHub=std::make_shared<sources::ChatHub>(core,visual,renderer,cursedMedia);auto ca=std::make_shared<sources::ChatInstance>(),cb=std::make_shared<sources::ChatInstance>();ca->visible=cb->visible=true;cursedHub->request(ca);cursedHub->request(cb);settle();
	 require(cursedHub->preview(ca,false,false,false,ChatPreviewEffects::FFZCursed),"Private FFZ Cursed TEST");const auto privateFrame=std::atomic_load(&ca->frame);require(privateFrame->rows.size()==1 && privateFrame->rows[0].emotes.size()==4,"Private FFZ Cursed composition");const unsigned flags[]={0,chat::Cursed,chat::Cursed|chat::FlipX,chat::Cursed|chat::FlipY};for(size_t i=0;i<4;++i)require(privateFrame->rows[0].emotes[i].effects==flags[i],"Private FFZ Cursed flags");require(std::atomic_load(&cb->frame)->rows.empty() && core->sharedSnapshot()->channels.at(key)->chat.size()==64 && core->sharedSnapshot()->channels.at(key)->daily.subs==0,"FFZ Cursed TEST leaks to real state or clone");require(cursedHub->preview(ca,false,true),"Finalize FFZ Cursed TEST");cursedMedia->stop();
	}
	const auto styledPreview=core->createChatPreview();const state::ChannelKey styledKey{events::EventOrigin::Preview,styledPreview};
	for(const auto &definition:registry.entries())for(const bool horizontal:{false,true}){
		require(bool(definition.chatStyle),"Factory chat style missing");sources::ChatSettings skin;skin.presetId=definition.id;skin.style=*definition.chatStyle;skin.layout=horizontal?sources::ChatLayout::Horizontal:sources::ChatLayout::Vertical;skin.width=horizontal?1920:600;skin.height=horizontal?100:700;
		const auto art=skin.style.atlas.isEmpty()?visual::ImagePtr{}:assets.loadRegion(skin.style.atlas,skin.style.crop);
		if(!skin.style.rowCards)require(art && art->image.pixelColor(art->image.width()/2,art->image.height()/2).alpha()==0,"Authored chat panel contains an opaque center");
		const auto decorated=sources::prepareChatFrame(*visual,skin,sources::chatView(core->sharedSnapshot(),styledKey,skin),{},mono,false,media.get(),art);
		require(bool(decorated->border)==(!skin.style.atlas.isEmpty() || skin.style.circuit),"Chat decoration not prepared");
		for(const auto &row:decorated->rows)if(horizontal || decorated->content.contains(row.position.toPoint()))require(row.graphic->image.height()<=decorated->content.height()-8,"Chat cell exceeds interior");
		if(!skin.style.rowCards){
			require(decorated->border->image.pixelColor(skin.width/2,skin.height/2).alpha()==skin.backgroundOpacity*255/100,"Panel opacity compounded by fill");
			for(const auto &row:decorated->rows)require(row.graphic->image.pixelColor(row.graphic->image.width()/2,row.graphic->image.height()-2).alpha()==0,"Shared panel still has individual message boxes");
			const auto switched=sources::prepareChatFrame(*visual,skin,decorated->view,richFrame,mono,false,media.get(),art);for(const auto &row:switched->rows)require(row.entered==visual::MotionClock::time_point{},"Style switch replays chat entrance");
		}
		const auto count=visual->stats().renders;require(sources::prepareChatFrame(*visual,skin,decorated->view,decorated,mono+std::chrono::seconds(1),false,media.get(),art)==decorated && count==visual->stats().renders,"Style idle reraster");
		if(skin.style.fractured || !skin.style.fillBehindRows){
			require(!decorated->border || decorated->border->image.pixelColor(skin.width/2,skin.height/2).alpha()==0,"Separate cells have an opaque shared base");
			auto translucent=skin;translucent.backgroundOpacity=85;
			const auto shown=sources::prepareChatFrame(*visual,translucent,decorated->view,{},mono,false,media.get(),art);
			translucent.showFrame=false;
			const auto hidden=sources::prepareChatFrame(*visual,translucent,shown->view,shown,mono,false,media.get());
			require(hidden->rows.back().graphic==shown->rows.back().graphic,"Hiding outer frame removes separate cell details");
			const auto &cell=hidden->rows.back().graphic->image;
			require(cell.pixelColor(cell.width()-20,cell.height()/2).alpha()==85*255/100,"Separate cell opacity is compounded");
			translucent.backgroundOpacity=0;
			const auto clear=sources::prepareChatFrame(*visual,translucent,shown->view,{},mono,false,media.get());
			const auto &clearCell=clear->rows.back().graphic->image;
			require(clearCell.pixelColor(clearCell.width()-20,clearCell.height()/2).alpha()==0,"Separate cell retains dark fill at zero opacity");
			bool retainedEdge=false;for(int x=0;x<clearCell.width();++x){const auto pixel=clearCell.pixelColor(x,2);retainedEdge=retainedEdge || (pixel.alpha()>0 && std::max({pixel.red(),pixel.green(),pixel.blue()})>100);}
			require(retainedEdge,"Zero background opacity removes cell contour");
		}
		auto clearView=decorated->view;clearView.messages.clear();const auto emptyStyle=sources::prepareChatFrame(*visual,skin,clearView,decorated,mono,false,media.get(),art);require(emptyStyle->rows.empty() && emptyStyle->border==decorated->border,"Clear loses frame or leaves messages");
		skin.backgroundOpacity=0;const auto faded=sources::prepareChatFrame(*visual,skin,decorated->view,{},mono,false,media.get(),art);require(!faded->border || faded->border->image.pixelColor(skin.width/2,skin.height/2).alpha()<8,"Opacity retains dark interior");
		skin.backgroundOpacity=85;skin.showFrame=false;
		const auto unframed=sources::prepareChatFrame(*visual,skin,decorated->view,decorated,mono,false,media.get());
		require(!unframed->artwork && unframed->rows.size()==decorated->rows.size(),"Hiding frame loses messages or requires artwork");
		if(skin.style.rowCards)require(!unframed->border,"Hidden decorative frame remains");
		else require(unframed->border && unframed->border->image.pixelColor(0,skin.height/2).alpha()==0 && unframed->border->image.pixelColor(skin.width/2,skin.height/2).alpha()==85*255/100,"Hidden frame loses adjustable panel background");
		if(definition.id=="minimal"){
			require(!art && !decorated->border,"Minimal adds decorative artwork absent from its reference");
			require(unframed->rows.back().graphic==decorated->rows.back().graphic,"Minimal frame switch alters its clean cells");
			const auto &cell=decorated->rows.back().graphic->image;
			require(cell.pixelColor(1,1).alpha()==0 && cell.pixelColor(cell.width()/2,2).alpha()>0,"Minimal rounded cell contour missing");
		}
		if(skin.style.riveted){
			require(definition.id=="urban-city" && !skin.style.fillBehindRows,"Industrial rails enabled outside their preset");
			auto plain=skin;plain.style.riveted=false;
			const auto withoutRails=sources::prepareChatFrame(*visual,plain,decorated->view,{},mono,false,media.get(),art);
			require(withoutRails->rows.back().graphic->image!=decorated->rows.back().graphic->image,"Industrial rails not drawn or style omitted from cache key");
			const auto &with=decorated->rows.back().graphic->image;const auto &without=withoutRails->rows.back().graphic->image;
			for(int y=7;y<with.height()-7;++y)for(int x=8;x<with.width()-8;++x)require(with.pixelColor(x,y)==without.pixelColor(x,y),"Industrial details obstruct message interior");
		}
		if(skin.style.holographic){
			require(definition.id=="futuristic-hologram" && !skin.style.fillBehindRows,"Holographic glass enabled outside its preset");
			auto plain=skin;plain.style.holographic=false;
			const auto withoutGlass=sources::prepareChatFrame(*visual,plain,decorated->view,{},mono,false,media.get(),art);
			require(withoutGlass->rows.back().graphic->image!=decorated->rows.back().graphic->image,"Holographic highlights absent or omitted from cache key");
			const auto &with=decorated->rows.back().graphic->image;const auto &without=withoutGlass->rows.back().graphic->image;
			for(int y=7;y<with.height()-7;++y)for(int x=8;x<with.width()-8;++x)require(with.pixelColor(x,y)==without.pixelColor(x,y),"Holographic details obstruct message interior");
		}
		if(!skin.style.rowCards && skin.style.scanlines){
			const auto &panel=unframed->border->image;
			const auto phosphor=panel.pixelColor(skin.width/2,skin.style.insetY+4),glass=panel.pixelColor(skin.width/2,skin.style.insetY+5);
			require(phosphor!=glass && phosphor.alpha()==glass.alpha(),"Shared phosphor lines missing or changing background opacity");
		}
		for(const auto &row:unframed->rows)require(row.entered==visual::MotionClock::time_point{},"Frame toggle replays entrance");
		skin.showFrame=true;const auto restored=sources::prepareChatFrame(*visual,skin,decorated->view,unframed,mono,false,media.get(),art);require(bool(restored->border)==bool(decorated->border),"Frame toggle cannot restore decoration");
	}
	// A round ornament must remain round when its frame becomes a wide strip.
	QImage roundArt(100,100,QImage::Format_RGBA8888_Premultiplied);roundArt.fill(Qt::transparent);
	{QPainter painter(&roundArt);painter.setRenderHint(QPainter::Antialiasing);painter.setPen(Qt::NoPen);painter.setBrush(Qt::red);painter.drawEllipse(QRect(4,4,12,12));}
	const auto ornament=std::make_shared<const visual::ImageData>(visual::ImageData{roundArt});
	for(const bool horizontal:{false,true}){
		sources::ChatSettings aspect;aspect.width=600;aspect.height=horizontal?100:700;
		aspect.style.cap=aspect.style.capY=200;aspect.style.corner=64;aspect.style.preserveCornerAspect=true;
		const auto panel=sources::prepareChatFrame(*visual,aspect,{key,0,{}},{},mono,false,nullptr,ornament)->border;
		QRect red;for(int y=0;y<100;++y)for(int x=0;x<100;++x){const auto pixel=panel->image.pixelColor(x,y);if(pixel.red()>200 && pixel.green()<50 && pixel.alpha()>200)red=red.united(QRect(x,y,1,1));}
		require(red.width()>10 && std::abs(red.width()-red.height())<=1,"Round corner ornament stretched into an ellipse");
	}
	// Extreme supported insets still fit all fragments inside a 100 px source.
	auto tight=tiny;tight.style=*registry.resolve("neon-dj").chatStyle;tight.style.insetY=32;
	const auto tightArt=assets.loadRegion(tight.style.atlas,tight.style.crop);
	const auto tightFrame=sources::prepareChatFrame(*visual,tight,sources::chatView(core->sharedSnapshot(),key,tight),{},mono,false,media.get(),tightArt);
	require(tightFrame->rows.back().graphic->image.height()<=tightFrame->content.height()-8,"Small decorated source clips its row");
	require(tightFrame->rows.back().bodySize.height()*tightFrame->rows.back().bodyScale<=tightFrame->rows.back().bodyBox.height(),"Small decorated source loses message body");
	tight.style=*registry.resolve("horror").chatStyle;
	const auto horrorSmall=sources::prepareChatFrame(*visual,tight,sources::chatView(core->sharedSnapshot(),key,tight),{},mono,false,media.get(),assets.loadRegion(tight.style.atlas,tight.style.crop));
	require(horrorSmall->rows.back().graphic->image.height()<=horrorSmall->content.height()-8 && horrorSmall->rows.back().bodySize.height()*horrorSmall->rows.back().bodyScale<=horrorSmall->rows.back().bodyBox.height(),"Fractured compact source clips complete message");
	tight.style=*registry.resolve("love").chatStyle;
	const auto loveSmall=sources::prepareChatFrame(*visual,tight,sources::chatView(core->sharedSnapshot(),key,tight),{},mono,false,media.get(),assets.loadRegion(tight.style.atlas,tight.style.crop));
	require(loveSmall->rows.back().graphic->image.height()<=loveSmall->content.height()-8 && loveSmall->rows.back().bodySize.height()*loveSmall->rows.back().bodyScale<=loveSmall->rows.back().bodyBox.height(),"Rounded compact source clips complete message");
	tight.style=*registry.resolve("graffiti").chatStyle;
	const auto paintSmall=sources::prepareChatFrame(*visual,tight,sources::chatView(core->sharedSnapshot(),key,tight),{},mono,false,media.get(),assets.loadRegion(tight.style.atlas,tight.style.crop));
	require(paintSmall->rows.back().graphic->image.height()<=paintSmall->content.height()-8 && paintSmall->rows.back().bodySize.height()*paintSmall->rows.back().bodyScale<=paintSmall->rows.back().bodyBox.height(),"Painted compact source clips complete message");
	tight.style=*registry.resolve("minimal").chatStyle;
	const auto minimalSmall=sources::prepareChatFrame(*visual,tight,sources::chatView(core->sharedSnapshot(),key,tight),{},mono,false,media.get());
	require(minimalSmall->rows.back().graphic->image.height()<=minimalSmall->content.height()-8 && minimalSmall->rows.back().bodySize.height()*minimalSmall->rows.back().bodyScale<=minimalSmall->rows.back().bodyBox.height(),"Minimal compact source clips complete message");
	tight.style=*registry.resolve("urban-city").chatStyle;
	const auto urbanArt=assets.loadRegion(tight.style.atlas,tight.style.crop);
	const auto urbanSmall=sources::prepareChatFrame(*visual,tight,sources::chatView(core->sharedSnapshot(),key,tight),{},mono,false,media.get(),urbanArt);
	require(urbanSmall->rows.back().graphic->image.height()<=urbanSmall->content.height()-8 && urbanSmall->rows.back().bodySize.height()*urbanSmall->rows.back().bodyScale<=urbanSmall->rows.back().bodyBox.height(),"Industrial compact source clips complete message");
	tight.style=*registry.resolve("futuristic-hologram").chatStyle;
	const auto holoArt=assets.loadRegion(tight.style.atlas,tight.style.crop);
	const auto holoSmall=sources::prepareChatFrame(*visual,tight,sources::chatView(core->sharedSnapshot(),key,tight),{},mono,false,media.get(),holoArt);
	require(holoSmall->rows.back().graphic->image.height()<=holoSmall->content.height()-8 && holoSmall->rows.back().bodySize.height()*holoSmall->rows.back().bodyScale<=holoSmall->rows.back().bodyBox.height(),"Holographic compact source clips complete message");
	core->releaseSourcePreview(styledPreview);core->releaseSourcePreview(preview);hub->stop();media->stop();renderer.reset();core->stop();obs_shutdown();std::cout<<"PASS live chat layouts, commands, shared GIF emotes, catalogs, movement, cache, private TEST, ten shared styles\n";return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}


