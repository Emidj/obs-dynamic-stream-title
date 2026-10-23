// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include "chat/personal-catalog.hpp"
#include <QCoreApplication>
#include <stdexcept>
namespace oto::testing {
inline void checkPersonalCatalog(const std::shared_ptr<visual::VisualEngine> &visual){
 int requests=0;auto fetch=[&](QString url,std::function<void(twitch::HttpReply)> done){++requests;
  if(url.endsWith("/user/id/77"))done({200,R"({"user":{"twitch_id":77,"emote_sets":[1]},"sets":{"1":{"emoticons":[{"name":"MyOnly","urls":{"2":"https://cdn.frankerfacez.com/emote/1/2"}}]},"2":{"emoticons":[{"name":"NotMine","urls":{"2":"https://cdn.frankerfacez.com/emote/2/2"}}]}}})",false});
  else if(url.endsWith("/users/twitch/77"))done({200,R"({"id":"77","platform":"TWITCH","user":{"id":"123456789012345678901234","emote_sets":[{"id":"223456789012345678901234","flags":4},{"id":"323456789012345678901234","flags":0}]},"emote_set":{"emotes":[{"name":"ChannelOnly"}]}})",false});
  else if(url.endsWith("/emote-sets/223456789012345678901234"))done({200,R"({"id":"223456789012345678901234","flags":4,"owner":{"id":"123456789012345678901234"},"emotes":[{"name":"PersonalSeven","data":{"flags":0,"host":{"url":"https://cdn.7tv.app/emote/fixture","files":[{"name":"2x.webp"}]}}}]})",false});
  else done({404,{},false});
 };
 chat::EmoteCache cache(visual,fetch);cache.ensureChannel("123");cache.ensureAuthor("77");const int loaded=requests;cache.ensureAuthor("77");if(requests!=loaded)throw std::runtime_error("Personal catalogs repeated per message");
 events::ChatEvent event{{"77","Owner"},"MyOnly NotMine PersonalSeven ChannelOnly"};const auto tokens=cache.tokens(event,true);int images=0;for(const auto &t:tokens)images+=!t.url.isEmpty();if(images!=2)throw std::runtime_error("Personal assignment or set type wrong");
 event.author.id="78";for(const auto &t:cache.tokens(event,true))if(!t.url.isEmpty())throw std::runtime_error("Personal emote leaked to another author");
 if(!cache.ingestPersonalBTTV(R"({"name":"lookup_user","data":{"providerId":"77","pro":true,"emotes":[{"id":"fixture","code":"BTTVMine","animated":true}]}})"))throw std::runtime_error("BTTV personal broadcast rejected");
 event.author.id="77";event.message="BTTVMine";if(cache.tokens(event,true)[0].url.isEmpty())throw std::runtime_error("BTTV personal missing");if(!cache.tokens(event,false)[0].url.isEmpty())throw std::runtime_error("Personal ignores third-party OFF");
 cache.setCatalogActive(false);if(cache.ingestPersonalBTTV(R"({"name":"lookup_user","data":{"providerId":"77","pro":false,"emotes":[]}})"))throw std::runtime_error("Hidden personal feed still active");
 cache.ensureChannel("456");cache.setCatalogActive(true);event.message="MyOnly BTTVMine PersonalSeven";for(const auto &t:cache.tokens(event,true))if(!t.url.isEmpty())throw std::runtime_error("Personal epoch leaked to new channel");cache.stop();
 std::vector<std::function<void(twitch::HttpReply)>> pending;chat::PersonalCatalog delayed([&](QString,std::function<void(twitch::HttpReply)> f){pending.push_back(f);},std::chrono::steady_clock::now,[]{});delayed.select("123",true);for(int i=1;i<=80;++i)delayed.ensure(QString::number(i));if(pending.size()!=2)throw std::runtime_error("Personal concurrency not bounded");delayed.select("456",false);for(auto &f:pending)f({200,R"({"user":{"twitch_id":1,"emote_sets":[]},"sets":{}})",false});if(delayed.find("1","MyOnly"))throw std::runtime_error("Late personal response accepted");delayed.stop();QCoreApplication::processEvents();
 // Keep the complete previous assignment if one set fails during a refresh.
 auto now=std::chrono::steady_clock::now();bool failRefresh=false;
 chat::PersonalCatalog atomic([&](QString url,std::function<void(twitch::HttpReply)> done){
  if(url.endsWith("/users/twitch/77"))done({200,R"({"id":"77","platform":"TWITCH","user":{"id":"123456789012345678901234","emote_sets":[{"id":"223456789012345678901234","flags":4},{"id":"323456789012345678901234","flags":4}]}})",false});
  else if(url.endsWith("/emote-sets/223456789012345678901234"))done({200,R"({"id":"223456789012345678901234","flags":4,"owner":null,"emotes":[{"name":"First","data":{"flags":0,"host":{"url":"https://cdn.7tv.app/emote/fixture","files":[{"name":"2x.webp"}]}}}]})",false});
  else if(url.endsWith("/emote-sets/323456789012345678901234")){
   if(failRefresh)done({503,{},false});else done({200,R"({"id":"323456789012345678901234","flags":4,"owner":null,"emotes":[{"name":"Second","data":{"flags":0,"host":{"url":"https://cdn.7tv.app/emote/fixture","files":[{"name":"2x.webp"}]}}}]})",false});
  }else done({404,{},false});
 },[&]{return now;},[]{});
 atomic.select("123",true);atomic.ensure("77");if(!atomic.find("77","First") || !atomic.find("77","Second"))throw std::runtime_error("Multi-set personal assignment missing");
 failRefresh=true;now+=std::chrono::minutes(6);atomic.ensure("77");if(!atomic.find("77","First") || !atomic.find("77","Second"))throw std::runtime_error("Partial refresh erased personal assignment");atomic.stop();
}
}
