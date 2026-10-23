// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include "chat-layer-checks.hpp"
namespace oto::testing {
inline void checkCatalogRefresh(std::shared_ptr<visual::VisualEngine> visual){
 const auto check=[](bool ok,const char *message){if(!ok)throw std::runtime_error(message);};
 using Time=std::chrono::steady_clock::time_point;auto now=Time{}+std::chrono::hours(2);
 const QString seven="https://7tv.io/v3/users/twitch/123",bttv="https://api.betterttv.net/3/cached/users/twitch/123",global="https://7tv.io/v3/emote-sets/global";
 const QByteArray a=R"({"emote_set":{"emotes":[{"name":"Live","data":{"flags":0,"host":{"url":"//cdn.7tv.app/emote/one","files":[{"name":"2x.webp"}]}}}]}})";
 const QByteArray b=R"({"emote_set":{"emotes":[{"name":"Alias","data":{"flags":256,"host":{"url":"//cdn.7tv.app/emote/one","files":[{"name":"2x.webp"}]}}}]}})";
 const auto empty=[](const QString &url){if(url.contains("7tv"))return url.endsWith("global")?QByteArray(R"({"emotes":[]})"):QByteArray(R"({"emote_set":null})");if(url.contains("betterttv"))return url.endsWith("global")?QByteArray("[]"):QByteArray(R"({"channelEmotes":[],"sharedEmotes":[]})");return QByteArray(R"({"sets":{},"default_sets":[]})");};
 for(int p=0;p<3;++p){const QString url=p==0?seven:p==1?bttv:QString("https://api.frankerfacez.com/v1/room/id/123");check(bool(chat::parseCatalogSnapshot(p,empty(url),true)),"Valid empty provider snapshot rejected");check(!chat::parseCatalogSnapshot(p,"{}",true),"Malformed provider snapshot accepted");check(!chat::parseCatalogSnapshot(p,"not-json",false),"Broken JSON accepted");}
 std::map<QString,twitch::HttpReply> replies;std::map<QString,int> counts;
 replies[seven]={200,a,false};replies[bttv]={200,R"({"channelEmotes":[{"code":"Live","id":"fallback"}],"sharedEmotes":[]})",false};
 chat::EmoteCache media(visual,[&](QString url,std::function<void(twitch::HttpReply)> done){++counts[url];const auto it=replies.find(url);done(it==replies.end()?twitch::HttpReply{200,empty(url),false}:it->second);},[&]{return now;});
 media.ensureChannel("123");check(counts.size()==6,"Initial provider requests missing");const auto revision=media.revision();media.ensureChannel("123");media.refreshCatalogs();check(counts[seven]==1,"Clone duplicates catalog request");
 events::ChatEvent message{{"viewer","Viewer"},"Live"};check(media.tokens(message,true)[0].url.contains("/one/"),"Provider priority lost");
 check(media.ingest("https://cdn.7tv.app/emote/one/2x.webp",layerPng(Qt::green)),"Catalog media fixture");
 const state::ChannelKey key{events::EventOrigin::Twitch,"123"};events::EventEngine engine;engine.start();auto event=engine.process({"refresh-row","123",std::chrono::system_clock::now(),key.origin,message});
 check(bool(event),"Catalog event fixture");sources::ChatView view{key,1,{{event.event,{},1,1}}};sources::ChatSettings settings;
 auto before=sources::prepareChatFrame(*visual,settings,view,{},now,true,&media);const auto entered=before->rows.front().entered;
 now+=std::chrono::seconds(299);media.refreshCatalogs();check(counts[seven]==1,"Channel TTL too short");now+=std::chrono::seconds(1);media.refreshCatalogs();check(counts[seven]==2 && counts[global]==1 && media.revision()==revision,"Unchanged snapshot invalidates render or refreshes globals early");
 auto stable=sources::prepareChatFrame(*visual,settings,view,before,now,false,&media);check(stable==before,"Unchanged catalog rebuilt frame");
 replies[seven]={200,b,false};now+=std::chrono::seconds(300);media.refreshCatalogs();check(media.tokens(message,true)[0].url.contains("fallback"),"Removed provider name does not expose fallback");
 message.message="Alias";check(media.tokens(message,true)[0].zeroWidth,"Alias/layer metadata not updated");
 auto changed=sources::prepareChatFrame(*visual,settings,view,before,now,false,&media);check(changed!=before && changed->rows.front().entered==entered && changed->mediaClock==before->mediaClock,"Catalog update replays entrance or animation clock");
 const auto validRevision=media.revision();replies[seven]={429,{},false};now+=std::chrono::seconds(300);media.refreshCatalogs();check(media.tokens(message,true)[0].zeroWidth && media.revision()==validRevision,"Failure erased previous provider snapshot");
 const int failedCount=counts[seven];now+=std::chrono::seconds(4);media.refreshCatalogs();check(counts[seven]==failedCount,"Failure retries too soon");now+=std::chrono::seconds(1);media.refreshCatalogs();check(counts[seven]==failedCount+1,"First retry missing");now+=std::chrono::seconds(9);media.refreshCatalogs();check(counts[seven]==failedCount+1,"Exponential retry missing");
 replies[seven]={200,"{}",false};now+=std::chrono::seconds(1);media.refreshCatalogs();check(media.tokens(message,true)[0].zeroWidth,"Malformed refresh erased data");
 replies[seven]={200,empty(seven),false};now+=std::chrono::seconds(20);media.refreshCatalogs();check(media.tokens(message,true)[0].url.isEmpty(),"Valid empty catalog cannot remove old emotes");
 media.setCatalogActive(false);const auto hiddenCounts=counts;now+=std::chrono::hours(2);media.refreshCatalogs();media.ensureChannel("123");check(counts==hiddenCounts && !media.requestCatalogRefresh(),"Hidden source continues polling");media.setCatalogActive(true);check(counts[global]==2 && counts[seven]>hiddenCounts.at(seven),"Reactivation does not refresh expired catalogs");
 check(media.requestCatalogRefresh(),"Manual refresh unavailable");const auto manualCounts=counts;check(!media.requestCatalogRefresh() && counts==manualCounts,"Manual refresh not throttled");now+=std::chrono::seconds(30);check(media.requestCatalogRefresh(),"Manual refresh throttle never resets");
 media.ensureChannel("not-a-channel");const auto invalidCounts=counts;now+=std::chrono::hours(1);media.refreshCatalogs();check(counts==invalidCounts && media.tokens(message,true)[0].url.isEmpty(),"Invalid channel leaked previous room");media.stop();
 // Two network slots, global reuse, and A -> B -> A late replies.
 struct Request{QString url;std::function<void(twitch::HttpReply)> done;};std::vector<Request> requests;
 auto async=std::make_unique<chat::EmoteCache>(visual,[&](QString url,std::function<void(twitch::HttpReply)> done){requests.push_back({url,std::move(done)});},[&]{return now;});
 async->ensureChannel("123");check(requests.size()==2,"Catalog exceeds two concurrent requests");
 requests[0].done({200,empty(requests[0].url),false});requests[1].done({200,empty(requests[1].url),false});check(requests.size()==4 && requests[3].url==seven,"Channel request fixture order");
 async->ensureChannel("456");async->ensureChannel("123");requests[3].done({200,a,false});message.message="Live";check(async->tokens(message,true)[0].url.isEmpty(),"Late A reply resurrected stale snapshot after A B A");
 async->setCatalogActive(false);const size_t inFlight=requests.size();for(size_t i=2;i<inFlight;++i)if(i!=3)requests[i].done({200,empty(requests[i].url),false});check(requests.size()==inFlight,"Hidden queue starts more catalog requests");
 async->setCatalogActive(true);now+=std::chrono::seconds(5);async->refreshCatalogs();check(requests.size()>inFlight,"Queued catalog cancellation cannot resume");const auto late=requests.back().done;async.reset();late({200,a,false});
 // Queue admission failure must be retried after space becomes available.
 requests.clear();chat::EmoteCache saturated(visual,[&](QString url,std::function<void(twitch::HttpReply)> done){requests.push_back({url,std::move(done)});},[&]{return now;});
 for(int i=0;i<132;++i)saturated.get("https://cdn.7tv.app/emote/q"+QString::number(i)+"/2x.webp");saturated.ensureChannel("123");check(requests.size()==2,"Saturated queue exceeds concurrency");
 for(size_t i=0;i<requests.size();++i){auto done=requests[i].done;done({404,{},false});}const auto drained=requests.size();now+=std::chrono::seconds(5);saturated.refreshCatalogs();check(requests.size()==drained+2,"Queue admission failure permanently lost catalogs");saturated.stop();auto stopped=requests.back().done;stopped({200,a,false});
}
}
