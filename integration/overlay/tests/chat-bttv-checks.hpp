// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include "chat-effect-checks.hpp"
namespace oto::testing {
inline events::ChatEvent bttvMessage(){events::ChatEvent m{{"viewer","BTTV"},"h! Kappa v! Kappa l! Kappa r! Kappa"};m.fragments={{"h! ",{},false},{"Kappa","25",false},{" v! ",{},false},{"Kappa","25",false},{" l! ",{},false},{"Kappa","25",false},{" r! ",{},false},{"Kappa","25",false}};return m;}
inline std::unique_ptr<chat::EmoteCache> bttvCache(std::shared_ptr<visual::VisualEngine> engine){auto media=std::make_unique<chat::EmoteCache>(engine,[](QString,std::function<void(twitch::HttpReply)> done){done({404,{},false});});media->ensureChannel("123");media->ingest(layerBase,effectPng());return media;}
inline void checkBTTVEffects(std::shared_ptr<visual::VisualEngine> engine){
 const auto check=[](bool v,const char *m){if(!v)throw std::runtime_error(m);};auto media=bttvCache(engine);auto m=bttvMessage();const auto tokens=media->tokens(m,true);
 check(tokens.size()==7 && tokens[0].effects==chat::FlipX && tokens[2].effects==chat::FlipY && tokens[4].effects==chat::TurnLeft && tokens[6].effects==chat::TurnRight,"BTTV prefixes require menu images or bind to wrong fragment");
 check(media->tokens(m,false).size()==8,"BTTV disabled consumes prefixes");
 m.fragments={{"h! text ",{},false},{"Kappa","25",false},{" r!",{},false}};const auto barrier=media->tokens(m,true);check(barrier.size()==5 && barrier[2].effects==0 && barrier[0].text=="h!" && barrier.back().text=="r!","BTTV orphan or text barrier is lost");
 m.fragments={{"h!\nv!\r",{},false},{"Kappa","25",false}};const auto newline=media->tokens(m,true);check(newline.size()==5 && newline.back().effects==0,"BTTV prefix crosses newline");
 m.fragments={{"h! h! l! r! ",{},false},{"Kappa","25",false},{" tail",{},false}};const auto chain=media->tokens(m,true);check(chain.size()==2 && chain[0].effects==(chat::FlipX|chat::TurnLeft|chat::TurnRight) && chain[1].text==" tail","BTTV chained prefixes lose text or repeat XOR");
 const auto aliases=chat::parseCatalog(1,R"([{"id":"6468f845aee1f7f47567709b","code":"MirrorAlias","modifier":true},{"id":"6468f89caee1f7f4756770c2","code":"VerticalAlias","modifier":true},{"id":"6468f869aee1f7f4756770a8","code":"LeftAlias","modifier":true},{"id":"6468f883aee1f7f4756770b5","code":"RightAlias","modifier":true},{"id":"unknown","code":"p!","modifier":true},{"id":"6468f845aee1f7f47567709b","code":"Ordinary","modifier":false}])",false);
 check(aliases.size()==6 && aliases[0].effects==chat::FlipX && aliases[1].effects==chat::FlipY && aliases[2].effects==chat::TurnLeft && aliases[3].effects==chat::TurnRight && aliases[0].prefix && !aliases[4].modifier && !aliases[5].modifier,"BTTV IDs/aliases or modifier metadata parsed loosely");
 auto collision=std::make_unique<chat::EmoteCache>(engine,[](QString u,std::function<void(twitch::HttpReply)> done){if(u=="https://7tv.io/v3/emote-sets/global")done({200,R"({"emotes":[{"name":"h!","data":{"host":{"url":"//cdn.7tv.app/emote/collision","files":[{"name":"2x.webp"}]}}}]})",false});else done({404,{},false});});collision->ensureChannel("123");m=bttvMessage();const auto real=collision->tokens(m,true);check(!real[0].modifier && !real[0].url.isEmpty() && real[2].effects==0,"BTTV prefix steals ordinary higher-priority emote");collision->stop();media->stop();
}
}
