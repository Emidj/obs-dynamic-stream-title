// SPDX-License-Identifier: GPL-2.0-or-later
#include "adapters/chat-draw.hpp"
#include "core/plugin-core.hpp"
#include "chat-fixtures.hpp"
#include "chat-badge-checks.hpp"
#include "chat-layer-checks.hpp"
#include "chat-effect-checks.hpp"
#include "chat-bttv-checks.hpp"
#include "chat-mood-checks.hpp"
#include "chat-wide-checks.hpp"
#include "chat-ffz-cursed-checks.hpp"
#include "chat-ffz-full-checks.hpp"
#include "chat-cheer-checks.hpp"
#include <QApplication>
#include <QDir>
#include <QBuffer>
#include <QFileInfo>
#include "visual/asset-manager.hpp"
#include <cmath>
#include <limits>
#include <obs.h>
#include <iostream>
using namespace oto;
namespace {
void require(bool value,const char *message){if(!value)throw std::runtime_error(message);}
int difference(const QImage &a,const QImage &b){int d=0;for(int y=0;y<a.height();++y)for(int x=0;x<a.width()*4;++x)d=std::max(d,std::abs(int(a.constScanLine(y)[x])-int(b.constScanLine(y)[x])));return d;}
struct Config:IConfigStore{ConfigLoadResult load()override{return {ConfigStatus::Loaded,{false}};}bool save(Settings)override{return true;}};
struct Log:ILogger{void write(LogLevel,std::string_view)const noexcept override{}};
}
int main(int argc,char **argv){QApplication app(argc,argv);try{
	require(argc==4 || argc==5,"Usage: chat-gpu-probe graphics-module obs-data output [style]");const QString onlyStyle=argc==5?QString::fromUtf8(argv[4]):QString{};require(obs_startup("en-US",nullptr,nullptr),"OBS startup");
#pragma warning(push)
#pragma warning(disable:4996)
	obs_add_data_path(argv[2]);
#pragma warning(pop)
	obs_video_info video{};video.graphics_module=argv[1];video.fps_num=30;video.fps_den=1;video.base_width=video.output_width=1920;video.base_height=video.output_height=1080;video.output_format=VIDEO_FORMAT_RGBA;video.colorspace=VIDEO_CS_DEFAULT;video.range=VIDEO_RANGE_FULL;video.scale_type=OBS_SCALE_DISABLE;require(obs_reset_video(&video)==OBS_VIDEO_SUCCESS,"D3D startup");
	PluginCore core(std::make_unique<Config>(),std::make_unique<Log>());core.start();auto engine=std::make_shared<visual::VisualEngine>();chat::EmoteCache media(engine);const QString url="https://static-cdn.jtvnw.net/emoticons/v2/fixture/animated/dark/2.0";require(media.ingest(url,animatedChatFixture()),"GIF fixture");
	const QString subBadge="https://static-cdn.jtvnw.net/badges/v1/sub-fixture/3",bitsBadge="https://static-cdn.jtvnw.net/badges/v1/bits-fixture/3";
	media.ensureBadges("123");require(media.ingestBadges({},badgeFixtureCatalog(subBadge)),"Subscriber catalog fixture");require(media.ingestBadges("123",badgeFixtureCatalog(bitsBadge,"bits","1000")),"Bits catalog fixture");
	require(media.ingest(subBadge,badgeFixturePng(QColor(0,255,0))) && media.ingest(bitsBadge,badgeFixturePng(QColor(0,200,255))),"Badge image fixtures");
	const auto start=visual::MotionClock::now();const QString output=QString::fromUtf8(argv[3]);QDir().mkpath(output);
	{ObsVisualRenderer renderer;if(onlyStyle.isEmpty())for(const bool horizontal:{false,true}){
		const auto preview=core.createChatPreview();const state::ChannelKey key{events::EventOrigin::Preview,preview};
		sources::ChatSettings settings;settings.layout=horizontal?sources::ChatLayout::Horizontal:sources::ChatLayout::Vertical;settings.width=horizontal?1920:600;settings.height=horizontal?100:700;
		auto frame=sources::prepareChatFrame(*engine,settings,sources::chatView(core.sharedSnapshot(),key,settings),{},start,false,&media);
		events::ChatEvent message{{"fixture-user","Álex_東京"},"Animated Dance + Dance"};message.fragments={{"Animated ",{},false},{"Dance","fixture",true},{" + ",{},false},{"Dance","fixture",true}};
		require(bool(core.processEvent({horizontal?"gif-horizontal":"gif-vertical",preview,std::chrono::system_clock::now(),events::EventOrigin::Preview,message})),"Admission");core.processPendingEventsFor(key.origin,key.channelId);
		frame=sources::prepareChatFrame(*engine,settings,sources::chatView(core.sharedSnapshot(),key,settings),frame,start+std::chrono::seconds(1),false,&media);require(frame->rows.back().emotes.size()==2,"Inline GIFs missing");
		const auto capture=[&](int ms){return renderer.renderOffscreen(QSize(settings.width+20,settings.height+20),[&]{drawChatFrame(renderer,*frame,start+std::chrono::milliseconds(ms),{10,10});});};
		const auto first=capture(1500),second=capture(1600);require(difference(first,second)>30,"Emote frames do not animate on GPU");require(first.save(output+(horizontal?"/horizontal.png":"/vertical.png")),"Save preview");
		for(int ms=1000;ms<=3000;ms+=33)capture(ms); // Also warm cards departing at the start of the move.
		const auto uploads=renderer.stats().uploads,rasters=engine->stats().renders;
		for(int ms=1000;ms<=3000;ms+=33){const auto image=capture(ms);for(int y=0;y<image.height();++y)for(int x=0;x<image.width();++x)if(!QRect(10,10,settings.width,settings.height).contains(x,y))require(image.constScanLine(y)[x*4+3]==0,"Chat exceeds source bounds");}
		require(renderer.stats().uploads==uploads && engine->stats().renders==rasters,"Animation rasterizes or uploads every frame");
		settings.animatedEmotes=false;frame=sources::prepareChatFrame(*engine,settings,frame->view,frame,start+std::chrono::seconds(4),false,&media);require(difference(capture(5000),capture(5100))==0,"Disabled emotes still animate");
		std::cout<<"PASS GPU chat "<<(horizontal?"horizontal":"vertical")<<": inline GIF loop, guards, no per-frame raster/upload, pause\n";
		QImage tail(56,56,QImage::Format_RGBA8888_Premultiplied);tail.fill(QColor(0,255,0));QByteArray png;QBuffer buffer(&png);buffer.open(QIODevice::WriteOnly);require(tail.save(&buffer,"PNG"),"Tail PNG");require(media.ingest("https://static-cdn.jtvnw.net/emoticons/v2/tail/static/dark/2.0",png),"Tail decode");
		events::ChatEvent many{{"fixture-user","MarinaDJ"},""};for(int i=0;i<60;++i){if(i){many.message+=' ';many.fragments.push_back({" ",{},false});}many.message+="Dance";many.fragments.push_back({"Dance",i==59?"tail":"fixture",i!=59});}
		require(bool(core.processEvent({horizontal?"many-horizontal":"many-vertical",preview,std::chrono::system_clock::now(),events::EventOrigin::Preview,many})),"Many admission");core.processPendingEventsFor(key.origin,key.channelId);
		settings.messages=1;frame=sources::prepareChatFrame(*engine,settings,sources::chatView(core.sharedSnapshot(),key,settings),{},start,false,&media);const auto row=frame->rows.back();require(row.emotes.size()==60,"Overflow discarded emotes");
		const int lastMs=3000;
		const auto head=capture(1000),last=capture(lastMs);const auto p=row.position+QPointF(10,10)+row.bodyBox.topLeft()+row.emotes.back().box.center()*row.bodyScale;
		const auto color=last.pixelColor(qRound(p.x()),qRound(p.y()));require(color.green()>240 && color.red()<10 && color.blue()<10,"Final emote missing from GPU output");
		for(const auto &emote:row.emotes){const auto point=row.position+QPointF(10,10)+row.bodyBox.topLeft()+emote.box.center()*row.bodyScale;const auto pixel=head.pixelColor(qRound(point.x()),qRound(point.y()));require(pixel.red()>200 || pixel.blue()>200 || pixel.green()>200,"Not all 60 emotes visible simultaneously");}
		require(difference(head,last)==0,"Message still does ping-pong");
		const QRect fixed(int(row.position.x())+10,int(row.position.y())+10,int(row.bodyBox.x())-1,row.graphic->image.height());require(difference(head.copy(fixed),last.copy(fixed))==0,"Scrolling body overlaps fixed name/badge");
		require(last.save(output+(horizontal?"/horizontal-tail.png":"/vertical-tail.png")),"Save final emote");
		// Warm every page, then sample the complete trip again. Pages may upload
		// once when first visible, but there is no shaping or repeated upload.
		for(int ms=0;ms<=lastMs+200;ms+=200)capture(ms);
		const auto warm=renderer.stats().uploads,prepared=engine->stats().renders;
		for(int ms=0;ms<=lastMs+200;ms+=200){const auto image=capture(ms);for(int y=0;y<image.height();++y)for(int x=0;x<image.width();++x)if(!QRect(10,10,settings.width,settings.height).contains(x,y))require(image.constScanLine(y)[x*4+3]==0,"Overflow exceeds source");}
		require(renderer.stats().uploads==warm && engine->stats().renders==prepared,"Overflow reraster/uploads after warmup");
		if(horizontal){
			frame=sources::prepareChatFrame(*engine,settings,frame->view,{},start,false,&media);const auto oldFrame=frame;
			require(bool(core.processEvent({"departure",preview,std::chrono::system_clock::now(),events::EventOrigin::Preview,events::ChatEvent{{"next","Next"},"Hello"}})),"Departure admission");core.processPendingEventsFor(key.origin,key.channelId);
			frame=sources::prepareChatFrame(*engine,settings,sources::chatView(core.sharedSnapshot(),key,settings),frame,start+std::chrono::seconds(1),false,&media);
			require(frame->rows.size()==2 && frame->rows.front().event==oldFrame->rows.front().event,"Departure dropped immediately");
			const auto &leaving=frame->rows.front();const auto middle=sources::sampleChatRow(leaving,settings.entryMs,settings.layout,start+std::chrono::milliseconds(1175));require(middle.position.x()<leaving.from.x() && middle.position.x()>leaving.position.x() && middle.opacity==1,"Departure fades or jumps");
			for(int ms=1000;ms<=1400;ms+=10){const auto image=capture(ms);for(int y=0;y<image.height();++y)for(int x=0;x<image.width();++x)if(!QRect(10,10,settings.width,settings.height).contains(x,y))require(image.constScanLine(y)[x*4+3]==0,"Departure exceeds overlay");}
		}
		std::cout<<"PASS GPU complete "<<(horizontal?"horizontal":"vertical")<<": all 60 emotes visible simultaneously, no ping-pong, fixed name, guards, warm cache, continuous departure\n";
		if(horizontal){
			events::ChatEvent extreme{{"tiny","TinyCanvas"},std::string(4096,'W')};require(bool(core.processEvent({"tiny-full-body",preview,std::chrono::system_clock::now(),events::EventOrigin::Preview,extreme})),"Extreme admission");core.processPendingEventsFor(key.origin,key.channelId);
			settings.width=320;settings.messageSize=128;frame=sources::prepareChatFrame(*engine,settings,sources::chatView(core.sharedSnapshot(),key,settings),{},start,false,&media);require(frame->rows.back().bodyPixelSize==8 && frame->rows.back().bodyScale<1,"Extreme body not fitted");
			const auto image=capture(1000);require(image.save(output+"/tiny-full-body.png"),"Save extreme");for(int y=0;y<image.height();++y)for(int x=0;x<image.width();++x)if(!QRect(10,10,settings.width,settings.height).contains(x,y))require(image.constScanLine(y)[x*4+3]==0,"Extreme body exceeds source");
			std::cout<<"PASS GPU 4096-character body fitted to 320x100 without clipping/crash\n";
		}
	}
	const auto registry=presets::Registry::fromFile(OTO_PRESET_FILE);visual::AssetManager assets(QFileInfo(OTO_PRESET_FILE).dir().filePath("assets"));
	if(!onlyStyle.isEmpty())require(std::any_of(registry.entries().begin(),registry.entries().end(),[&](const auto &d){return d.id==onlyStyle;}),"Unknown style filter");
	const auto styledPreview=core.createChatPreview();const state::ChannelKey styledKey{events::EventOrigin::Preview,styledPreview};
	events::ChatEvent styledMessage{{"fixture","MarinaDJ"},"Great set! Dance Dance",false,"#FF44BB",events::ChatRole::Subscriber,{{"Great set! ",{},false},{"Dance","fixture",true},{" ",{},false},{"Dance","fixture",true}}};
	styledMessage.badges={{"subscriber","0",{}},{"bits","1000",{}}};
	require(bool(core.processEvent({"styled-gif",styledPreview,std::chrono::system_clock::now(),events::EventOrigin::Preview,styledMessage})),"Styled admission");core.processPendingEventsFor(styledKey.origin,styledKey.channelId);
	for(const auto &definition:registry.entries())for(const bool horizontal:{false,true}){
		if(!onlyStyle.isEmpty() && definition.id!=onlyStyle)continue;
		sources::ChatSettings skin;skin.presetId=definition.id;skin.style=*definition.chatStyle;skin.nameFamily=skin.messageFamily=skin.style.family;skin.layout=horizontal?sources::ChatLayout::Horizontal:sources::ChatLayout::Vertical;skin.width=horizontal?1920:600;skin.height=horizontal?100:700;
		const auto art=skin.style.atlas.isEmpty()?visual::ImagePtr{}:assets.loadRegion(skin.style.atlas,skin.style.crop);
		const auto decorated=sources::prepareChatFrame(*engine,skin,sources::chatView(core.sharedSnapshot(),styledKey,skin),{},start,false,&media,art);
		const auto shot=[&](int ms){return renderer.renderOffscreen({skin.width+20,skin.height+20},[&]{drawChatFrame(renderer,*decorated,start+std::chrono::milliseconds(ms),{10,10});});};
		const auto first=shot(1000),second=shot(1100);require(difference(first,second)>30,"Styled emotes lost animation");require(first.save(output+"/"+definition.id+(horizontal?"-horizontal.png":"-vertical.png")),"Save style");
		const auto &lastRow=decorated->rows.back();require(lastRow.emotes.size()==2 && !lastRow.emotes[0].box.intersects(lastRow.emotes[1].box),"Styled inline emotes overlap");for(const auto &emote:lastRow.emotes){const auto p=lastRow.position+QPointF(10,10)+lastRow.bodyBox.topLeft()+emote.box.center()*lastRow.bodyScale;require(first.pixelColor(qRound(p.x()),qRound(p.y())).red()>240,"Styled inline emote missing");}
		require(lastRow.badges.size()==2,"GPU lost multiple badges");for(const auto &badge:lastRow.badges){const auto p=lastRow.position+QPointF(10,10)+badge.box.center();const auto color=first.pixelColor(qRound(p.x()),qRound(p.y()));require(color.green()>180 && color.red()<10,"Badge absent from GPU output");}
		const auto uploads=renderer.stats().uploads,rasters=engine->stats().renders;
		for(int ms=1200;ms<=1700;ms+=100){const auto image=shot(ms);for(int y=0;y<image.height();++y)for(int x=0;x<image.width();++x)if(!QRect(10,10,skin.width,skin.height).contains(x,y))require(image.constScanLine(y)[x*4+3]==0,"Style exceeds canvas");}
		require(renderer.stats().uploads==uploads && engine->stats().renders==rasters,"Style per-frame work");
		if(definition.id=="neon-dj" || definition.id=="cyberpunk" || definition.id=="retro-crt" || definition.id=="space" || definition.id=="horror" || definition.id=="love" || definition.id=="graffiti" || definition.id=="minimal" || definition.id=="urban-city" || definition.id=="futuristic-hologram"){
			skin.showFrame=false;const auto unframed=sources::prepareChatFrame(*engine,skin,decorated->view,decorated,start,false,&media);
			const auto hidden=renderer.renderOffscreen({skin.width+20,skin.height+20},[&]{drawChatFrame(renderer,*unframed,start+std::chrono::milliseconds(1000),{10,10});});
			require(hidden.save(output+"/"+definition.id+(horizontal?"-horizontal-no-frame.png":"-vertical-no-frame.png")),"Save hidden border");
			for(int y=0;y<hidden.height();++y)for(int x=0;x<hidden.width();++x)if(!unframed->content.translated(10,10).contains(x,y))require(hidden.constScanLine(y)[x*4+3]==0,"Hidden frame still draws outside content");
			const auto &r=unframed->rows.back();for(const auto &emote:r.emotes){const auto point=r.position+QPointF(10,10)+r.bodyBox.topLeft()+emote.box.center()*r.bodyScale;require(hidden.pixelColor(qRound(point.x()),qRound(point.y())).red()>240,"Frame toggle removes emotes");}
			for(const auto &badge:r.badges){const auto point=r.position+QPointF(10,10)+badge.box.center();require(hidden.pixelColor(qRound(point.x()),qRound(point.y())).green()>180,"Frame toggle removes badges");}
			std::cout<<"PASS GPU "<<definition.id.toStdString()<<' '<<(horizontal?"horizontal":"vertical")<<": no-frame content retains complete inline emotes\n";
		}
		std::cout<<"PASS GPU chat style "<<definition.id.toStdString()<<' '<<(horizontal?"horizontal":"vertical")<<": authored border, GIF, guards, warm cache\n";
		const auto moderationPreview=core.createChatPreview();const state::ChannelKey moderationKey{events::EventOrigin::Preview,moderationPreview};skin.showFrame=true;
		auto moderated=sources::prepareChatFrame(*engine,skin,sources::chatView(core.sharedSnapshot(),moderationKey,skin),{},start,false,&media,art);
		const auto deleted=moderated->view.messages.back().event;core.moderateChatPreview(moderationPreview,events::ChatModerationAction::DeleteMessage);
		moderated=sources::prepareChatFrame(*engine,skin,sources::chatView(core.sharedSnapshot(),moderationKey,skin),moderated,start+std::chrono::milliseconds(1100),false,&media,art);for(const auto &row:moderated->rows)require(row.event!=deleted,"GPU retains deleted row");
		const auto deletedShot=renderer.renderOffscreen({skin.width+20,skin.height+20},[&]{drawChatFrame(renderer,*moderated,start+std::chrono::milliseconds(1600),{10,10});});require(deletedShot.save(output+"/"+definition.id+(horizontal?"-horizontal-moderated.png":"-vertical-moderated.png")),"Save moderated style");
		const auto silenced=std::get<events::ChatEvent>(moderated->view.messages.back().event->data().payload).author.id;core.moderateChatPreview(moderationPreview,events::ChatModerationAction::ClearUser);
		moderated=sources::prepareChatFrame(*engine,skin,sources::chatView(core.sharedSnapshot(),moderationKey,skin),moderated,start+std::chrono::milliseconds(1700),false,&media,art);for(const auto &row:moderated->rows)require(std::get<events::ChatEvent>(row.event->data().payload).author.id!=silenced,"GPU retains silenced user");
		core.moderateChatPreview(moderationPreview,events::ChatModerationAction::ClearAll);moderated=sources::prepareChatFrame(*engine,skin,sources::chatView(core.sharedSnapshot(),moderationKey,skin),moderated,start+std::chrono::milliseconds(1800),false,&media,art);require(moderated->rows.empty(),"GPU clear retains content");
		const auto cleared=renderer.renderOffscreen({skin.width+20,skin.height+20},[&]{drawChatFrame(renderer,*moderated,start+std::chrono::seconds(2),{10,10});});auto emptyView=moderated->view;emptyView.messages.clear();const auto empty=sources::prepareChatFrame(*engine,skin,emptyView,{},start,false,&media,art);
		const auto expected=renderer.renderOffscreen({skin.width+20,skin.height+20},[&]{drawChatFrame(renderer,*empty,start+std::chrono::seconds(2),{10,10});});require(difference(cleared,expected)==0 && difference(cleared,deletedShot)>0,"GPU clear retains ghost pixels or removes border");
		for(int y=0;y<cleared.height();++y)for(int x=0;x<cleared.width();++x)if(!QRect(10,10,skin.width,skin.height).contains(x,y))require(cleared.constScanLine(y)[x*4+3]==0,"Moderation exceeds canvas");core.releaseSourcePreview(moderationPreview);
		std::cout<<"PASS GPU moderation "<<definition.id.toStdString()<<' '<<(horizontal?"horizontal":"vertical")<<": delete, user clear, room clear, no ghost pixels, frame retained\n";
	}core.releaseSourcePreview(styledPreview);
	{
		auto layers=testing::layerCache(engine);events::EventEngine eventEngine;eventEngine.start();
		const auto event=eventEngine.process({"gpu-layers","123",std::chrono::system_clock::now(),events::EventOrigin::Preview,testing::layerMessage()});require(bool(event),"GPU layer admission");
		const sources::ChatView view{{events::EventOrigin::Preview,"123"},1,{{event.event,{},1,1}}};
		for(const auto &definition:registry.entries())for(bool horizontal:{false,true}){
			if(!onlyStyle.isEmpty() && definition.id!=onlyStyle)continue;
			sources::ChatSettings skin;skin.presetId=definition.id;skin.style=*definition.chatStyle;skin.layout=horizontal?sources::ChatLayout::Horizontal:sources::ChatLayout::Vertical;skin.width=horizontal?1920:600;skin.height=horizontal?100:700;
			const auto art=skin.style.atlas.isEmpty()?visual::ImagePtr{}:assets.loadRegion(skin.style.atlas,skin.style.crop);
			auto layered=sources::prepareChatFrame(*engine,skin,view,{},start,false,layers.get(),art);const auto &row=layered->rows.back();require(row.emotes.size()==4 && row.emotes[0].box==row.emotes[1].box && row.emotes[1].box==row.emotes[2].box,"GPU stack geometry");
			const auto shot=[&](int ms){return renderer.renderOffscreen({skin.width+20,skin.height+20},[&]{drawChatFrame(renderer,*layered,start+std::chrono::milliseconds(ms),{10,10});});};
			const auto first=shot(1000),second=shot(1100);const auto point=row.position+QPointF(10,10)+row.bodyBox.topLeft()+row.emotes[0].box.center()*row.bodyScale;
			require(first.pixelColor(qRound(point.x()),qRound(point.y())).green()>240 && second.pixelColor(qRound(point.x()),qRound(point.y())).blue()>240,"GPU overlay animation/alpha loses base");
			const auto edge=point-QPointF(row.emotes[0].box.width()*row.bodyScale*.4,0);const auto yellow=second.pixelColor(qRound(edge.x()),qRound(edge.y()));require(yellow.red()>240 && yellow.green()>240 && yellow.blue()<10,"GPU layer order incorrect");
			const auto uploads=renderer.stats().uploads,rasters=engine->stats().renders;for(int ms=1200;ms<=1500;ms+=100)shot(ms);require(uploads==renderer.stats().uploads && rasters==engine->stats().renders,"GPU layers do per-frame upload/raster");
			require(first.save(output+"/"+definition.id+(horizontal?"-horizontal-layers.png":"-vertical-layers.png")),"Save GPU layers");
			skin.animatedEmotes=false;layered=sources::prepareChatFrame(*engine,skin,view,layered,start,false,layers.get(),art);require(difference(shot(1000),shot(1100))==0,"Layers ignore animation OFF");
			for(int y=0;y<first.height();++y)for(int x=0;x<first.width();++x)if(!QRect(10,10,skin.width,skin.height).contains(x,y))require(first.constScanLine(y)[x*4+3]==0,"Layers exceed source canvas");
			std::cout<<"PASS GPU layers "<<definition.id.toStdString()<<' '<<(horizontal?"horizontal":"vertical")<<": base, animated alpha, ordered stack, no extra advance, guards, warm cache, animation OFF\n";
		}layers->stop();eventEngine.stop();
	}
	{
		auto effects=testing::effectCache(engine);events::EventEngine eventEngine;eventEngine.start();const auto event=eventEngine.process({"gpu-effects","123",std::chrono::system_clock::now(),events::EventOrigin::Preview,testing::effectMessage()});require(bool(event),"GPU effects admission");
		const sources::ChatView view{{events::EventOrigin::Preview,"123"},1,{{event.event,{},1,1}}};
		for(const auto &definition:registry.entries())for(bool horizontal:{false,true}){
			if(!onlyStyle.isEmpty() && definition.id!=onlyStyle)continue;
			sources::ChatSettings skin;skin.presetId=definition.id;skin.style=*definition.chatStyle;skin.layout=horizontal?sources::ChatLayout::Horizontal:sources::ChatLayout::Vertical;skin.width=horizontal?1920:600;skin.height=horizontal?100:700;
			const auto art=skin.style.atlas.isEmpty()?visual::ImagePtr{}:assets.loadRegion(skin.style.atlas,skin.style.crop);
			auto frame=sources::prepareChatFrame(*engine,skin,view,{},start,false,effects.get(),art);const auto prepared=frame;const auto &row=prepared->rows.back();require(row.emotes.size()==3,"GPU draws modifier icons");
			const auto shot=[&](int ms){return renderer.renderOffscreen({skin.width+20,skin.height+20},[&]{drawChatFrame(renderer,*frame,start+std::chrono::milliseconds(ms),{10,10});});};
			const auto sample=[&](const QImage &image,int i){const auto &e=row.emotes[size_t(i)];const auto point=row.position+QPointF(10,10)+row.bodyBox.topLeft()+(e.box.center()-QPointF(e.box.width()*.15,e.box.height()*.15))*row.bodyScale;return image.pixelColor(qRound(point.x()),qRound(point.y()));};
			const auto zero=shot(1500),quarter=shot(1875);const auto x=sample(zero,0),y=sample(zero,1),spin=sample(zero,2),rotated=sample(quarter,2);
			require(x.red()>220 && x.green()<40 && y.green()>220 && y.red()<40,"GPU mirror axes incorrect");require(spin.red()>220 && spin.green()>220 && rotated.green()>220 && rotated.red()<40,"GPU spin phase/direction incorrect");require(difference(zero,quarter)>40,"GPU spin static");
			const auto uploads=renderer.stats().uploads,rasters=engine->stats().renders;
			for(int ms=0;ms<=1500;ms+=75){const auto image=shot(ms);for(int py=0;py<image.height();++py)for(int px=0;px<image.width();++px)if(!QRect(10,10,skin.width,skin.height).contains(px,py))require(image.constScanLine(py)[px*4+3]==0,"Effects exceed source bounds");}
			require(uploads==renderer.stats().uploads && rasters==engine->stats().renders,"Effects rasterize/upload per frame");require(zero.save(output+"/"+definition.id+(horizontal?"-horizontal-effects.png":"-vertical-effects.png")),"Save effects");
			skin.animatedEmotes=false;frame=sources::prepareChatFrame(*engine,skin,view,frame,start,false,effects.get(),art);require(difference(shot(1500),shot(1875))==0,"Animation OFF does not freeze spin");const auto frozen=sample(shot(1500),0);require(frozen.red()>220 && frozen.green()<40,"Animation OFF removes static mirror");
			std::cout<<"PASS GPU effects "<<definition.id.toStdString()<<' '<<(horizontal?"horizontal":"vertical")<<": horizontal/vertical mirror, 1.5s spin, transparent guards, shared atlas, no per-frame upload/raster, animation OFF\n";
		}effects->stop();eventEngine.stop();

	}
	}
	{
		const auto registry=presets::Registry::fromFile(OTO_PRESET_FILE);visual::AssetManager assets(QFileInfo(OTO_PRESET_FILE).dir().filePath("assets"));
		ObsVisualRenderer renderer;auto media=testing::bttvCache(engine);events::EventEngine events;events.start();const auto event=events.process({"gpu-bttv","123",std::chrono::system_clock::now(),events::EventOrigin::Preview,testing::bttvMessage()});require(bool(event),"GPU BTTV admission");
		const sources::ChatView view{{events::EventOrigin::Preview,"123"},1,{{event.event,{},1,1}}};
		for(const auto &definition:registry.entries())for(bool horizontal:{false,true}){
			if(!onlyStyle.isEmpty() && definition.id!=onlyStyle)continue;
			sources::ChatSettings skin;skin.presetId=definition.id;skin.style=*definition.chatStyle;skin.layout=horizontal?sources::ChatLayout::Horizontal:sources::ChatLayout::Vertical;skin.width=horizontal?1920:600;skin.height=horizontal?100:700;
			const auto art=skin.style.atlas.isEmpty()?visual::ImagePtr{}:assets.loadRegion(skin.style.atlas,skin.style.crop);auto frame=sources::prepareChatFrame(*engine,skin,view,{},start,false,media.get(),art);const auto prepared=frame;const auto &row=prepared->rows.back();require(row.emotes.size()==4,"GPU BTTV modifier consumes an emote");for(const auto &e:row.emotes)require(e.media==row.emotes.front().media,"BTTV duplicates atlas");
			const auto shot=[&](int ms){return renderer.renderOffscreen({skin.width+20,skin.height+20},[&]{drawChatFrame(renderer,*frame,start+std::chrono::milliseconds(ms),{10,10});});};
			const auto sample=[&](const QImage &image,int i){const auto &e=row.emotes[size_t(i)];const auto p=row.position+QPointF(10,10)+row.bodyBox.topLeft()+(e.box.center()-QPointF(e.box.width()*.15,e.box.height()*.15))*row.bodyScale;return image.pixelColor(qRound(p.x()),qRound(p.y()));};
			const auto first=shot(1000);for(int i=0;i<4;++i){const auto c=sample(first,i);require(i%2?c.green()>220 && c.red()<40:c.red()>220 && c.green()<40,"BTTV mirror/turn orientation incorrect");}require(difference(first,shot(1700))==0,"Static BTTV transform moves");
			const auto uploads=renderer.stats().uploads,rasters=engine->stats().renders;for(int ms=0;ms<=1500;ms+=75){const auto image=shot(ms);for(int y=0;y<image.height();++y)for(int x=0;x<image.width();++x)if(!QRect(10,10,skin.width,skin.height).contains(x,y))require(image.constScanLine(y)[x*4+3]==0,"BTTV exceeds source canvas");}require(uploads==renderer.stats().uploads && rasters==engine->stats().renders,"BTTV per-frame upload/raster");
			require(first.save(output+"/"+definition.id+(horizontal?"-horizontal-bttv.png":"-vertical-bttv.png")),"Save BTTV");skin.animatedEmotes=false;frame=sources::prepareChatFrame(*engine,skin,view,frame,start,false,media.get(),art);require(difference(first,shot(1000))==0,"Animation OFF removes static BTTV transforms");
			std::cout<<"PASS GPU BTTV "<<definition.id.toStdString()<<' '<<(horizontal?"horizontal":"vertical")<<": mirrors, quarter turns, guards, shared atlas, no per-frame upload/raster, animation OFF\n";
		}media->stop();events.stop();
	}
	{
		const auto registry=presets::Registry::fromFile(OTO_PRESET_FILE);visual::AssetManager assets(QFileInfo(OTO_PRESET_FILE).dir().filePath("assets"));ObsVisualRenderer renderer;
		auto media=testing::bttvCache(engine);events::EventEngine events;events.start();const auto event=events.process({"gpu-mood","123",std::chrono::system_clock::now(),events::EventOrigin::Preview,testing::moodMessage()});require(bool(event),"GPU mood admission");const sources::ChatView view{{events::EventOrigin::Preview,"123"},1,{{event.event,{},1,1}}};
		for(const auto &definition:registry.entries())for(bool horizontal:{false,true}){
			if(!onlyStyle.isEmpty() && definition.id!=onlyStyle)continue;
			sources::ChatSettings skin;skin.presetId=definition.id;skin.style=*definition.chatStyle;skin.layout=horizontal?sources::ChatLayout::Horizontal:sources::ChatLayout::Vertical;skin.width=horizontal?1920:600;skin.height=horizontal?100:700;
			const auto art=skin.style.atlas.isEmpty()?visual::ImagePtr{}:assets.loadRegion(skin.style.atlas,skin.style.crop);auto frame=sources::prepareChatFrame(*engine,skin,view,{},start,false,media.get(),art);const auto prepared=frame;const auto &row=prepared->rows.back();require(row.emotes.size()==4,"GPU mood creates modifier icons");
			const auto shot=[&](int ms){return renderer.renderOffscreen({skin.width+20,skin.height+20},[&]{drawChatFrame(renderer,*frame,start+std::chrono::milliseconds(ms),{10,10});});};
			const auto region=[&](int i){return QRectF(row.position+QPointF(10,10)+row.bodyBox.topLeft()+row.emotes[size_t(i)].box.topLeft()*row.bodyScale,row.emotes[size_t(i)].box.size()*row.bodyScale).toAlignedRect();};
			const auto first=shot(1500),next=shot(1550),hue=shot(1875);require(difference(first.copy(region(0)),hue.copy(region(0)))>40,"Party hue does not change");require(difference(first.copy(region(1)),next.copy(region(1)))>40 && difference(first.copy(region(1)),shot(1549).copy(region(1)))==0,"Shake is static or lacks step timing");require(difference(first.copy(region(1)),shot(2000).copy(region(1)))==0,"Shake period not 500ms");require(difference(first.copy(region(2)),hue.copy(region(2)))==0,"Cursed filter moves");
			const auto cursed=first.copy(region(2));bool white=false,black=false;for(int y=3;y<cursed.height()-3;++y)for(int x=3;x<cursed.width()-3;++x){const auto c=cursed.pixelColor(x,y);require(std::abs(c.red()-c.green())<=2 && std::abs(c.green()-c.blue())<=2,"Cursed filter not monochrome");white=white || c.red()>200;black=black || c.red()<10;}require(white && black,"Cursed lacks contrast");
			const auto fixed=QRect(int(row.position.x())+10,int(row.position.y())+10,int(row.bodyBox.x())-1,row.graphic->image.height());require(difference(first.copy(fixed),hue.copy(fixed))==0,"Mood leaks to fixed name/badge");
			const auto uploads=renderer.stats().uploads,rasters=engine->stats().renders;for(int ms=0;ms<=1500;ms+=25){const auto image=shot(ms);for(int y=0;y<image.height();++y)for(int x=0;x<image.width();++x)if(!QRect(10,10,skin.width,skin.height).contains(x,y))require(image.constScanLine(y)[x*4+3]==0,"Mood exceeds source");}require(uploads==renderer.stats().uploads && rasters==engine->stats().renders,"Mood uploads/rasterizes each frame");require(first.save(output+"/"+definition.id+(horizontal?"-horizontal-mood.png":"-vertical-mood.png")),"Save mood");require(hue.save(output+"/"+definition.id+(horizontal?"-horizontal-mood-later.png":"-vertical-mood-later.png")),"Save mood later");
			skin.animatedEmotes=false;frame=sources::prepareChatFrame(*engine,skin,view,frame,start,false,media.get(),art);const auto stopped=shot(1500);require(difference(stopped,shot(1875))==0,"Animation OFF does not pause party/shake");require(difference(stopped.copy(region(2)),first.copy(region(2)))==0,"Animation OFF removes cursed filter");
			std::cout<<"PASS GPU mood "<<definition.id.toStdString()<<' '<<(horizontal?"horizontal":"vertical")<<": party hue, step shake, cursed contrast, guards, shared atlas, no per-frame raster/upload, animation OFF\n";
		}media->stop();events.stop();
		// Isolated cells exercise premultiplied alpha, atlas neighbors and neutral reset.
		QImage image(112,56,QImage::Format_RGBA8888_Premultiplied);image.fill(Qt::blue);for(int y=0;y<56;++y)for(int x=0;x<56;++x)image.setPixelColor(x,y,x>=8 && x<48 && y>=8 && y<48?QColor(220,90,40,128):QColor(0,0,0,0));const auto atlas=engine->prepareRaster(image);
		DrawParams p;p.x=p.y=10;p.sourceRect={0,0,56,56};const auto draw=[&]{return renderer.renderOffscreen({100,100},[&]{renderer.draw(atlas,true,p);});};const auto neutral=draw();
		for(bool cursed:{false,true}){p.cellParty=!cursed;p.cellCursed=cursed;p.cellHue=cursed?0:90;const auto colored=draw();for(int y=0;y<100;++y)for(int x=0;x<100;++x){const auto a=neutral.pixelColor(x,y),b=colored.pixelColor(x,y);require(a.alpha()==b.alpha(),"Filter changes alpha");if(!a.alpha())require(b.red()==0 && b.green()==0 && b.blue()==0,"Transparent filter leaves RGB halo");}require(difference(neutral,colored)>30,"Filter has no effect on translucent cell");}
		p.cellParty=true;p.cellCursed=false;p.cellHue=90;const auto partyOnly=draw();p.cellCursed=true;require(difference(partyOnly,draw())==0,"Party does not take filter precedence over cursed");
		p.cellParty=p.cellCursed=false;p.cellHue=0;require(difference(neutral,draw())==0,"Filter uniform leaks to neutral draw");p.cellPadding=float(chat::emotePadding(chat::Shake));p.scale=p.cellPadding;
		for(int i=0;i<10;++i){const auto e=chat::sampleEmote(chat::Shake,true,i*50);p.cellShiftX=e.shiftX;p.cellShiftY=e.shiftY;const auto shifted=draw();for(int y=0;y<100;++y)for(int x=0;x<100;++x){const auto c=shifted.pixelColor(x,y);require(c.alpha()<200,"Shake samples opaque neighboring atlas cell");if(!QRectF(10,10,56*p.scale,56*p.scale).contains(x,y))require(c.alpha()==0,"Shake exceeds allocated box");}}
		p.cellPadding=float(chat::emotePadding(chat::Shake|chat::Spin));p.scale=p.cellPadding;for(int i=0;i<10;++i){const auto e=chat::sampleEmote(chat::Shake|chat::Spin,true,i*50);p.cellShiftX=e.shiftX;p.cellShiftY=e.shiftY;p.cellAngle=e.angle;const auto shifted=draw();for(int y=0;y<100;++y)for(int x=0;x<100;++x){const auto c=shifted.pixelColor(x,y);require(c.alpha()<200,"Combined spin/shake samples neighbor");if(!QRectF(10,10,56*p.scale,56*p.scale).contains(x,y))require(c.alpha()==0,"Combined spin/shake exceeds box");}}
		const auto rejected=[&](DrawParams invalid){bool caught=false;try{renderer.renderOffscreen({100,100},[&]{renderer.draw(atlas,true,invalid);});}catch(const std::invalid_argument &){caught=true;}require(caught,"Invalid mood parameters accepted");};auto bad=p;bad.cellHue=std::numeric_limits<float>::quiet_NaN();rejected(bad);bad=p;bad.cellShiftX=2.1f;rejected(bad);bad=p;bad.sourceRect={};rejected(bad);
	}
    {
        const auto registry=presets::Registry::fromFile(OTO_PRESET_FILE);visual::AssetManager assets(QFileInfo(OTO_PRESET_FILE).dir().filePath("assets"));ObsVisualRenderer renderer;
        auto media=testing::bttvCache(engine);events::EventEngine events;events.start();const auto admitted=events.process({"gpu-wide","123",std::chrono::system_clock::now(),events::EventOrigin::Preview,testing::wideMessage()});require(bool(admitted),"GPU wide admission");const sources::ChatView view{{events::EventOrigin::Preview,"123"},1,{{admitted.event,{},1,1}}};
        for(const auto &definition:registry.entries())for(bool horizontal:{false,true}){
            if(!onlyStyle.isEmpty() && definition.id!=onlyStyle)continue;
            sources::ChatSettings skin;skin.presetId=definition.id;skin.style=*definition.chatStyle;skin.layout=horizontal?sources::ChatLayout::Horizontal:sources::ChatLayout::Vertical;skin.width=horizontal?1920:600;skin.height=horizontal?100:700;
            const auto art=skin.style.atlas.isEmpty()?visual::ImagePtr{}:assets.loadRegion(skin.style.atlas,skin.style.crop);auto frame=sources::prepareChatFrame(*engine,skin,view,{},start,false,media.get(),art);const auto prepared=frame;const auto &row=prepared->rows.back();require(row.emotes.size()==4,"Wide drops emotes");require(row.emotes[1].box.width()==4*row.emotes[1].box.height() && row.emotes[3].box.height()==4*row.emotes[3].box.width(),"GPU wide boxes wrong");
            const auto shot=[&](int ms){return renderer.renderOffscreen({skin.width+20,skin.height+20},[&]{drawChatFrame(renderer,*frame,start+std::chrono::milliseconds(ms),{10,10});});};
            const auto region=[&](int i){return QRectF(row.position+QPointF(10,10)+row.bodyBox.topLeft()+row.emotes[size_t(i)].box.topLeft()*row.bodyScale,row.emotes[size_t(i)].box.size()*row.bodyScale).toAlignedRect();};
            const auto first=shot(1500),later=shot(1875);require(difference(first.copy(region(1)),later.copy(region(1)))==0 && difference(first.copy(region(2)),later.copy(region(2)))>40,"Wide static/animated combination wrong");
            const auto uploads=renderer.stats().uploads,rasters=engine->stats().renders;for(int ms=0;ms<=1500;ms+=75){const auto image=shot(ms);for(int y=0;y<image.height();++y)for(int x=0;x<image.width();++x)if(!QRect(10,10,skin.width,skin.height).contains(x,y))require(image.constScanLine(y)[x*4+3]==0,"Wide outside source bounds");}require(uploads==renderer.stats().uploads && rasters==engine->stats().renders,"Wide uploads/rasterizes each tick");require(first.save(output+"/"+definition.id+(horizontal?"-horizontal-wide.png":"-vertical-wide.png")),"Save wide");
            skin.animatedEmotes=false;frame=sources::prepareChatFrame(*engine,skin,view,frame,start,false,media.get(),art);require(difference(shot(1500),shot(1875))==0,"Wide ignores animation OFF");require(difference(first.copy(region(1)),shot(1500).copy(region(1)))==0,"OFF removes wide");
            std::cout<<"PASS GPU wide "<<definition.id.toStdString()<<' '<<(horizontal?"horizontal":"vertical")<<": 4:1 stretch, quarter turn, party/shake, complete layout, guards, shared atlas, animation OFF, warm cache\n";
        }
        require(media->ingest(testing::layerBase,animatedChatFixture()),"Wide animated image fixture");sources::ChatSettings animated;animated.entryMs=0;auto animFrame=sources::prepareChatFrame(*engine,animated,view,{},start,false,media.get());const auto animShot=[&](int ms){return renderer.renderOffscreen({600,700},[&]{drawChatFrame(renderer,*animFrame,start+std::chrono::milliseconds(ms));});};const auto &we=animFrame->rows.back().emotes[1];const auto &wr=animFrame->rows.back();const auto area=QRectF(wr.position+wr.bodyBox.topLeft()+we.box.topLeft()*wr.bodyScale,we.box.size()*wr.bodyScale).toAlignedRect();require(difference(animShot(1000).copy(area),animShot(1100).copy(area))>40,"Wide image animation stopped");animated.animatedEmotes=false;animFrame=sources::prepareChatFrame(*engine,animated,view,animFrame,start,false,media.get());require(difference(animShot(1000),animShot(1100))==0,"Wide animated image ignores OFF");
        media->stop();events.stop();
        // An opaque adjacent atlas cell exposes edge leakage; a full rectangle proves corner retention.
        QImage image(112,56,QImage::Format_RGBA8888_Premultiplied);image.fill(QColor(0,0,255,255));for(int y=0;y<56;++y)for(int x=0;x<56;++x)image.setPixelColor(x,y,y<28?(x<28?QColor(255,255,0,128):QColor(255,0,0,128)):(x<28?QColor(0,255,0,128):QColor(0,0,255,128)));const auto atlas=engine->prepareRaster(image);DrawParams p;p.x=p.y=20;p.sourceRect={0,0,56,56};
        const auto set=[&](unsigned flags,int ms){const auto e=chat::sampleEmote(flags,true,ms);p.cellAspect=e.aspect;p.cellExtentX=e.extentX;p.cellExtentY=e.extentY;p.cellPadding=e.padding;p.cellAngle=e.angle;p.cellShiftX=e.shiftX;p.cellShiftY=e.shiftY;p.cellFlipX=bool(flags & chat::FlipX);p.cellParty=e.party;p.cellHue=e.hue;};
        const auto shot=[&]{return renderer.renderOffscreen({300,300},[&]{renderer.draw(atlas,true,p);});};set(chat::Wide,0);const auto plain=shot();require(plain.pixelColor(40,30).red()>220 && plain.pixelColor(200,30).green()<30,"Wide distorted source orientation");int baseCount=0;for(int y=0;y<300;++y)for(int x=0;x<300;++x)baseCount+=plain.pixelColor(x,y).alpha()>20;require(baseCount==56*56*4,"Wide does not occupy full 4:1 rectangle");
        set(chat::Wide|chat::FlipX,0);require(shot().pixelColor(40,30).green()<30,"Wide mirror wrong");set(chat::Wide|chat::TurnRight,0);require(shot().pixelColor(30,40).green()>220 && shot().pixelColor(30,40).red()<30,"Wide quarter turn wrong");
        for(unsigned flags:{unsigned(chat::Wide|chat::Spin),unsigned(chat::Wide|chat::Spin|chat::Shake),unsigned(chat::Wide|chat::Shake)})for(int ms=0;ms<1500;ms+=50){set(flags,ms);const auto out=shot();int count=0;for(int y=0;y<300;++y)for(int x=0;x<300;++x){const auto c=out.pixelColor(x,y);require(c.alpha()<180,"Wide samples neighboring atlas cell");count+=c.alpha()>20;if(!QRectF(20,20,56*p.cellExtentX,56*p.cellExtentY).contains(x+.5,y+.5))require(c.alpha()==0,"Wide exceeds reserved rectangle");}require(std::abs(count-baseCount)<500,"Wide rotation clips corners");}
        set(0,0);const auto reset=shot();require(reset.pixelColor(100,30).alpha()==0 && reset.pixelColor(30,30).red()>220,"Wide extent leaks to neutral draw");
        const auto rejected=[&](DrawParams invalid){bool caught=false;try{renderer.renderOffscreen({300,300},[&]{renderer.draw(atlas,true,invalid);});}catch(const std::invalid_argument &){caught=true;}require(caught,"Invalid wide extent accepted");};auto bad=p;bad.cellAspect=3;rejected(bad);bad=p;bad.cellExtentX=9;rejected(bad);bad=p;bad.cellExtentY=std::numeric_limits<float>::quiet_NaN();rejected(bad);bad=p;bad.cellAspect=4;bad.sourceRect={};rejected(bad);
    }

	{
	 const auto registry=presets::Registry::fromFile(OTO_PRESET_FILE);visual::AssetManager assets(QFileInfo(OTO_PRESET_FILE).dir().filePath("assets"));ObsVisualRenderer renderer;auto ffz=testing::ffzCursedCache(engine);events::EventEngine events;events.start();const auto event=events.process({"gpu-ffz-cursed","123",std::chrono::system_clock::now(),events::EventOrigin::Preview,testing::ffzCursedMessage()});require(bool(event),"GPU FFZ Cursed admission");const sources::ChatView view{{events::EventOrigin::Preview,"123"},1,{{event.event,{},1,1}}};
	 for(const auto &definition:registry.entries())for(bool horizontal:{false,true}){
	  if(!onlyStyle.isEmpty() && definition.id!=onlyStyle)continue;sources::ChatSettings skin;skin.presetId=definition.id;skin.style=*definition.chatStyle;skin.layout=horizontal?sources::ChatLayout::Horizontal:sources::ChatLayout::Vertical;skin.width=horizontal?1920:600;skin.height=horizontal?100:700;
	  const auto art=skin.style.atlas.isEmpty()?visual::ImagePtr{}:assets.loadRegion(skin.style.atlas,skin.style.crop);auto frame=sources::prepareChatFrame(*engine,skin,view,{},start,false,ffz.get(),art);const auto prepared=frame;const auto &row=prepared->rows.back();require(row.emotes.size()==4,"FFZ Cursed draws extra modifier icons");
	  const auto shot=[&](int ms){return renderer.renderOffscreen({skin.width+20,skin.height+20},[&]{drawChatFrame(renderer,*frame,start+std::chrono::milliseconds(ms),{10,10});});};const auto region=[&](int i){const auto &e=row.emotes[size_t(i)];return QRectF(row.position+QPointF(10,10)+row.bodyBox.topLeft()+e.box.topLeft()*row.bodyScale,e.box.size()*row.bodyScale).toAlignedRect();};
	  const auto first=shot(1000);require(difference(first,shot(1700))==0,"Static FFZ Cursed moves");for(int i=1;i<4;++i){const auto tile=first.copy(region(i));bool white=false,black=false;for(int y=3;y<tile.height()-3;++y)for(int x=3;x<tile.width()-3;++x){const auto c=tile.pixelColor(x,y);require(std::abs(c.red()-c.green())<=2 && std::abs(c.green()-c.blue())<=2,"FFZ Cursed is not grayscale");white=white || c.red()>200;black=black || c.red()<10;}require(white && black,"FFZ Cursed lacks contrast");require(row.emotes[size_t(i)].media==row.emotes[0].media,"FFZ Cursed duplicates atlas");}
	  const auto sample=[&](int i){const auto &e=row.emotes[size_t(i)];const auto point=row.position+QPointF(10,10)+row.bodyBox.topLeft()+(e.box.center()-QPointF(e.box.width()*.15,e.box.height()*.15))*row.bodyScale;return first.pixelColor(qRound(point.x()),qRound(point.y()));};require(sample(0).red()>220 && sample(0).green()>220 && sample(1).red()>220 && sample(2).red()<10 && sample(3).red()>100 && sample(3).red()<180,"FFZ Cursed mirror orientation wrong");
	  const auto uploads=renderer.stats().uploads,rasters=engine->stats().renders;for(int ms=0;ms<=1500;ms+=75){const auto image=shot(ms);for(int y=0;y<image.height();++y)for(int x=0;x<image.width();++x)if(!QRect(10,10,skin.width,skin.height).contains(x,y))require(image.constScanLine(y)[x*4+3]==0,"FFZ Cursed outside canvas");}require(uploads==renderer.stats().uploads && rasters==engine->stats().renders,"FFZ Cursed rasterizes/uploads per tick");require(first.save(output+"/"+definition.id+(horizontal?"-horizontal-ffz-cursed.png":"-vertical-ffz-cursed.png")),"Save FFZ Cursed");skin.animatedEmotes=false;frame=sources::prepareChatFrame(*engine,skin,view,frame,start,false,ffz.get(),art);require(difference(first,shot(1000))==0,"OFF removes static FFZ Cursed");
	  std::cout<<"PASS GPU FFZ Cursed "<<definition.id.toStdString()<<' '<<(horizontal?"horizontal":"vertical")<<": grayscale contrast, mirrors, complete message, source guards, shared atlas, OFF retained, warm cache\n";
	 }
	 // An animated image keeps its loop under the static filter; OFF stops it.
	 require(ffz->ingest(testing::layerBase,animatedChatFixture()),"Cursed GIF fixture");sources::ChatSettings settings;settings.entryMs=0;auto frame=sources::prepareChatFrame(*engine,settings,view,{},start,false,ffz.get());const auto shot=[&](int ms){return renderer.renderOffscreen({600,700},[&]{drawChatFrame(renderer,*frame,start+std::chrono::milliseconds(ms));});};require(difference(shot(1000),shot(1100))>40,"Cursed stops animated image");settings.animatedEmotes=false;frame=sources::prepareChatFrame(*engine,settings,view,frame,start,false,ffz.get());require(difference(shot(1000),shot(1100))==0,"Cursed GIF ignores OFF");ffz->stop();events.stop();
	}
 {
  const auto registry=presets::Registry::fromFile(OTO_PRESET_FILE);visual::AssetManager assets(QFileInfo(OTO_PRESET_FILE).dir().filePath("assets"));ObsVisualRenderer renderer;auto media=testing::fullFFZCache(engine);events::EventEngine e;e.start();const auto ev=e.process({"ffz-full-gpu","123",std::chrono::system_clock::now(),events::EventOrigin::Preview,testing::fullFFZMessage()});require(bool(ev),"FFZ full admission");const sources::ChatView view{{events::EventOrigin::Preview,"123"},1,{{ev.event,{},1,1}}};
  for(const auto &d:registry.entries())for(bool horizontal:{false,true}){if(!onlyStyle.isEmpty() && d.id!=onlyStyle)continue;sources::ChatSettings s;s.presetId=d.id;s.style=*d.chatStyle;s.entryMs=0;s.layout=horizontal?sources::ChatLayout::Horizontal:sources::ChatLayout::Vertical;s.width=horizontal?1920:600;s.height=horizontal?100:700;const auto art=s.style.atlas.isEmpty()?visual::ImagePtr{}:assets.loadRegion(s.style.atlas,s.style.crop);auto frame=sources::prepareChatFrame(*engine,s,view,{},start,false,media.get(),art);require(frame->rows.back().emotes.size()==14,"FFZ full loses icons");
   const auto shot=[&](int t){return renderer.renderOffscreen({s.width+20,s.height+20},[&]{drawChatFrame(renderer,*frame,start+std::chrono::milliseconds(t),{10,10});});};const auto first=shot(1000);require(difference(first,shot(1200))>20,"FFZ full effects static");const auto uploads=renderer.stats().uploads,rasters=engine->stats().renders;for(int t=0;t<6000;t+=125){const auto out=shot(t);for(int y=0;y<out.height();++y)for(int x=0;x<out.width();++x)if(!QRect(10,10,s.width,s.height).contains(x,y))require(out.constScanLine(y)[x*4+3]==0,"FFZ full outside source");}require(uploads==renderer.stats().uploads && rasters==engine->stats().renders,"FFZ full warm raster/upload");require(first.save(output+"/"+d.id+(horizontal?"-horizontal-ffz-full.png":"-vertical-ffz-full.png")),"Save FFZ full");s.animatedEmotes=false;frame=sources::prepareChatFrame(*engine,s,view,frame,start,false,media.get(),art);require(difference(shot(1000),shot(1700))==0,"FFZ full OFF");std::cout<<"PASS GPU FFZ complete "<<d.id.toStdString()<<' '<<(horizontal?"horizontal":"vertical")<<": filters, loops, scales, guards, complete layout, OFF, warm cache\n";
  }
  // Each effect must act independently; full-opacity neighbor detects atlas leakage.
  QImage source(112,56,QImage::Format_RGBA8888_Premultiplied);source.fill(Qt::white);for(int y=0;y<56;++y)for(int x=0;x<56;++x)source.setPixelColor(x,y,x<28?QColor(255,80,10,128):QColor(30,240,255,128));const auto atlas=engine->prepareRaster(source);
  for(const auto flags:testing::fullFlags){const unsigned effect=chat::ffzEffects(flags);DrawParams p;p.x=p.y=20;p.sourceRect={0,0,56,56};const auto set=[&](int t){const auto a=chat::sampleEmote(effect,true,t);p.cellPadding=a.padding;p.cellExtentX=a.extentX;p.cellExtentY=a.extentY;p.cellAspect=a.aspect;p.cellScaleX=a.scaleX;p.cellScaleY=a.scaleY;p.cellSlide=a.slide;p.cellAngle=a.angle;p.cellShiftX=a.shiftX;p.cellShiftY=a.shiftY;p.cellFilter=a.filter;p.cellHue=a.hue;};const auto shot=[&](int t){set(t);return renderer.renderOffscreen({300,300},[&]{renderer.draw(atlas,true,p);});};const auto first=shot(1000);bool changed=false;for(int t=0;t<6000;t+=113){const auto out=shot(t);changed=changed || difference(first,out)>10;for(int y=0;y<300;++y)for(int x=0;x<300;++x){const auto c=out.pixelColor(x,y);require(c.alpha()<180,"FFZ full samples neighbor");if(!QRectF(20,20,56*p.cellExtentX,56*p.cellExtentY).contains(x+.5,y+.5))require(c.alpha()==0,"FFZ full outside cell");}}if(effect & (chat::Slide|chat::Appear|chat::Leave|chat::Rainbow|chat::HyperShake|chat::Jam|chat::Bounce))require(changed,"FFZ individual loop static");}
  media->stop();e.stop();
 }

 {
  ObsVisualRenderer renderer;QImage source(112,56,QImage::Format_RGBA8888_Premultiplied);source.fill(Qt::white);for(int y=0;y<56;++y)for(int x=0;x<56;++x)source.setPixelColor(x,y,QColor(255,50,20,128));const auto atlas=engine->prepareRaster(source);DrawParams p;p.x=p.y=20;p.sourceRect={0,0,56,56};p.cellLayer={0,0,.625,.625};
  const auto shot=[&]{return renderer.renderOffscreen({100,100},[&]{renderer.draw(atlas,true,p);});};const auto out=shot();require(out.pixelColor(30,30).alpha()>100 && out.pixelColor(60,30).alpha()==0,"Accessory offset not placed");
  for(int angle=0;angle<360;angle+=15){p.cellAngle=float(angle);p.cellPadding=float(std::sqrt(2.));const auto moved=shot();for(int y=0;y<100;++y)for(int x=0;x<100;++x)require(moved.pixelColor(x,y).alpha()<180,"Accessory samples neighbor atlas");}
  p.cellAngle=0;p.cellPadding=1;p.cellLayer={0,0,1,1};require(shot().pixelColor(60,30).alpha()>100,"Accessory uniform not reset");
  p.cellLayer={0,0,0,1};bool rejected=false;try{shot();}catch(const std::invalid_argument &){rejected=true;}require(rejected,"Invalid accessory accepted");std::cout<<"PASS GPU accessory placement, shared transforms, atlas guards, neutral reset\n";
 }
 {
  const auto registry=presets::Registry::fromFile(OTO_PRESET_FILE);visual::AssetManager assets(QFileInfo(OTO_PRESET_FILE).dir().filePath("assets"));ObsVisualRenderer renderer;auto cheers=testing::cheerCache(engine);events::EventEngine events;events.start();const auto event=events.process({"gpu-cheermotes","123",std::chrono::system_clock::now(),events::EventOrigin::Preview,testing::cheerMessage()});require(bool(event),"Cheermote admission");const sources::ChatView view{{events::EventOrigin::Preview,"123"},1,{{event.event,{},1,1}}};
  for(const auto &d:registry.entries())for(bool horizontal:{false,true}){if(!onlyStyle.isEmpty() && d.id!=onlyStyle)continue;sources::ChatSettings s;s.presetId=d.id;s.style=*d.chatStyle;s.entryMs=0;s.thirdParty=false;s.layout=horizontal?sources::ChatLayout::Horizontal:sources::ChatLayout::Vertical;s.width=horizontal?1920:600;s.height=horizontal?100:700;const auto art=s.style.atlas.isEmpty()?visual::ImagePtr{}:assets.loadRegion(s.style.atlas,s.style.crop);auto frame=sources::prepareChatFrame(*engine,s,view,{},start,false,cheers.get(),art);require(frame->rows.back().emotes.size()==2 && !frame->rows.back().bodyPages.empty(),"Cheermotes or amounts lost");const auto shot=[&](int t){return renderer.renderOffscreen({s.width+20,s.height+20},[&]{drawChatFrame(renderer,*frame,start+std::chrono::milliseconds(t),{10,10});});};const auto first=shot(1000);require(difference(first,shot(1100))>20,"Cheermote loop static");const auto uploads=renderer.stats().uploads,rasters=engine->stats().renders;
   for(int t=1000;t<=1500;t+=50){const auto out=shot(t);for(int y=0;y<out.height();++y)for(int x=0;x<out.width();++x)if(!QRect(10,10,s.width,s.height).contains(x,y))require(out.constScanLine(y)[x*4+3]==0,"Cheermote outside source");}require(uploads==renderer.stats().uploads && rasters==engine->stats().renders,"Cheermote warm upload/raster");require(first.save(output+"/"+d.id+(horizontal?"-horizontal-cheermotes.png":"-vertical-cheermotes.png")),"Save cheer");s.animatedEmotes=false;frame=sources::prepareChatFrame(*engine,s,view,frame,start,false,cheers.get(),art);require(difference(shot(1000),shot(1100))==0,"Cheermote ignores OFF");std::cout<<"PASS GPU Cheermotes "<<d.id.toStdString()<<' '<<(horizontal?"horizontal":"vertical")<<": tier GIF, amount, guards, third-party OFF, animation OFF, warm cache\n";
  }cheers->stop();events.stop();
 }
	media.stop();core.stop();obs_shutdown();return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}

