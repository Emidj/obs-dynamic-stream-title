// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include "chat-effect-checks.hpp"
#include <QJsonArray>
namespace oto::testing {
inline QByteArray ffzCursedCatalog(){
 auto root=QJsonDocument::fromJson(effectCatalog()).object();auto sets=root["sets"].toObject();auto set=sets["1"].toObject();auto emotes=set["emoticons"].toArray();
 const auto add=[&](QString name,QJsonValue modifier,int flags,bool prefix=false){emotes.append(QJsonObject{{"name",name},{"modifier",modifier},{"modifier_flags",flags},{"modifier_prefix",prefix},{"urls",QJsonObject{{"2","//cdn.frankerfacez.com/emote/720510/2"}}}});};
 add("ffzCursed",true,16385);add("CursedAlias",true,16386,true);add("UnknownCursed",true,16385|1048576);add("OrdinaryCursed",false,16385);add("TruthyCursed",1,16385);add("NameOnlyCursed",true,1);
 set["emoticons"]=emotes;sets["1"]=set;root["sets"]=sets;return QJsonDocument(root).toJson(QJsonDocument::Compact);
}
inline std::shared_ptr<chat::EmoteCache> ffzCursedCache(std::shared_ptr<visual::VisualEngine> visual,std::vector<QString> *requests=nullptr){
 auto media=std::make_shared<chat::EmoteCache>(visual,[requests](QString url,std::function<void(twitch::HttpReply)> done){if(requests)requests->push_back(url);if(url=="https://api.frankerfacez.com/v1/set/global")done({200,ffzCursedCatalog(),false});else if(url=="https://7tv.io/v3/emote-sets/global")done({200,R"({"emotes":[{"name":"Paint","data":{"flags":256,"host":{"url":"//cdn.7tv.app/emote/paint","files":[{"name":"2x.webp"}]}}}]})",false});else done({404,{},false});});media->ensureChannel("123");media->ingest(layerBase,effectPng());return media;
}
inline events::ChatEvent ffzCursedMessage(){events::ChatEvent m{{"viewer","FFZ_Cursed"},"Kappa Kappa ffzCursed Kappa ffzCursed ffzX Kappa ffzCursed ffzY end"};m.fragments={{"Kappa","25",false},{" ",{},false},{"Kappa","25",false},{" ffzCursed ",{},false},{"Kappa","25",false},{" ffzCursed ffzX ",{},false},{"Kappa","25",false},{" ffzCursed ffzY end",{},false}};return m;}
inline void checkFFZCursed(std::shared_ptr<visual::VisualEngine> visual){
 const auto check=[](bool ok,const char *why){if(!ok)throw std::runtime_error(why);};std::vector<QString> requests;auto media=ffzCursedCache(visual,&requests);const auto parsed=chat::parseCatalog(2,ffzCursedCatalog(),false);
 check(parsed.size()==12 && parsed[6].effects==chat::Cursed && !parsed[6].prefix && parsed[7].effects==(chat::Cursed|chat::FlipX) && parsed[7].prefix,"FFZ Cursed metadata or alias wrong");for(int i=8;i<12;++i)check(!parsed[size_t(i)].modifier && parsed[size_t(i)].effects==0,"FFZ Cursed accepts unknown or nonboolean flags");
 const auto tokens=media->tokens(ffzCursedMessage(),true);check(tokens.size()==8 && tokens[0].effects==0 && tokens[2].effects==chat::Cursed && tokens[4].effects==(chat::Cursed|chat::FlipX) && tokens[6].effects==(chat::Cursed|chat::FlipY) && tokens[7].text==" end","FFZ Cursed loses emotes, mirror or final text");
 check(media->tokens(ffzCursedMessage(),false)[3].text==" ffzCursed ","Third-party OFF consumes FFZ Cursed");
 auto m=ffzCursedMessage();m.fragments={{"CursedAlias ",{},false},{"Kappa","25",false},{" ffzCursed ffzCursed tail",{},false}};const auto chain=media->tokens(m,true);check(chain.size()==2 && chain[0].effects==(chat::Cursed|chat::FlipX) && chain[1].text==" tail","Repeated/prefix Cursed composition wrong");
 m.fragments={{"ffzCursed\n",{},false},{"Kappa","25",false},{"\nffzCursed",{},false}};const auto barrier=media->tokens(m,true);check(barrier.size()==5 && barrier[2].effects==0 && barrier[0].effects==0 && barrier[4].effects==0,"Cursed crosses line or applies without base");
 m.fragments={{"Kappa","25",false},{" Paint ffzCursed ffzX",{},false}};const auto layers=media->tokens(m,true);check(layers.size()==2 && layers[1].layerOf==0 && layers[0].effects==(chat::Cursed|chat::FlipX) && layers[1].effects==layers[0].effects,"FFZ Cursed stack loses shared filter");
 m.fragments={{"Kappa","25",false},{" UnknownCursed OrdinaryCursed TruthyCursed NameOnlyCursed",{},false}};for(const auto &t:media->tokens(m,true))check(t.effects==0,"Unsupported Cursed combination activated");
 auto collision=std::make_unique<chat::EmoteCache>(visual,[](QString url,std::function<void(twitch::HttpReply)> done){if(url=="https://7tv.io/v3/emote-sets/global")done({200,R"({"emotes":[{"name":"ffzCursed","data":{"host":{"url":"//cdn.7tv.app/emote/ordinary","files":[{"name":"2x.webp"}]}}}]})",false});else if(url=="https://api.frankerfacez.com/v1/set/global")done({200,ffzCursedCatalog(),false});else done({404,{},false});});collision->ensureChannel("123");check(collision->tokens(ffzCursedMessage(),true)[2].effects==0,"FFZ Cursed steals higher priority normal emote");collision->stop();
 events::EventEngine events;events.start();const auto event=events.process({"cursed","123",std::chrono::system_clock::now(),events::EventOrigin::Preview,ffzCursedMessage()});check(bool(event),"FFZ Cursed admission");const sources::ChatView view{{events::EventOrigin::Preview,"123"},1,{{event.event,{},1,1}}};const auto now=visual::MotionClock::now();
 for(bool horizontal:{false,true})for(bool compact:{false,true}){sources::ChatSettings settings;settings.width=compact?320:(horizontal?1920:600);settings.height=compact?100:(horizontal?100:700);settings.layout=horizontal?sources::ChatLayout::Horizontal:sources::ChatLayout::Vertical;
  const auto frame=sources::prepareChatFrame(*visual,settings,view,{},now,false,media.get());const auto &row=frame->rows.back();check(row.emotes.size()==4,"FFZ Cursed compact layout loses emotes");for(const auto &e:row.emotes)check(e.media==row.emotes[0].media && e.box.size()==row.emotes[0].box.size() && QRectF(QPointF{},row.bodyBox.size()).contains(QRectF(e.box.topLeft()*row.bodyScale,e.box.size()*row.bodyScale)),"Cursed duplicates image or changes geometry");const auto rasters=visual->stats().renders;check(sources::prepareChatFrame(*visual,settings,view,frame,now+std::chrono::seconds(1),false,media.get())==frame && visual->stats().renders==rasters,"Cursed rerasterizes idle");settings.animatedEmotes=false;const auto paused=sources::prepareChatFrame(*visual,settings,view,frame,now,false,media.get());check(paused->rows.back().emotes[1].effects==chat::Cursed,"OFF removes static Cursed");
 }
 check(requests.size()==6,"Consumed Cursed downloads a separate icon");media->stop();events.stop();
}
}
