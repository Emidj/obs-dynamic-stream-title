// SPDX-License-Identifier: GPL-2.0-or-later
#include "shared-provider.hpp"
#include <QCoreApplication>
#include <QThread>
#include <cstring>
#include <map>
#include <algorithm>
#include <stdexcept>
#include <QJsonDocument>
#include "channel-fields.hpp"
namespace {
struct Client {NexusTwitchCallback callback=nullptr;void *context=nullptr;bool pending=false;};
std::weak_ptr<oto::twitch::Service> owner;
std::map<uint64_t,Client> clients;
uint64_t nextLease=1,instance=0;
bool ui(){return QCoreApplication::instance()&&QThread::currentThread()==QCoreApplication::instance()->thread();}
template<size_t N>void copy(char (&out)[N],QString value){auto bytes=value.toUtf8();auto length=std::min(size_t(bytes.size()),N-1);std::memcpy(out,bytes.constData(),length);out[length]=0;}
NexusTwitchMessage makeState(uint32_t kind=NEXUS_STATE){
 NexusTwitchMessage result{};result.size=sizeof(result);result.kind=kind;result.instance=instance;
 if(auto service=owner.lock()){
  result.active=service->active();result.authorized=service->authorized();copy(result.status,service->status());
  auto actor=service->previewActor();copy(result.account,QString::fromStdString(actor.id));copy(result.login,QString::fromStdString(actor.displayName));
  result.can_manage=service->canManageBroadcast();copy(result.verification,service->verificationUrl());copy(result.code,service->code());
 }
 return result;
}
void send(const NexusTwitchMessage &message){
 // A callback can release itself or a different client; never retain map iterators.
 const auto snapshot=clients;
 for(const auto &[id,value]:snapshot){auto current=clients.find(id);if(current==clients.end()||!current->second.callback)continue;
  const auto client=current->second;try{client.callback(client.context,&message);}catch(...){}
 }
}
uint64_t acquire(NexusTwitchCallback callback,void *context){
 if(!ui()||owner.expired()||clients.size()>=32)return 0;
 try{auto id=nextLease++;clients.emplace(id,Client{callback,context,false});return id;}catch(...){return 0;}
}
void release(uint64_t lease){if(ui())clients.erase(lease);}
int snapshot(uint64_t lease,NexusTwitchMessage *out,uint32_t bytes){
 if(!ui()||!out||bytes<sizeof(*out)||!clients.count(lease)||owner.expired())return 0;
 try{*out=makeState();return 1;}catch(...){return 0;}
}
int authorize(uint64_t lease){
 if(!ui()||!clients.count(lease))return 0;
 try{if(auto service=owner.lock()){
  service->authorizeBroadcast();return 1;
 }}catch(...){}return 0;
}
int channelInfo(uint64_t lease,NexusTwitchReply callback,void *context){
 if(!ui()||!callback||!clients.count(lease)||clients.at(lease).pending)return 0;
 try{auto service=owner.lock();if(!service||!service->authorized())return 0;
  clients.at(lease).pending=true;
  service->channelInformation([lease,callback,context](oto::twitch::HttpReply reply){
   auto client=clients.find(lease);if(client==clients.end())return;client->second.pending=false;
   try{callback(context,int(reply.status),reply.body.constData(),uint32_t(reply.body.size()));}catch(...){}
  });return 1;
 }catch(...){if(clients.count(lease))clients.at(lease).pending=false;return 0;}
}
int disconnect(uint64_t lease){
 if(!ui()||!clients.count(lease))return 0;
 try{if(auto service=owner.lock()){service->disconnectAccount();return 1;}}catch(...){}return 0;
}
template<class Start>int operation(uint64_t lease,NexusTwitchReply callback,void *context,Start start){
 if(!ui()||!callback||!clients.count(lease)||clients.at(lease).pending)return 0;
 try{auto service=owner.lock();if(!service||!service->authorized())return 0;
  clients.at(lease).pending=true;
  start(*service,[lease,callback,context](oto::twitch::HttpReply reply){auto it=clients.find(lease);if(it==clients.end())return;it->second.pending=false;try{callback(context,int(reply.status),reply.body.constData(),uint32_t(reply.body.size()));}catch(...){}});return 1;
 }catch(...){if(clients.count(lease))clients.at(lease).pending=false;return 0;}
}
int searchCategories(uint64_t lease,const char *query,uint32_t bytes,NexusTwitchReply callback,void *context){
 if(!query||!bytes||bytes>400)return 0;const auto text=QString::fromUtf8(query,int(bytes));
 return operation(lease,callback,context,[text](auto &service,auto done){service.searchCategories(text,std::move(done));});
}
int updateChannel(uint64_t lease,const char *json,uint32_t bytes,NexusTwitchReply callback,void *context){
 if(!json||!bytes||bytes>16384)return 0;
 const auto document=QJsonDocument::fromJson(QByteArray(json,int(bytes)));if(!document.isObject())return 0;
 const auto row=document.object();if(row.size()!=3||!row.value("title").isString()||!row.value("game_id").isString()||!row.value("tags").isArray())return 0;
 dst::ChannelFields fields;fields.title=row.value("title").toString();fields.gameId=row.value("game_id").toString();
 for(const auto &tag:row.value("tags").toArray()){if(!tag.isString())return 0;fields.tags.append(tag.toString());}
 if(!fields.error().isEmpty())return 0;
 const auto payload=QJsonDocument(fields.payload()).toJson(QJsonDocument::Compact);
 return operation(lease,callback,context,[payload](auto &service,auto done){service.updateChannel(payload,std::move(done));});
}
int updateTitle(uint64_t lease,const char *title,uint32_t bytes,NexusTwitchReply callback,void *context){
 if(!title||!bytes||bytes>2048)return 0;const auto text=QString::fromUtf8(title,int(bytes));
 if(text.trimmed().isEmpty()||text.toUcs4().size()>140)return 0;
 const auto payload=QJsonDocument(QJsonObject{{"title",text}}).toJson(QJsonDocument::Compact);
 return operation(lease,callback,context,[payload](auto &service,auto done){service.updateChannel(payload,std::move(done));});
}
int guardedUpdate(uint64_t lease,const char *json,uint32_t bytes,NexusTwitchReply callback,void *context){
 if(!json||!bytes||bytes>32768)return 0;
 const auto root=QJsonDocument::fromJson(QByteArray(json,int(bytes))).object();
 const auto expected=root.value("expected").toObject(),desired=root.value("desired").toObject();
 auto valid=[](const QJsonObject &row){dst::ChannelFields f;f.title=row.value("title").toString();f.gameId=row.value("game_id").toString();for(auto t:row.value("tags").toArray()){if(!t.isString())return false;f.tags.append(t.toString());}return row.size()==3&&row.value("tags").isArray()&&f.error().isEmpty();};
 const auto title=desired.value("title").toString();
 if(!valid(expected)||!(valid(desired)||(desired.size()==1&&!title.trimmed().isEmpty()&&title.toUcs4().size()<=140)))return 0;
 if(root.contains("require_live")&&!root.value("require_live").isBool())return 0;
 const bool live=root.value("require_live").toBool();if(live&&desired.size()!=1)return 0;
 return operation(lease,callback,context,[expected,desired,live](auto &service,auto done){
  auto *same=&service;
  auto publish=[expected,desired,done,same]{same->channelInformation([expected,desired,done,same](auto reply){
   if(reply.status!=200){done(std::move(reply));return;}
   const auto rows=QJsonDocument::fromJson(reply.body).object().value("data").toArray();
   const auto row=rows.isEmpty()?QJsonObject{}:rows.first().toObject();
   const auto game=row.value("game_id").toString().isEmpty()?QString("0"):row.value("game_id").toString();
   if(row.value("title")!=expected.value("title")||(desired.size()!=1&&(game!=expected.value("game_id").toString()||row.value("tags")!=expected.value("tags")))){reply.status=409;done(std::move(reply));return;}
   same->updateChannel(QJsonDocument(desired).toJson(QJsonDocument::Compact),std::move(done));
  });};
  if(!live){publish();return;}
  const auto account=QString::fromStdString(service.previewActor().id);
  service.streamInformation([publish,done,account](auto reply){
   if(reply.networkError||reply.status!=200){done(std::move(reply));return;}
   const auto object=QJsonDocument::fromJson(reply.body).object();
   if(!object.value("data").isArray()){reply.status=502;done(std::move(reply));return;}
   const auto rows=object.value("data").toArray();
   if(rows.isEmpty()){reply.status=412;reply.body="{}";done(std::move(reply));return;}
   if(rows.size()!=1||rows.first().toObject().value("user_id").toString()!=account||rows.first().toObject().value("type").toString()!="live"){reply.status=502;done(std::move(reply));return;}
   publish();
  });
 });
}
const NexusTwitchApi api{sizeof(NexusTwitchApi),NEXUS_TWITCH_ABI,acquire,release,snapshot,authorize,channelInfo,disconnect,searchCategories,updateChannel,updateTitle,guardedUpdate};
}
extern "C" __declspec(dllexport) const NexusTwitchApi *nexus_twitch_core_get_api(uint32_t abi,uint32_t bytes){
 return ui()&&abi==NEXUS_TWITCH_ABI&&bytes>=sizeof(NexusTwitchApi)?&api:nullptr;
}
namespace oto::twitch {
void publishSharedService(std::shared_ptr<Service> service){
 if(!ui()||!service)return;
 if(auto existing=owner.lock()){if(existing!=service)throw std::logic_error("Shared Twitch owner already registered");return;}
 owner=service;++instance;
 service->setSharedCallbacks([]{if(auto service=owner.lock()){if(!service->authorized())for(auto &[id,client]:clients)client.pending=false;}send(makeState());},[](const events::EventPtr &event){
  if(!event||event->data().origin!=events::EventOrigin::Twitch)return;
  auto message=makeState();const auto &data=event->data();
  if(const auto *raid=std::get_if<events::RaidEvent>(&data.payload)){message.kind=NEXUS_RAID;copy(message.actor,QString::fromStdString(raid->login));copy(message.actor_id,QString::fromStdString(raid->raider.id));}
  else if(const auto *hype=std::get_if<events::HypeTrainEvent>(&data.payload)){
   if(hype->stage==events::HypeTrainStage::Progress)return;
   message.kind=hype->stage==events::HypeTrainStage::Begin?NEXUS_HYPE_BEGIN:NEXUS_HYPE_END;
   copy(message.train_id,QString::fromStdString(hype->trainId));
   message.train_started_ms=std::chrono::duration_cast<std::chrono::milliseconds>(hype->startedAt.time_since_epoch()).count();
  }else return;
  // Retained batches may belong to a prior channel: identify the event's channel,
  // never relabel it with the currently authorized account.
  copy(message.account,QString::fromStdString(data.channelId));
  copy(message.event_id,QString::fromStdString(data.id));send(message);
 });
}
void unpublishSharedService(){
 if(!ui())return;
 if(auto service=owner.lock())service->setSharedCallbacks({},{});
 // New acquire is impossible during the closing notification.
 auto message=makeState(NEXUS_CLOSING);owner.reset();send(message);clients.clear();
}
}

