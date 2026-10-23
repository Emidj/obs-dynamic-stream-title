// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include "chat/emote-cache.hpp"
#include "sources/chat-model.hpp"
#include "chat-fixtures.hpp"
#include <QBuffer>
namespace oto::testing {
inline const QString layerBase="https://static-cdn.jtvnw.net/emoticons/v2/25/static/dark/2.0";
inline const QString layerRain="https://cdn.7tv.app/emote/rain/2x.webp";
inline const QString layerSpark="https://cdn.7tv.app/emote/spark/2x.webp";
inline QByteArray layerPng(QColor color,bool stripe=false){
	QImage image(56,56,QImage::Format_RGBA8888_Premultiplied);image.fill(Qt::transparent);
	for(int y=0;y<56;++y)for(int x=0;x<(stripe?14:56);++x)image.setPixelColor(x,y,color);
	QByteArray bytes;QBuffer buffer(&bytes);buffer.open(QIODevice::WriteOnly);image.save(&buffer,"PNG");return bytes;
}
inline QByteArray layerAnimation(){auto bytes=animatedChatFixture();const auto control=bytes.indexOf(QByteArray::fromHex("21f90400"));bytes[control+3]=char(1);return bytes;}
inline std::unique_ptr<chat::EmoteCache> layerCache(std::shared_ptr<visual::VisualEngine> visual,bool ready=true){
	auto media=std::make_unique<chat::EmoteCache>(visual,[](QString url,std::function<void(twitch::HttpReply)> done){
		if(url=="https://7tv.io/v3/emote-sets/global")done({200,R"({"emotes":[{"name":"Rain","data":{"flags":256,"host":{"url":"//cdn.7tv.app/emote/rain","files":[{"name":"2x.webp"}]}}},{"name":"Spark","data":{"flags":257,"host":{"url":"//cdn.7tv.app/emote/spark","files":[{"name":"2x.webp"}]}}}]})",false});
		else if(url=="https://api.betterttv.net/3/cached/emotes/global")done({200,R"([{"code":"Collision","id":"bttv"},{"code":"SoSnowy","id":"567b5b520e984428652809b6"},{"code":"Transform","id":"transform","modifier":true}])",false});
		else if(url=="https://7tv.io/v3/users/twitch/123")done({200,R"({"emote_set":{"emotes":[{"name":"Collision","flags":256,"data":{"flags":0,"host":{"url":"//cdn.7tv.app/emote/channel","files":[{"name":"2x.webp"}]}}}]}})",false});
		else done({404,{},false});
	});media->ensureChannel("123");
	if(ready){media->ingest(layerBase,layerPng(QColor(0,255,0)));media->ingest(layerRain,layerAnimation());media->ingest(layerSpark,layerPng(QColor(255,255,0),true));}return media;
}
inline events::ChatEvent layerMessage(){events::ChatEvent c{{"viewer","Layers"},"Kappa Rain Spark end Kappa"};c.fragments={{"Kappa","25",false},{" Rain Spark end ",{},false},{"Kappa","25",false}};return c;}
inline void checkChatLayers(std::shared_ptr<visual::VisualEngine> visual){
	const auto check=[](bool ok,const char *message){if(!ok)throw std::runtime_error(message);};
	auto media=layerCache(visual,false);auto c=layerMessage();const auto tokens=media->tokens(c,true);
	check(tokens.size()==5 && tokens[1].layerOf==0 && tokens[2].layerOf==0 && tokens[3].text==" end " && tokens[4].layerOf==-1,"Layer chain loses text, separators or base");
	c.fragments.clear();c.message="Rain before Kappa\nSpark";auto orphan=media->tokens(c,true);check(orphan.front().layerOf==-1 && orphan.back().layerOf==-1,"Orphan/newline layer attached");
	c.message="Collision SoSnowy Transform";const auto resolved=media->tokens(c,true);check(resolved[0].url.contains("channel") && !resolved[0].zeroWidth && resolved[1].layerOf==0 && !resolved.back().zeroWidth,"Provider priority, v3 flags or BTTV transforms confused");
	check(media->tokens(c,false).size()==1 && media->tokens(c,false)[0].text==c.message.c_str(),"Third-party OFF interprets layers");
	const auto ffz=chat::parseCatalog(2,R"({"default_sets":[1],"sets":{"1":{"emoticons":[{"name":"ffzSpin","modifier":true,"modifier_flags":129,"urls":{"2":"//cdn.frankerfacez.com/emote/723886/2"}}]}}})",false);
	check(ffz.size()==1 && !ffz.front().zeroWidth,"FFZ transform incorrectly treated as image overlay");
	events::EventEngine events;events.start();const auto admitted=events.process({"layers","123",std::chrono::system_clock::now(),events::EventOrigin::Preview,layerMessage()});
	check(bool(admitted),"Layer event normalization");sources::ChatView view{{events::EventOrigin::Preview,"123"},1,{{admitted.event,{},1,1}}};
	sources::ChatSettings settings;settings.messages=1;const auto now=visual::MotionClock::now();auto pending=sources::prepareChatFrame(*visual,settings,view,{},now,false,media.get());
	check(pending->rows.back().emotes.size()==4 && !pending->rows.back().emotes[1].media,"Pending base layer visible");
	check(media->ingest(layerRain,layerAnimation()),"Layer animation ingest");media->setChanged({}); // Ingest is deliberately a low-level fixture hook.
	// A changed setting forces preparation, matching a cache notify in production.
	settings.messageSize=23;auto early=sources::prepareChatFrame(*visual,settings,view,pending,now,false,media.get());check(!early->rows.back().emotes[1].media,"Layer painted over missing base fallback");
	check(media->ingest(layerBase,layerPng(QColor(0,255,0))) && media->ingest(layerSpark,layerPng(QColor(255,255,0),true)),"Layer PNG ingest");
	for(bool horizontal:{false,true}){settings.layout=horizontal?sources::ChatLayout::Horizontal:sources::ChatLayout::Vertical;settings.width=horizontal?1920:320;settings.height=horizontal?100:700;
		auto frame=sources::prepareChatFrame(*visual,settings,view,{},now,false,media.get());const auto &row=frame->rows.back();
		check(row.emotes.size()==4 && row.emotes[0].box==row.emotes[1].box && row.emotes[1].box==row.emotes[2].box && !row.emotes[0].box.intersects(row.emotes[3].box),"Stack layout advances or misaligns");
		check(row.emotes[0].media && row.emotes[1].media && row.emotes[2].media && row.emotes[0].media==row.emotes[3].media,"Stack images not shared");
		for(const auto &emote:row.emotes)check(QRectF(QPointF{},row.bodyBox.size()).contains(QRectF(emote.box.topLeft()*row.bodyScale,emote.box.size()*row.bodyScale)),"Layer escapes complete body");
		const auto count=visual->stats().renders;check(sources::prepareChatFrame(*visual,settings,view,frame,now+std::chrono::seconds(5),false,media.get())==frame && count==visual->stats().renders,"Layer stack rerasterizes in idle");
	}media->stop();events.stop();
}
}
