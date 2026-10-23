// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include "chat-bttv-checks.hpp"
namespace oto::testing {
inline events::ChatEvent moodMessage(){events::ChatEvent m{{"viewer","Mood"},"p! Kappa s! Kappa c! Kappa p! s! Kappa"};m.fragments={{"p! ",{},false},{"Kappa","25",false},{" s! ",{},false},{"Kappa","25",false},{" c! ",{},false},{"Kappa","25",false},{" p! s! ",{},false},{"Kappa","25",false}};return m;}
inline void checkChatMood(std::shared_ptr<visual::VisualEngine> visual){
 const auto check=[](bool ok,const char *why){if(!ok)throw std::runtime_error(why);};auto media=bttvCache(visual);const auto tokens=media->tokens(moodMessage(),true);
 check(tokens.size()==7 && tokens[0].effects==chat::Party && tokens[2].effects==chat::Shake && tokens[4].effects==chat::Cursed && tokens[6].effects==(chat::Party|chat::Shake),"BTTV mood prefixes lost or create icons");check(media->tokens(moodMessage(),false).size()==8,"Third-party OFF consumes mood prefixes");
 auto m=moodMessage();m.fragments={{"p!\ns! text c!",{},false}};const auto orphan=media->tokens(m,true);QString text;for(const auto &t:orphan){check(t.effects==0,"Orphan mood active");text+=t.text;}check(text=="p!\ns! text c!","Orphan mood text changed");
 auto layers=effectCache(visual);m.fragments={{"p! s! ",{},false},{"Kappa","25",false},{" Paint",{},false}};const auto stack=layers->tokens(m,true);check(stack.size()==2 && stack[1].layerOf==0 && stack[0].effects==(chat::Party|chat::Shake) && stack[1].effects==stack[0].effects,"Mood layers lose shared filter/movement");layers->stop();
 const auto aliases=chat::parseCatalog(1,R"([{"id":"65cbe7dbaed093b2eaf87c65","code":"ColorAlias","modifier":true},{"id":"662953eeea369c0ece39eb93","code":"ShakeAlias","modifier":true},{"id":"6468f7acaee1f7f47567708e","code":"CursedAlias","modifier":true},{"id":"unknown","code":"UnknownMood","modifier":true},{"id":"65cbe7dbaed093b2eaf87c65","code":"OrdinaryMood","modifier":false}])",false);
 check(aliases.size()==5 && aliases[0].effects==chat::Party && aliases[1].effects==chat::Shake && aliases[2].effects==chat::Cursed && !aliases[3].modifier && !aliases[4].modifier,"BTTV mood metadata parsed loosely");
 const auto first=chat::sampleEmote(chat::Shake|chat::Party,true,0),last=chat::sampleEmote(chat::Shake|chat::Party,true,1499),loop=chat::sampleEmote(chat::Shake|chat::Party,true,1500),pause=chat::sampleEmote(chat::Shake|chat::Party|chat::Cursed,false,833);
 check(first.shiftX==0 && first.shiftY>0 && last.shiftX<0 && last.shiftY<0 && last.hue>359 && loop.hue==0 && loop.shiftY==first.shiftY,"Mood loop boundary or initial phase wrong");
 check(chat::sampleEmote(chat::Shake,true,49).shiftY==first.shiftY && chat::sampleEmote(chat::Shake,true,50).shiftX>0,"Shake does not step at 50ms");check(pause.shiftX==0 && pause.shiftY==0 && pause.hue==0 && pause.party && pause.cursed,"Animation OFF does not pause motion/keep static filter");
 events::EventEngine engine;engine.start();const auto event=engine.process({"mood","123",std::chrono::system_clock::now(),events::EventOrigin::Preview,moodMessage()});check(bool(event),"Mood admission");const sources::ChatView view{{events::EventOrigin::Preview,"123"},1,{{event.event,{},1,1}}};const auto now=visual::MotionClock::now();
 for(bool horizontal:{false,true}){sources::ChatSettings settings;settings.width=horizontal?1920:600;settings.height=horizontal?100:700;settings.layout=horizontal?sources::ChatLayout::Horizontal:sources::ChatLayout::Vertical;
  const auto frame=sources::prepareChatFrame(*visual,settings,view,{},now,false,media.get());const auto &row=frame->rows.back();check(row.emotes.size()==4 && row.emotes[1].box.width()>row.emotes[0].box.width(),"Shake has no safety space");for(const auto &e:row.emotes)check(e.media==row.emotes[0].media && QRectF(QPointF{},row.bodyBox.size()).contains(QRectF(e.box.topLeft()*row.bodyScale,e.box.size()*row.bodyScale)),"Mood media duplicate or outside body");
  const auto renders=visual->stats().renders;check(sources::prepareChatFrame(*visual,settings,view,frame,now+std::chrono::seconds(1),false,media.get())==frame && visual->stats().renders==renders,"Mood rerasterizes each tick");
  for(int ms=0;ms<1500;++ms){const auto pose=chat::sampleEmote(chat::Spin|chat::Shake,true,ms);const double margin=(pose.padding-std::sqrt(2.))/2;check(std::abs(pose.shiftX)<=margin+1e-6 && std::abs(pose.shiftY)<=margin+1e-6,"Spin plus shake lacks safe margin");}
 }media->stop();engine.stop();
}
}
