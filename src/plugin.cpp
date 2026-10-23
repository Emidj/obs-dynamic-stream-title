// SPDX-License-Identifier: GPL-2.0-or-later
#include <obs-module.h>
#include <obs-frontend-api.h>
#include <QPointer>
#include <exception>
#include "dock.hpp"
#include "ui-language.hpp"
#include "compatibility.hpp"
#include "twitch-core-client.hpp"
#include <util/platform.h>
#include "twitch-core-runtime.h"
#include "twitch-core-diagnostics.hpp"
#include "ui/authorization-launch.hpp"
#include <QDesktopServices>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDir>
OBS_DECLARE_MODULE()
namespace {
QPointer<dst::Dock> dock;
dst::TwitchCoreClient twitchCore;
bool registered=false;
bool retained=false;
QString channelRead;
oto::ui::AuthorizationLaunch authorizationLaunch;
void updateConnection(){
 if(!dock)return;
 NexusTwitchMessage state{};
 if(!twitchCore.snapshot(state)){dock->liveState(false,false,false,dst::uiText(QString::fromUtf8("Core no disponible; actualiza los plugins compatibles.")));return;}
 dock->liveState(state.active,state.authorized,state.can_manage,QString::fromUtf8(state.status));
 if(auto url=authorizationLaunch.update(state.active,state.authorized,QString::fromUtf8(state.verification)))QDesktopServices::openUrl(*url);
 if(!state.authorized){channelRead.clear();return;}
 const auto account=QString::fromUtf8(state.account);
 dock->setRaidAccount(account);
 if(channelRead==account)return;
 channelRead=account;
 if(!twitchCore.channelInfo([](void *,int http,const char *json,uint32_t bytes){
  if(!dock)return;
  if(http!=200){dock->requestFailed(dst::uiText(QString::fromUtf8("No se pudo leer el canal. Reconecta para reintentar.")));return;}
  const auto data=QJsonDocument::fromJson(QByteArray(json,int(bytes))).object().value("data").toArray();
  if(data.isEmpty()){dock->channelInformation(dst::uiText(QString::fromUtf8("Respuesta de canal vacía.")));return;}
  const auto row=data.first().toObject();QStringList tags;for(auto tag:row.value("tags").toArray())tags.append(tag.toString());
  dst::ChannelFields fields;fields.title=row.value("title").toString();fields.gameId=row.value("game_id").toString();fields.category=row.value("game_name").toString();fields.tags=tags;if(fields.gameId.isEmpty()){fields.gameId="0";fields.category=dst::uiText(QString::fromUtf8("Sin categoría"));}dock->channelLoaded(fields);
 },nullptr))channelRead.clear();
}
void releaseCore(){twitchCore.setChanged({});twitchCore.setMessage({});twitchCore.detach();if(retained){retained=false;nexus_twitch_runtime_release();}channelRead.clear();}
constexpr const char *dockId="nexus.dynamic-stream-title";
void frontendEvent(enum obs_frontend_event event,void *)
{
 if(event!=OBS_FRONTEND_EVENT_EXIT)return;
 releaseCore();
 if(dock)obs_frontend_remove_dock(dockId);
 dock=nullptr;
 obs_frontend_remove_event_callback(frontendEvent,nullptr);
 registered=false;
 blog(LOG_INFO,"[Dynamic Stream Title] Frontend shutdown completed");
}
}
MODULE_EXPORT const char *obs_module_name(){return "OBS Dynamic Stream Title";}
MODULE_EXPORT const char *obs_module_description(){return "NEXUS native title dock - local development foundation";}
bool obs_module_load()
{
 const auto version=obs_get_version();
 if(!dst::supportedObsVersion(version)){
  blog(LOG_ERROR,"[Dynamic Stream Title] Unsupported OBS version %s; expected OBS 32.x (Windows x64)",obs_get_version_string());
  return false;
 }
 blog(LOG_INFO,"[Dynamic Stream Title] Loading %s on OBS %s (Windows x64, supported range 32.x)",DST_VERSION,obs_get_version_string());
 return true;
}
void obs_module_post_load()
{
 dst::setUiLanguage(QString::fromUtf8(obs_get_locale()));
 nexus_twitch_runtime_set_locale(obs_get_locale());
 // Old Overlay owns a private session. Refuse a second session until upgraded.
 NexusTwitchCoreInfo identity{};
 bool compatible=nexus_twitch_core_info(&identity,sizeof(identity))&&nexus::compatibleCore(identity);
 blog(compatible?LOG_INFO:LOG_ERROR,"[Dynamic Stream Title] Twitch Core %s ABI %u loaded from %s",identity.version,identity.abi,nexus::corePath(identity).toUtf8().constData());
 if(auto *overlay=obs_get_module("obs-twitch-overlays"))compatible=compatible&&os_dlsym(obs_get_module_lib(overlay),"nexus_overlay_uses_shared_runtime")!=nullptr;
 if(compatible&&!retained){
  char *path=obs_module_config_path("../nexus-twitch-core/twitch-session.bin");
  const auto session=QDir::cleanPath(QString::fromUtf8(path?path:""));bfree(path);
  retained=nexus_twitch_runtime_retain(session.toUtf8().constData())!=0;
  if(retained)twitchCore.attach(nexus_twitch_core_get_api(NEXUS_TWITCH_ABI,sizeof(NexusTwitchApi)));
  NexusTwitchMessage state{};if(twitchCore.snapshot(state))blog(LOG_INFO,"[Dynamic Stream Title] Shared runtime instance %llu; independent provider available",static_cast<unsigned long long>(state.instance));
 }else if(!compatible)blog(LOG_WARNING,"[Dynamic Stream Title] Legacy Overlay detected; connection blocked to prevent duplicate sessions");
 if(dock)return;
 try {
  auto *widget=new dst::Dock;
  if(!obs_frontend_add_dock_by_id(dockId,"Dynamic Stream Title",widget)){
   delete widget;blog(LOG_ERROR,"[Dynamic Stream Title] Dock registration failed");return;
  }
  dock=widget;
  dock->setDiagnostics([](QString message){blog(LOG_INFO,"[Dynamic Stream Title] %s",message.toUtf8().constData());});
  dock->setCoreIdentity(nexus::coreLabel(identity),nexus::corePath(identity));
  dock->enableLive([]{authorizationLaunch.arm();if(twitchCore.authorize())updateConnection();},[]{twitchCore.disconnect();});
  dock->enableEditing([](const dst::ChannelFields &fields){
   const auto json=QJsonDocument(QJsonObject{{"expected",dock->publishedFields().payload()},{"desired",fields.payload()}}).toJson(QJsonDocument::Compact);
   return twitchCore.guardedUpdate(json.constData(),uint32_t(json.size()),[](void *,int http,const char *,uint32_t){if(dock)dock->publicationFinished(http);},nullptr);
  },[](QString query){
   const auto utf8=query.toUtf8();
   return twitchCore.searchCategories(utf8.constData(),uint32_t(utf8.size()),[](void *,int http,const char *json,uint32_t bytes){
    if(!dock)return;if(http!=200){dock->requestFailed(dst::uiText(QString::fromUtf8("No se pudo buscar en Twitch. Inténtalo de nuevo.")));return;}
    const auto document=QJsonDocument::fromJson(QByteArray(json,int(bytes)));const auto data=document.object().value("data");
    if(!data.isArray()){dock->requestFailed(dst::uiText(QString::fromUtf8("Respuesta de categorías no válida.")));return;}
    QVector<QPair<QString,QString>> categories;for(auto item:data.toArray()){auto row=item.toObject();if(!row.value("id").toString().isEmpty()&&!row.value("name").toString().isEmpty())categories.append({row.value("id").toString(),row.value("name").toString()});}
    dock->categoriesLoaded(categories);
   },nullptr);
  });
  dock->enableRaiders([](QString title){const auto utf8=QJsonDocument(QJsonObject{{"expected",dock->publishedFields().payload()},{"desired",QJsonObject{{"title",title}}},{"require_live",true}}).toJson(QJsonDocument::Compact);return twitchCore.guardedUpdate(utf8.constData(),uint32_t(utf8.size()),[](void *,int http,const char *,uint32_t){if(dock)dock->automaticPublicationFinished(http);},nullptr);});
  dock->enableHype();
  char *statePath=obs_module_config_path("state.json");dock->enableProtection(QString::fromUtf8(statePath?statePath:""),[]{channelRead.clear();updateConnection();});bfree(statePath);
  twitchCore.setMessage([](const NexusTwitchMessage &message){
   if(!dock)return;
   if(message.kind==NEXUS_RAID)dock->receiveRaid(QString::fromUtf8(message.account),QString::fromUtf8(message.event_id),QString::fromUtf8(message.actor_id),QString::fromUtf8(message.actor));
   else if(message.kind==NEXUS_HYPE_BEGIN||message.kind==NEXUS_HYPE_END)dock->receiveHype(QString::fromUtf8(message.account),QString::fromUtf8(message.train_id),message.train_started_ms,message.kind==NEXUS_HYPE_BEGIN);
  });
  twitchCore.setChanged([](uint32_t kind){if(kind==NEXUS_STATE||kind==NEXUS_CLOSING)updateConnection();});updateConnection();
  obs_frontend_add_event_callback(frontendEvent,nullptr);registered=true;
  blog(LOG_INFO,"[Dynamic Stream Title] Dock registered");
 } catch(const std::exception &error){
  blog(LOG_ERROR,"[Dynamic Stream Title] Dock initialization failed: %s",error.what());
 }
}
void obs_module_unload()
{
 releaseCore();
 if(registered){obs_frontend_remove_event_callback(frontendEvent,nullptr);registered=false;}
 if(dock){obs_frontend_remove_dock(dockId);dock=nullptr;}
 blog(LOG_INFO,"[Dynamic Stream Title] Module unloaded");
}
