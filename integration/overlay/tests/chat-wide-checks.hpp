// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include "chat-mood-checks.hpp"
namespace oto::testing {
inline events::ChatEvent wideMessage(){events::ChatEvent m{{"viewer","Wide"},"Kappa w! Kappa w! p! s! Kappa w! r! Kappa end"};m.fragments={{"Kappa","25",false},{" w! ",{},false},{"Kappa","25",false},{" w! p! s! ",{},false},{"Kappa","25",false},{" w! r! ",{},false},{"Kappa","25",false},{" end",{},false}};return m;}
inline void checkChatWide(std::shared_ptr<visual::VisualEngine> visual){
 const auto check=[](bool ok,const char *why){if(!ok)throw std::runtime_error(why);};auto media=bttvCache(visual);const auto tokens=media->tokens(wideMessage(),true);
 check(tokens.size()==8 && tokens[0].effects==0 && tokens[2].effects==chat::Wide && tokens[4].effects==(chat::Wide|chat::Party|chat::Shake) && tokens[6].effects==(chat::Wide|chat::TurnRight) && tokens[7].text==" end","Wide composition loses trailing text or emotes");
 const auto disabled=media->tokens(wideMessage(),false);check(disabled.size()==8 && disabled[1].text==" w! " && disabled[2].effects==0,"Wide disabled consumes text");
 auto m=wideMessage();m.fragments={{"w! w! ",{},false},{"Kappa","25",false},{" tail",{},false}};const auto repeated=media->tokens(m,true);check(repeated.size()==2 && repeated[0].effects==chat::Wide,"Wide repetitions multiply size");
 m.fragments={{"w! text\nw!",{},false}};QString retained;for(const auto &t:media->tokens(m,true)){check(t.effects==0,"Wide orphan active");retained+=t.text;}check(retained=="w! text\nw!","Wide barrier text changed");
 const auto aliases=chat::parseCatalog(1,R"([{"id":"64e3b31920cb0d25d950a9f9","code":"WideAlias","modifier":true},{"id":"64e3b31920cb0d25d950a9f9","code":"OrdinaryWide","modifier":false},{"id":"unknown","code":"w!","modifier":true}])",false);check(aliases.size()==3 && aliases[0].effects==chat::Wide && aliases[0].prefix && !aliases[1].modifier && !aliases[2].modifier,"Wide alias metadata loose");
 auto collision=std::make_unique<chat::EmoteCache>(visual,[](QString url,std::function<void(twitch::HttpReply)> done){if(url=="https://7tv.io/v3/emote-sets/global")done({200,R"({"emotes":[{"name":"w!","data":{"host":{"url":"//cdn.7tv.app/emote/collision","files":[{"name":"2x.webp"}]}}}]})",false});else done({404,{},false});});collision->ensureChannel("123");const auto ordinary=collision->tokens(wideMessage(),true);check(ordinary[2].effects==0 && ordinary[2].text=="w!" && !ordinary[2].modifier && ordinary[4].effects==0,"Wide steals ordinary catalog emote");collision->stop();
 auto layers=layerCache(visual);m.fragments={{"w! ",{},false},{"Kappa","25",false},{" Rain Spark tail",{},false}};const auto stack=layers->tokens(m,true);check(stack.size()==4 && stack[1].layerOf==0 && stack[2].layerOf==0 && stack[2].effects==chat::Wide,"Wide stack effects lost");
 events::EventEngine engine;engine.start();const auto event=engine.process({"wide","123",std::chrono::system_clock::now(),events::EventOrigin::Preview,wideMessage()});check(bool(event),"Wide admission");const sources::ChatView view{{events::EventOrigin::Preview,"123"},1,{{event.event,{},1,1}}};const auto now=visual::MotionClock::now();
 for(bool horizontal:{false,true})for(bool compact:{false,true}){
  sources::ChatSettings settings;settings.width=compact?320:(horizontal?1920:600);settings.height=compact?100:(horizontal?100:700);settings.layout=horizontal?sources::ChatLayout::Horizontal:sources::ChatLayout::Vertical;
  const auto frame=sources::prepareChatFrame(*visual,settings,view,{},now,false,media.get());const auto &row=frame->rows.back();check(row.emotes.size()==4,"Wide compact layout drops emote");check(row.emotes[1].box.width()==4*row.emotes[1].box.height() && row.emotes[3].box.height()==4*row.emotes[3].box.width(),"Wide aspect or quarter turn wrong");
  for(const auto &e:row.emotes)check(e.media==row.emotes.front().media && QRectF(QPointF{},row.bodyBox.size()).contains(QRectF(e.box.topLeft()*row.bodyScale,e.box.size()*row.bodyScale)),"Wide outside body or duplicates media");
  const auto rasters=visual->stats().renders;check(sources::prepareChatFrame(*visual,settings,view,frame,now+std::chrono::seconds(1),false,media.get())==frame && visual->stats().renders==rasters,"Wide reflows every tick");
 }
 const auto spin=chat::emoteExtent(chat::Wide|chat::Spin|chat::Shake);for(int ms=0;ms<1500;++ms){const auto s=chat::sampleEmote(chat::Wide|chat::Spin|chat::Shake,true,ms);const double r=s.angle*3.141592653589793/180;check((4*std::abs(std::cos(r))+std::abs(std::sin(r)))/2+std::abs(s.shiftX)<=spin[0]/2+1e-5 && (4*std::abs(std::sin(r))+std::abs(std::cos(r)))/2+std::abs(s.shiftY)<=spin[1]/2+1e-5,"Wide spin corner or shake clipped");}
 m.message="w! Kappa Rain Spark tail";const auto layered=engine.process({"wide-layers","123",std::chrono::system_clock::now(),events::EventOrigin::Preview,m});check(bool(layered),"Wide layer admission rejected");const sources::ChatView lv{{events::EventOrigin::Preview,"123"},1,{{layered.event,{},1,1}}};const auto lf=sources::prepareChatFrame(*visual,{},lv,{},now,false,layers.get());check(lf->rows.back().emotes.size()==3 && lf->rows.back().emotes[0].box==lf->rows.back().emotes[1].box && lf->rows.back().emotes[0].box==lf->rows.back().emotes[2].box,"Wide layers do not share rectangle");layers->stop();media->stop();engine.stop();
}
}


