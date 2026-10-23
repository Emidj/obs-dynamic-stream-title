// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include "chat-layer-checks.hpp"
namespace oto::testing {
inline QByteArray effectCatalog(){return R"({"default_sets":[1],"sets":{"1":{"emoticons":[{"name":"ffzX","modifier":true,"modifier_flags":3,"urls":{"2":"//cdn.frankerfacez.com/emote/720508/2"}},{"name":"ffzY","modifier":true,"modifier_flags":5,"urls":{"2":"//cdn.frankerfacez.com/emote/720509/2"}},{"name":"ffzSpin","modifier":true,"modifier_flags":129,"urls":{"2":"//cdn.frankerfacez.com/emote/723886/2"}},{"name":"Alias","modifier":true,"modifier_flags":131,"modifier_prefix":true,"urls":{"2":"//cdn.frankerfacez.com/emote/723886/2"}},{"name":"Unknown","modifier":true,"modifier_flags":1048577,"urls":{"2":"//cdn.frankerfacez.com/emote/unknown/2"}},{"name":"Ordinary","modifier":false,"modifier_flags":129,"urls":{"2":"//cdn.frankerfacez.com/emote/ordinary/2"}}]}}})";}
inline QByteArray effectPng(){QImage image(56,56,QImage::Format_RGBA8888_Premultiplied);for(int y=0;y<56;++y)for(int x=0;x<56;++x)image.setPixelColor(x,y,y<28?(x<28?Qt::yellow:Qt::red):(x<28?Qt::green:Qt::blue));QByteArray bytes;QBuffer b(&bytes);b.open(QIODevice::WriteOnly);image.save(&b,"PNG");return bytes;}
inline std::unique_ptr<chat::EmoteCache> effectCache(std::shared_ptr<visual::VisualEngine> visual,std::vector<QString> *requests=nullptr){
 auto result=std::make_unique<chat::EmoteCache>(visual,[requests](QString url,std::function<void(twitch::HttpReply)> done){if(requests)requests->push_back(url);if(url=="https://api.frankerfacez.com/v1/set/global")done({200,effectCatalog(),false});else if(url=="https://7tv.io/v3/emote-sets/global")done({200,R"({"emotes":[{"name":"Paint","data":{"flags":256,"host":{"url":"//cdn.7tv.app/emote/paint","files":[{"name":"2x.webp"}]}}}]})",false});else done({404,{},false});});result->ensureChannel("123");result->ingest(layerBase,effectPng());return result;
}
inline events::ChatEvent effectMessage(){events::ChatEvent message{{"viewer","Effects"},"Kappa ffzX Kappa ffzY Kappa ffzSpin"};message.fragments={{"Kappa","25",false},{" ffzX ",{},false},{"Kappa","25",false},{" ffzY ",{},false},{"Kappa","25",false},{" ffzSpin",{},false}};return message;}
inline void checkChatEffects(std::shared_ptr<visual::VisualEngine> visual){
 const auto check=[](bool ok,const char *m){if(!ok)throw std::runtime_error(m);};std::vector<QString> requests;auto media=effectCache(visual,&requests);auto message=effectMessage();const auto tokens=media->tokens(message,true);
 check(tokens.size()==5 && tokens[0].effects==chat::FlipX && tokens[2].effects==chat::FlipY && tokens[4].effects==chat::Spin,"FFZ effects do not bind across Twitch fragments");
 message.fragments.clear();message.message="Alias Kappa"; // No Kappa catalog: retain prefix as an ordinary icon.
 check(media->tokens(message,true)[0].effects==0,"Prefix without recognized base activated");
 message.fragments={{"Alias ",{},false},{"Kappa","25",false},{" ffzY ffzX ffzX end",{},false}};const auto combined=media->tokens(message,true);check(combined.size()==2 && combined[0].effects==(chat::FlipX|chat::FlipY|chat::Spin) && combined[1].text==" end","Prefix/postfix chain loses text, repeats XOR or adds cells");
 message.fragments={{"ffzX\n",{},false},{"Kappa","25",false},{"\nffzY",{},false}};const auto newline=media->tokens(message,true);check(newline.size()==5 && newline[2].effects==0 && newline[0].effects==0 && newline.back().effects==0,"Modifier crossed a line break");
 message.fragments={{"Kappa","25",false},{" text ffzSpin Unknown Ordinary",{},false}};const auto broken=media->tokens(message,true);check(broken[0].effects==0 && broken[2].effects==0 && broken[4].effects==0 && broken.back().effects==0,"Broken/unsupported/ordinary modifier activated");
 message.fragments={{"Kappa","25",false},{" Paint ffzSpin ffzX",{},false}};const auto stack=media->tokens(message,true);check(stack.size()==2 && stack[1].layerOf==0 && stack[0].effects==(chat::Spin|chat::FlipX) && stack[1].effects==stack[0].effects,"Base and overlay lost shared transform");
 check(media->tokens(effectMessage(),false).size()==6,"Third-party OFF consumed FFZ modifiers");
 const auto parsed=chat::parseCatalog(2,effectCatalog(),false);check(parsed.size()==6 && parsed[0].modifier && parsed[2].effects==chat::Spin && parsed[3].prefix && !parsed[4].modifier && !parsed[5].modifier,"FFZ metadata parsed loosely");
 const auto scoped=chat::parseCatalog(2,R"({"default_sets":[1539687],"sets":{"1539687":{"emoticons":[{"name":"ffzX","modifier":true,"modifier_flags":3,"urls":{"2":"https://cdn.frankerfacez.com/emote/720508/2"}}]},"1532818":{"emoticons":[{"name":"ffzSpin","modifier":true,"modifier_flags":129,"urls":{"2":"https://cdn.frankerfacez.com/emote/723886/2"}}]}}})",false);
 check(scoped.size()==1 && scoped[0].name=="ffzX","FFZ per-user spin leaked into default global catalog");
 events::EventEngine engine;engine.start();const auto event=engine.process({"effects","123",std::chrono::system_clock::now(),events::EventOrigin::Preview,effectMessage()});check(bool(event),"Effects admission");sources::ChatView view{{events::EventOrigin::Preview,"123"},1,{{event.event,{},1,1}}};const auto now=visual::MotionClock::now();
 for(bool horizontal:{false,true}){sources::ChatSettings settings;settings.width=horizontal?1920:600;settings.height=horizontal?100:700;settings.layout=horizontal?sources::ChatLayout::Horizontal:sources::ChatLayout::Vertical;
  const auto before=sources::prepareChatFrame(*visual,settings,view,{},now,false,media.get());const auto &row=before->rows.back();check(row.emotes.size()==3 && row.emotes[0].media==row.emotes[1].media && row.emotes[1].media==row.emotes[2].media,"Effects duplicate media or modifier icons");check(row.emotes[2].box.width()>row.emotes[0].box.width()*1.4,"Spin has no corner safety space");
  for(const auto &e:row.emotes)check(QRectF(QPointF{},row.bodyBox.size()).contains(QRectF(e.box.topLeft()*row.bodyScale,e.box.size()*row.bodyScale)),"Effect exceeds complete body");
  const auto renders=visual->stats().renders;check(sources::prepareChatFrame(*visual,settings,view,before,now+std::chrono::seconds(2),false,media.get())==before && visual->stats().renders==renders,"Effect rerasterizes in idle");
 }
 check(requests.size()==6,"Consumed modifiers trigger their own downloads");
 media->stop();
}
}
