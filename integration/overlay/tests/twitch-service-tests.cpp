// SPDX-License-Identifier: GPL-2.0-or-later
#include "twitch/service.hpp"
#include "twitch/shared-provider.hpp"
#include "twitch-core-client.hpp"
#include "ui/authorization-launch.hpp"
#include "ui/twitch-panel.hpp"
#include "twitch/protocol.hpp"
#include "adapters/json-daily-store.hpp"
#include "sources/supporter-model.hpp"
#include <QApplication>
#include <QWidget>
#include <QLabel>
#include <obs-module.h>
#include <QImage>
#include <QBuffer>
#include <QJsonDocument>
#include <QDateTime>
#include <QTcpServer>
#include <QTcpSocket>
#include <QCryptographicHash>
#include <QEventLoop>
#include <QTemporaryDir>
#include <QUrlQuery>
#include <QRegularExpression>
#include <iostream>
#include <stdexcept>
using namespace oto;
const char *obs_module_text(const char *key){return key;}
namespace {
void require(bool ok,const char *why){if(!ok)throw std::runtime_error(why);}
void waitUntil(std::function<bool()> ready){QEventLoop loop;QTimer check;QObject::connect(&check,&QTimer::timeout,&loop,[&]{if(ready())loop.quit();});check.start(5);QTimer::singleShot(6000,&loop,&QEventLoop::quit);loop.exec();require(ready(),"Service test timed out");}
QByteArray browserRequest(const QUrl &uri,QByteArray method,QByteArray path,QByteArray body={}){
 QTcpSocket client;QByteArray response;bool finished=false;
 QObject::connect(&client,&QTcpSocket::connected,&client,[&]{client.write(method+" "+path+" HTTP/1.1\r\nHost: localhost:"+QByteArray::number(uri.port())+"\r\nContent-Type: application/json\r\nContent-Length: "+QByteArray::number(body.size())+"\r\n\r\n"+body);});
 QObject::connect(&client,&QTcpSocket::readyRead,&client,[&]{response+=client.readAll();});QObject::connect(&client,&QTcpSocket::disconnected,&client,[&]{response+=client.readAll();finished=true;});
 client.connectToHost(QHostAddress::LocalHost,uri.port());waitUntil([&]{return finished;});return response;
}
void completeBrowser(twitch::Service &service,QString token="authorized-access"){
 const QUrl authorize(service.verificationUrl());const QUrlQuery query(authorize);const QUrl uri(query.queryItemValue("redirect_uri"));
 const auto page=browserRequest(uri,"GET","/");const auto match=QRegularExpression("fetch\\('(/complete/[a-f0-9]{64})'").match(QString::fromUtf8(page));require(match.hasMatch(),"Browser token bridge missing");
 const auto body=QJsonDocument(QJsonObject{{"state",query.queryItemValue("state")},{"access_token",token},{"token_type","bearer"}}).toJson();
 require(browserRequest(uri,"POST",match.captured(1).toUtf8(),body).startsWith("HTTP/1.1 200"),"Browser token delivery failed");
}
struct Config:IConfigStore{ConfigLoadResult load()override{return {ConfigStatus::Loaded,{false}};}bool save(Settings)override{return true;}};
struct Log:ILogger{mutable std::vector<std::string> lines;void write(LogLevel,std::string_view text)const noexcept override{try{lines.emplace_back(text);}catch(...){}}};
struct Daily:IDailyStore{bool fail=false;state::DailyBook book;state::DailyBook load()override{return book;}bool save(const state::DailyBook &next)override{if(fail)return false;book=next;return true;}};
QByteArray frame(QJsonObject object){const auto bytes=QJsonDocument(object).toJson(QJsonDocument::Compact);QByteArray out;out+=char(0x81);if(bytes.size()<126)out+=char(bytes.size());else{out+=char(126);out+=char(bytes.size()>>8);out+=char(bytes.size());}return out+bytes;}
struct Mock:QTcpServer {
 bool isLive=false;int streamReads=0,streamStatus=200;
 int channelWrites=0,channelStatus=204;QJsonObject channelPayload;bool channelAuth=false;QJsonObject channelState{{"title","Initial"},{"game_id","509660"},{"tags",QJsonArray{"Art"}},{"game_name","Just Chatting"}};
	QTcpSocket *ws=nullptr;int polls=0,subscriptions=0,refreshes=0,sockets=0,historyRequests=0,hypeRequests=0;QJsonValue hypeCurrent{QJsonValue::Null};int hypeDelay=0;bool leakedSecret=false;
	QString deniedType;bool denyAll=false,missingScopes=false,denyRefresh=false;int accessLifetime=3600,validations=0,profileLookups=0,avatarDownloads=0,loginLookups=0;bool writeChat=false;int chatWrites=0;QString sentChat;bool avatarAuth=false,badAvatar=false;int avatarDelay=0;int badgeRequests=0;bool badgeAuth=true;
	bool broadcast=false;QString expectedClient="publicclient123456";int deviceRequests=0;
	Mock(){require(listen(QHostAddress::LocalHost,0),"Mock server listen");QObject::connect(this,&QTcpServer::newConnection,this,[this]{auto *s=nextPendingConnection();auto data=std::make_shared<QByteArray>();
		QObject::connect(s,&QTcpSocket::readyRead,this,[this,s,data]{if(s->property("ws").toBool()){s->readAll();return;}*data+=s->readAll();const auto headerEnd=data->indexOf("\r\n\r\n");if(headerEnd<0)return;
			int length=0;QByteArray key;for(const auto &line:data->left(headerEnd).split('\n')){if(line.toLower().startsWith("content-length:"))length=line.mid(line.indexOf(':')+1).trimmed().toInt();if(line.toLower().startsWith("sec-websocket-key:"))key=line.mid(line.indexOf(':')+1).trimmed();}
			if(data->size()<headerEnd+4+length)return;const auto path=data->split(' ').value(1);const auto body=data->mid(headerEnd+4);leakedSecret=leakedSecret || data->contains("client_secret");
			if(path.startsWith("/ws")){
				++sockets;ws=s;s->setProperty("ws",true);const auto accept=QCryptographicHash::hash(key+"258EAFA5-E914-47DA-95CA-C5AB0DC85B11",QCryptographicHash::Sha1).toBase64();
				s->write("HTTP/1.1 101 Switching Protocols\r\nUpgrade: websocket\r\nConnection: Upgrade\r\nSec-WebSocket-Accept: "+accept+"\r\n\r\n");
				s->write(frame({{"metadata",QJsonObject{{"message_type","session_welcome"}}},{"payload",QJsonObject{{"session",QJsonObject{{"id",QStringLiteral("session-%1").arg(sockets)},{"keepalive_timeout_seconds",10}}}}}}));return;
			}
			QJsonObject reply;int status=200;
			if(path.startsWith("/avatars/")){
				++avatarDownloads;avatarAuth=avatarAuth || data->contains("Authorization:") || data->contains("Client-Id:");QImage image(32,32,QImage::Format_RGBA8888_Premultiplied);image.fill(QColor(240,100,50));QByteArray bytes;QBuffer buffer(&bytes);buffer.open(QIODevice::WriteOnly);image.save(&buffer,"PNG");
				if(badAvatar)bytes="invalid image";const auto response="HTTP/1.1 200 OK\r\nContent-Length: "+QByteArray::number(bytes.size())+"\r\nConnection: close\r\n\r\n"+bytes;
				QTimer::singleShot(avatarDelay,s,[s,response]{s->write(response);s->disconnectFromHost();});return;
			}
			if(path=="/device"){++deviceRequests;require(body.contains("client_id="+expectedClient.toUtf8()),"Device grant uses wrong application");reply={{"device_code","synthetic-device"},{"user_code","ABCD-EFGH"},{"verification_uri","https://www.twitch.tv/activate"},{"expires_in",60},{"interval",1}};}
			else if(path=="/token"){
				if(body.contains("grant_type=refresh_token")){++refreshes;if(denyRefresh){status=401;reply={{"message","invalid refresh token"}};}else reply={{"access_token","fresh-access"},{"refresh_token","rotated-refresh"}};}
				else if(++polls==1){status=400;reply={{"message","authorization_pending"}};}
				else reply={{"access_token","authorized-access"},{"refresh_token","authorized-refresh"}};
			}else if(path=="/validate"){
				++validations;
				if(data->contains("OAuth expired-access")){status=401;reply={{"status",401}};}
				else{QJsonArray scopes;if(!missingScopes)for(const auto &scope:twitch::requiredScopes())scopes.append(scope);if(writeChat)scopes.append("user:write:chat");if(broadcast)scopes.append("channel:manage:broadcast");reply={{"client_id",expectedClient},{"user_id","123"},{"login","mock-channel"},{"scopes",scopes},{"expires_in",data->contains("OAuth fresh-access")?3600:accessLifetime}};}
			}else if(path.startsWith("/users?login=")){++loginLookups;const auto login=QString::fromUtf8(path.mid(13));QJsonArray rows;if(login=="viewer" || login=="raider")rows.append(QJsonObject{{"id",login=="viewer"?"456":"789"},{"login",login},{"display_name",login=="viewer"?"FixtureViewer":"FixtureRaider"}});reply={{"data",rows}};}
			else if(path.startsWith("/users?id=")){++profileLookups;const auto id=QString::fromUtf8(path.mid(10));reply={{"data",QJsonArray{QJsonObject{{"id",id},{"profile_image_url",QString::fromUtf8(endpoints().avatarCdn)+id+".png"}}}}};}
			else if(path.startsWith("/streams?")){++streamReads;status=streamStatus;require(path.contains("user_id=123"),"Live check used another channel");reply={{"data",isLive?QJsonArray{QJsonObject{{"user_id","123"},{"type","live"}}}:QJsonArray{}}};}
            else if(path.startsWith("/channels?")&&data->startsWith("PATCH ")){++channelWrites;channelPayload=QJsonDocument::fromJson(body).object();channelAuth=data->contains("Authorization: Bearer authorized-access")&&data->contains("Client-Id: "+expectedClient.toUtf8())&&data->contains("Content-Type: application/json");status=channelStatus;if(status==204)for(auto i=channelPayload.begin();i!=channelPayload.end();++i)channelState.insert(i.key(),i.value());}
			else if(path.startsWith("/channels?")){auto row=channelState;row.insert("broadcaster_id",QString::fromUtf8(path.mid(path.indexOf('=')+1)));reply={{"data",QJsonArray{row}}};}
   else if(path.startsWith("/categories?")){reply={{"data",QJsonArray{QJsonObject{{"id","509660"},{"name","Art"}}}}};}
			else if(path=="/badges/global" || path=="/badges?broadcaster_id=123" || path=="/cheermotes?broadcaster_id=123"){++badgeRequests;badgeAuth=badgeAuth && data->contains("Authorization: Bearer authorized-access") && data->contains("Client-Id: publicclient123456");reply={{"data",QJsonArray{}}};}
			else if(path.startsWith("/hype?")){++hypeRequests;reply={{"data",QJsonArray{QJsonObject{{"current",hypeCurrent}}}}};}
			else if(path.startsWith("/followers?")){++historyRequests;reply={{"total",125000},{"data",QJsonArray{QJsonObject{{"user_id","456"},{"user_name","FixtureViewer"},{"followed_at","2026-01-01T12:00:00Z"}}}}};}
			else if(path=="/chat/messages"){++chatWrites;const auto message=QJsonDocument::fromJson(body).object();require(message["sender_id"]=="123" && message["broadcaster_id"]=="123","Wrong chat destination");sentChat=message["message"].toString();reply={{"data",QJsonArray{QJsonObject{{"message_id","sent-123"},{"is_sent",true}}}}};}
			else if(path=="/subscriptions"){++subscriptions;const auto type=QJsonDocument::fromJson(body).object().value("type").toString();status=denyAll || type==deniedType?403:202;reply={{"data",QJsonArray{}}};}
			else{status=404;}
			const auto json=status==204?QByteArray{}:QJsonDocument(reply).toJson(QJsonDocument::Compact);const auto response="HTTP/1.1 "+QByteArray::number(status)+" Reply\r\nContent-Length: "+QByteArray::number(json.size())+"\r\nConnection: close\r\n\r\n"+json;QTimer::singleShot(path.startsWith("/hype?")?hypeDelay:0,s,[s,response]{s->write(response);s->disconnectFromHost();});
		});
	});}
	twitch::Endpoints endpoints()const{const auto base="http://127.0.0.1:"+QByteArray::number(serverPort())+"/";return {base,base+"subscriptions","ws://127.0.0.1:"+QByteArray::number(serverPort())+"/ws",base+"users",base+"avatars/",base+"followers",base+"chat/messages",base+"channels",base+"hype",base+"badges",base+"cheermotes"};}
	void notify(QString type,QString id,QJsonObject event,QString timestamp={}){
		require(ws,"No socket for notification");event.insert("broadcaster_user_id","123");
 if(type.startsWith("channel.hype_train.")) {
  if(!event.contains("id"))event.insert("id","train-fixture");if(!event.contains("started_at"))event.insert("started_at","2026-10-04T08:00:00Z");if(!event.contains("top_contributions"))event.insert("top_contributions",QJsonArray{});
  if(type.endsWith(".end"))event.insert("ended_at","2026-10-04T08:04:00Z");
  else {if(!event.contains("expires_at"))event.insert("expires_at","2026-10-04T08:05:00Z");if(!event.contains("progress"))event.insert("progress",event.value("total"));}
 }
		ws->write(frame({{"metadata",QJsonObject{{"message_type","notification"},{"message_id",id},{"message_timestamp",timestamp.isEmpty()?QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs):timestamp}}},
			{"payload",QJsonObject{{"subscription",QJsonObject{{"type",type},{"version",type=="channel.follow" || type.startsWith("channel.hype_train.")?"2":"1"}}},{"event",event}}}}));
	}
	void cheer(QString id,int bits){require(ws,"No shared socket");ws->write(frame({{"metadata",QJsonObject{{"message_type","notification"},{"message_id",id},{"message_timestamp",QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs)}}},
		{"payload",QJsonObject{{"subscription",QJsonObject{{"type","channel.cheer"},{"version","1"}}},{"event",QJsonObject{{"broadcaster_user_id","123"},{"user_id","456"},{"user_name","Viewer"},{"is_anonymous",false},{"bits",bits}}}}}}));}
	void migrate(){ws->write(frame({{"metadata",QJsonObject{{"message_type","session_reconnect"}}},{"payload",QJsonObject{{"session",QJsonObject{{"reconnect_url",QString::fromUtf8(endpoints().socket)+"?session=migration"}}}}}}));}
	void chat(){ws->write(frame({{"metadata",QJsonObject{{"message_type","notification"},{"message_id","opaque+chat/id=="},{"message_timestamp",QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs)}}},
		{"payload",QJsonObject{{"subscription",QJsonObject{{"type","channel.chat.message"},{"version","1"}}},{"event",QJsonObject{{"broadcaster_user_id","123"},{"chatter_user_id","123"},{"chatter_user_name","PrivateName"},{"message",QJsonObject{{"text","private-chat-content"}}}}}}}}));}
	void revoke(QString reason,QString type="channel.cheer",QString version="1"){
		require(ws,"No socket to revoke");ws->write(frame({{"metadata",QJsonObject{{"message_type","revocation"},{"message_id","synthetic-revocation"}}},
			{"payload",QJsonObject{{"subscription",QJsonObject{{"status",reason},{"type",type},{"version",version}}}}}}));
	}
};
}
int main(int argc,char **argv){QApplication app(argc,argv);try{
    ui::AuthorizationLaunch launch;
    const QString activation="https://www.twitch.tv/activate?public=true&device-code=FIXTURE";
    require(!launch.update(true,false,activation),"Restored session opened browser without click");
    launch.arm();require(!launch.update(true,false,{}),"Browser opened before code ready");
    const auto launched=launch.update(true,false,activation);require(launched && launched->toString()==activation && !launch.update(true,false,activation),"Connect launch not exact or repeated on poll");
    launch.arm();require(!launch.update(false,false,{}) && !launch.update(true,false,activation),"Failed Connect left stale intent");
    launch.arm();require(!launch.update(true,true,{}) && !launch.update(true,false,activation),"Saved authorization left stale intent");
    for(const auto &url:{"http://www.twitch.tv/activate","https://evil.example/activate","https://www.twitch.tv.evil.example/activate","https://user@www.twitch.tv/activate","https://www.twitch.tv:444/activate","https://www.twitch.tv/settings/connections","https://www.twitch.tv/activate#fragment"}){launch.arm();require(!launch.update(true,false,url),"Unsafe authorization URL launched");}
    launch.arm();require(launch.update(true,false,"https://id.twitch.tv/oauth2/authorize?client_id=fixture").has_value(),"Direct browser authorization not launched");
    for(const auto &url:{"http://id.twitch.tv/oauth2/authorize","https://id.twitch.tv.evil.example/oauth2/authorize","https://user@id.twitch.tv/oauth2/authorize","https://id.twitch.tv:444/oauth2/authorize","https://id.twitch.tv/oauth2/token","https://id.twitch.tv/oauth2/authorize#token"}){launch.arm();require(!launch.update(true,false,url),"Unsafe direct OAuth URL launched");}
	// Direct grants have no refresh token. Red until API validation, green on a
	// validated grant, and restored without opening an authorization page.
	{
	 QTemporaryDir direct;auto directCore=std::make_shared<PluginCore>(std::make_unique<Config>(),std::make_unique<Log>(),std::make_unique<Daily>());directCore->start();
	 Mock oauth;auto endpoints=oauth.endpoints();endpoints.browserPort=0;const auto file=direct.filePath("session.bin");
	 {twitch::Service first(directCore,file,endpoints);const auto borrowed=std::shared_ptr<twitch::Service>(&first,[](auto *){});QWidget panel;ui::createTwitchControls(&panel,borrowed);auto *indicator=panel.findChild<QLabel *>("otoTwitchAuthorization");
	  require(indicator && indicator->text().contains("Twitch.Unauthorized") && indicator->styleSheet().contains("#ff6b6b") && !first.authorized(),"Fresh session not visibly red");first.connectAccount({},twitch::AuthorizationFlow::Browser);
	  require(!first.authorized() && first.code().isEmpty() && first.verificationUrl().startsWith("https://id.twitch.tv/oauth2/authorize") && oauth.deviceRequests==0,"Browser flow fell back to device or turned green early");
	  oauth.expectedClient=first.client();completeBrowser(first);waitUntil([&]{return first.status().contains("13/13");});require(first.authorized() && first.canRenewAuthorization(),"Validated browser grant not authorized");
	  require(indicator->text().contains("Twitch.Authorized") && indicator->styleSheet().contains("#55d887") && indicator->accessibleName()=="Twitch.Authorized","Validated session not visibly green");
	  twitch::Session saved;require(twitch::SessionStore(file).load(saved)==twitch::SessionLoad::Loaded && saved.refresh.isEmpty() && saved.enabled,"Implicit grant persisted as renewable");first.stop();require(!first.authorized(),"Stopped session still green");}
	 {twitch::Service restored(directCore,file,endpoints);waitUntil([&]{return restored.status().contains("13/13");});require(restored.authorized() && restored.verificationUrl().isEmpty() && oauth.deviceRequests==0 && oauth.refreshes==0 && oauth.sockets==2,"Saved implicit grant requested another authorization");
	  restored.renewAuthorization();require(!restored.authorized() && !restored.verificationUrl().isEmpty(),"Manual implicit reauthorization failed");completeBrowser(restored);waitUntil([&]{return restored.status().contains("13/13");});
	  require(restored.authorized() && oauth.refreshes==0 && oauth.deviceRequests==0,"Implicit renewal incorrectly used refresh/device grant");restored.disconnectAccount();require(!restored.authorized(),"Disconnected session still green");restored.stop();}
	 directCore->stop();
	}
	{
	 QTemporaryDir direct;auto c=std::make_shared<PluginCore>(std::make_unique<Config>(),std::make_unique<Log>(),std::make_unique<Daily>());c->start();Mock oauth;oauth.accessLifetime=1;oauth.expectedClient=twitch::Service::defaultClient();auto e=oauth.endpoints();e.browserPort=0;const auto file=direct.filePath("session.bin");
	 twitch::Service first(c,file,e);const auto borrowed=std::shared_ptr<twitch::Service>(&first,[](auto *){});QWidget panel;ui::createTwitchControls(&panel,borrowed);auto *indicator=panel.findChild<QLabel *>("otoTwitchAuthorization");first.connectAccount({},twitch::AuthorizationFlow::Browser);completeBrowser(first);waitUntil([&]{return first.authorized();});waitUntil([&]{return !first.active();});
	 require(!first.authorized() && first.status().contains(i18n::qtext("Status.Expired")) && first.verificationUrl().isEmpty() && oauth.refreshes==0 && oauth.deviceRequests==0,"Expired implicit session still green or opened authorization automatically");
	 require(indicator->text().contains("Twitch.Unauthorized") && indicator->styleSheet().contains("#ff6b6b"),"Expired session not visibly red");
	 oauth.accessLifetime=3600;first.connectAccount({},twitch::AuthorizationFlow::Browser);require(!first.verificationUrl().isEmpty(),"Expired session could not reopen direct authorization");completeBrowser(first);waitUntil([&]{return first.status().contains("13/13");});require(first.authorized(),"Reauthorization did not recover expired session");first.stop();c->stop();
	}
	{
	 QTemporaryDir direct;auto c=std::make_shared<PluginCore>(std::make_unique<Config>(),std::make_unique<Log>(),std::make_unique<Daily>());c->start();Mock oauth;oauth.expectedClient=twitch::Service::defaultClient();auto e=oauth.endpoints();e.browserPort=0;const auto file=direct.filePath("session.bin");
	 twitch::Session previous{twitch::Service::defaultClient(),"expired-access",{},false};require(twitch::SessionStore(file).save(previous),"Previous grant fixture");
	 twitch::Service first(c,file,e);first.connectAccount({},twitch::AuthorizationFlow::Browser);waitUntil([&]{return !first.verificationUrl().isEmpty();});require(oauth.deviceRequests==0,"Expired saved grant required second Connect or device page");
	 oauth.missingScopes=true;completeBrowser(first);waitUntil([&]{return !first.active();});twitch::Session kept;
	 require(!first.authorized() && oauth.sockets==0 && twitch::SessionStore(file).load(kept)==twitch::SessionLoad::Loaded && kept.access==previous.access,"Unvalidated browser token became green, persisted or erased previous grant");first.stop();c->stop();
	}

 // Broadcast grant is requested only by the Title consumer; validate before use.
 {
  QTemporaryDir grant;auto c=std::make_shared<PluginCore>(std::make_unique<Config>(),std::make_unique<Log>(),std::make_unique<Daily>());c->start();
  Mock oauth;oauth.broadcast=true;oauth.expectedClient=twitch::Service::defaultClient();
  auto endpoints=oauth.endpoints();endpoints.categories="http://127.0.0.1:"+QByteArray::number(oauth.serverPort())+"/categories";
  twitch::Service title(c,grant.filePath("session.bin"),endpoints);title.authorizeBroadcast();
  require(QUrlQuery(QUrl(title.verificationUrl())).queryItemValue("scope",QUrl::FullyDecoded).split(' ').contains("channel:manage:broadcast"),"Broadcast scope missing");
  completeBrowser(title);waitUntil([&]{return title.status().contains("13/13");});
  require(title.canManageBroadcast()&&oauth.sockets==1&&oauth.subscriptions==13,"Broadcast grant not validated on existing transport");
  const auto borrowedTitle=std::shared_ptr<twitch::Service>(&title,[](auto*){});twitch::publishSharedService(borrowedTitle);
  const auto *api=nexus_twitch_core_get_api(NEXUS_TWITCH_ABI,sizeof(NexusTwitchApi));auto lease=api->acquire(nullptr,nullptr);
  auto reply=[](void *context,int status,const char *,uint32_t){*static_cast<int*>(context)=status;};int code=0;
  const auto payload=QByteArray("{\"title\":\"Manual test\",\"game_id\":\"509660\",\"tags\":[\"Art\"]}");
  require(!api->update_channel(lease,"{}",2,reply,&code)&&oauth.channelWrites==0,"Invalid draft reached network");
  require(api->update_channel(lease,payload.constData(),payload.size(),reply,&code),"Manual update rejected");
  require(!api->update_channel(lease,payload.constData(),payload.size(),reply,&code),"Duplicate in-flight update accepted");
  waitUntil([&]{return code!=0;});require(code==204&&oauth.channelWrites==1&&oauth.channelAuth&&oauth.channelPayload.value("title")=="Manual test"&&oauth.channelPayload.value("game_id")=="509660"&&oauth.channelPayload.value("tags").toArray().first()=="Art","Wrong PATCH payload or auth");
  oauth.channelStatus=503;code=0;require(api->update_channel(lease,payload.constData(),payload.size(),reply,&code),"Error test update rejected");waitUntil([&]{return code!=0;});require(code==503&&oauth.channelWrites==2,"Write error retried or hidden");
  int searchReply=0;require(api->search_categories(lease,"Art",3,reply,&searchReply),"Category search rejected");waitUntil([&]{return searchReply!=0;});require(searchReply==200&&oauth.channelWrites==2&&oauth.sockets==1&&oauth.subscriptions==13,"Search published or duplicated transport");
  oauth.channelStatus=204;code=0;
  const QByteArray automaticTitle="Applied base | Thanks Raiders @fixtureraider";
  require(!api->update_title(lease," ",1,reply,&code)&&oauth.channelWrites==2,"Invalid automatic title reached network");
  require(api->update_title(lease,automaticTitle.constData(),automaticTitle.size(),reply,&code),"Automatic title rejected");
  require(!api->update_title(lease,automaticTitle.constData(),automaticTitle.size(),reply,&code),"Concurrent automatic write accepted");
  waitUntil([&]{return code!=0;});
  require(code==204&&oauth.channelWrites==3&&oauth.channelAuth&&oauth.channelPayload.size()==1&&oauth.channelPayload.value("title")==QString::fromUtf8(automaticTitle)&&oauth.sockets==1&&oauth.subscriptions==13,"Automatic update leaked manual fields or duplicated transport");
  auto expected=oauth.channelState;expected.remove("game_name");
  auto guarded=QJsonDocument(QJsonObject{{"expected",expected},{"desired",QJsonObject{{"title","Guarded title"}}}}).toJson(QJsonDocument::Compact);code=0;
  require(api->guarded_update(lease,guarded.constData(),guarded.size(),reply,&code),"Guarded title rejected");waitUntil([&]{return code!=0;});
  require(code==204&&oauth.channelWrites==4&&oauth.channelPayload.size()==1,"Guarded title failed or leaked fields");
  oauth.channelState.insert("title","External edit");code=0;
  require(api->guarded_update(lease,guarded.constData(),guarded.size(),reply,&code),"Conflict check rejected before read");waitUntil([&]{return code!=0;});
  require(code==409&&oauth.channelWrites==4&&oauth.channelState.value("title")=="External edit","External title overwritten");
  expected=oauth.channelState;expected.remove("game_name");auto desired=expected;desired.insert("title","Manual after conflict");
  guarded=QJsonDocument(QJsonObject{{"expected",expected},{"desired",desired}}).toJson(QJsonDocument::Compact);oauth.channelState.insert("game_id","999");code=0;
  require(api->guarded_update(lease,guarded.constData(),guarded.size(),reply,&code),"Manual conflict check rejected");waitUntil([&]{return code!=0;});require(code==409&&oauth.channelWrites==4,"External category overwritten");
  expected=oauth.channelState;expected.remove("game_name");
  guarded=QJsonDocument(QJsonObject{{"expected",expected},{"desired",QJsonObject{{"title","Live only event"}}},{"require_live",true}}).toJson(QJsonDocument::Compact);code=0;
  require(api->guarded_update(lease,guarded.constData(),guarded.size(),reply,&code),"Offline check rejected");waitUntil([&]{return code!=0;});require(code==412&&oauth.channelWrites==4&&oauth.streamReads==1,"Offline event published or polled");
  oauth.isLive=true;code=0;require(api->guarded_update(lease,guarded.constData(),guarded.size(),reply,&code),"Live check rejected");waitUntil([&]{return code!=0;});require(code==204&&oauth.channelWrites==5&&oauth.streamReads==2&&oauth.sockets==1&&oauth.subscriptions==13,"Live event failed or added polling/subscriptions");
  oauth.streamStatus=503;code=0;require(api->guarded_update(lease,guarded.constData(),guarded.size(),reply,&code),"Live failure request rejected");waitUntil([&]{return code!=0;});require(code==503&&oauth.channelWrites==5,"Unknown stream state allowed publication");
  api->release(lease);twitch::unpublishSharedService();
  title.disconnectAccount();require(!title.canManageBroadcast(),"Broadcast permission survived disconnect");title.stop();c->stop();
 }
	// A clean installation connects without manual application configuration;
	// reopening reuses its protected grant rather than requesting another code.
	{
	 QTemporaryDir clean;auto cleanCore=std::make_shared<PluginCore>(std::make_unique<Config>(),std::make_unique<Log>(),std::make_unique<Daily>());cleanCore->start();
	 Mock onboarding;onboarding.expectedClient=twitch::Service::defaultClient();const auto filename=clean.filePath("session.bin");
	 {twitch::Service first(cleanCore,filename,onboarding.endpoints());require(first.client()==onboarding.expectedClient,"Missing built-in public application");first.connectAccount();waitUntil([&]{return first.status().contains("13/13");});first.stop();}
	 {twitch::Service reopened(cleanCore,filename,onboarding.endpoints());waitUntil([&]{return reopened.status().contains("13/13");});require(onboarding.deviceRequests==1 && onboarding.sockets==2 && !onboarding.leakedSecret,"Saved grant lost during onboarding restart");reopened.stop();}
	 cleanCore->stop();
	}
	QTemporaryDir dir;require(dir.isValid(),"Temp path");auto daily=std::make_unique<Daily>();auto *storage=daily.get();auto log=std::make_unique<Log>();auto *logs=log.get();auto core=std::make_shared<PluginCore>(std::make_unique<Config>(),std::move(log),std::move(daily));core->start();core->setDiagnosticsEnabled(true);
	Mock mock;twitch::Service service(core,dir.filePath("session.bin"),mock.endpoints());core->simulateEvent(events::EventType::Follow);
	service.connectAccount("publicclient123456");waitUntil([&]{return !service.code().isEmpty();});require(service.verificationUrl()=="https://www.twitch.tv/activate","Activation URL");
	waitUntil([&]{return service.status().contains("13/13");});require(mock.sockets==1 && mock.subscriptions==13 && mock.polls==2 && !mock.leakedSecret,"OAuth/EventSub shared session");
	const auto existingOwner=std::shared_ptr<twitch::Service>(&service,[](auto *){});
    twitch::publishSharedService(existingOwner);
    struct SharedGuard {~SharedGuard(){twitch::unpublishSharedService();}} sharedGuard;
    const auto *sharedApi=nexus_twitch_core_get_api(NEXUS_TWITCH_ABI,sizeof(NexusTwitchApi));require(sharedApi,"Shared ABI missing");
    dst::TwitchCoreClient titleClient,otherClient;
    require(titleClient.attach(sharedApi)&&otherClient.attach(sharedApi),"Two external consumers could not attach");
    NexusTwitchMessage titleState{},otherState{};
    require(titleClient.snapshot(titleState)&&otherClient.snapshot(otherState)&&titleState.instance==otherState.instance&&titleState.authorized&&mock.sockets==1&&mock.subscriptions==13,"Acquisition duplicated OAuth/EventSub or exposed a different owner");
    bool channelReply=false;auto readLease=sharedApi->acquire(nullptr,nullptr);
    int deniedWrite=0;const QByteArray deniedPayload="{\"title\":\"Denied\",\"game_id\":\"509660\",\"tags\":[]}";
    require(sharedApi->update_channel(readLease,deniedPayload.constData(),deniedPayload.size(),[](void *context,int status,const char *,uint32_t){*static_cast<int*>(context)=status;},&deniedWrite),"Permission check not dispatched");
    waitUntil([&]{return deniedWrite!=0;});require(deniedWrite==403&&mock.channelWrites==0,"Missing broadcast permission still wrote channel");
    require(sharedApi->channel_info(readLease,[](void *context,int status,const char *bytes,uint32_t size){require(status==200&&QByteArray(bytes,size).contains("Just Chatting"),"Shared authenticated Helix read failed");*static_cast<bool*>(context)=true;},&channelReply),"Shared Helix request rejected");
    waitUntil([&]{return channelReply;});sharedApi->release(readLease);
    titleClient.detach();require(service.authorized()&&mock.sockets==1&&otherClient.snapshot(otherState),"Client release stopped another consumer");
    int badgeReplies=0;for(const auto route:{"oto://badges/global","oto://badges/123"})service.fetchChatResource(route,[&](auto reply){require(reply.status==200 && !reply.networkError,"Badge catalog GET failed");++badgeReplies;});
	waitUntil([&]{return badgeReplies==2;});require(mock.badgeRequests==2 && mock.badgeAuth && mock.sockets==1 && mock.subscriptions==13,"Badges did not reuse authenticated transport");
	bool unsafeDone=false;service.fetchChatResource("oto://badges/123?redirect=https://evil.example",[&](auto reply){require(reply.status==0,"Unsafe catalog route accepted");unsafeDone=true;});waitUntil([&]{return unsafeDone;});require(mock.badgeRequests==2,"Unsafe catalog fetched");
	bool cheersDone=false;service.fetchChatResource("oto://cheermotes/123",[&](auto reply){require(reply.status==200 && !reply.networkError,"Cheermote catalog GET failed");cheersDone=true;});waitUntil([&]{return cheersDone;});require(mock.badgeRequests==3 && mock.badgeAuth,"Cheermote endpoint/credentials incorrect");
	waitUntil([&]{return core->sharedSnapshot()->channels.count({events::EventOrigin::Twitch,"123"})!=0;});
	const auto restoredHistory=core->sharedSnapshot()->channels.at({events::EventOrigin::Twitch,"123"});
	require(mock.historyRequests==1 && restoredHistory->activity.size()==1 && restoredHistory->activity[0].displayName && *restoredHistory->activity[0].displayName=="FixtureViewer" && restoredHistory->activityDelivery==0 && restoredHistory->daily.bits==0 && restoredHistory->daily.subs==0,"Startup follower history did not arrive quietly through shared transport");
	const auto liveRoute=core->selectedChannel();const auto persisted=state::dailyBook(core->sharedSnapshot());
	core->simulatePanelEvent(events::EventType::Cheer);core->simulatePanelEvent(events::EventType::Shoutout);
	require(core->selectedChannel()==liveRoute && service.active() && mock.sockets==1 && mock.subscriptions==13 && mock.chatWrites==0 && core->sharedSnapshot()->channels.at(liveRoute)==restoredHistory && state::dailyBook(core->sharedSnapshot())==persisted,"Panel TEST changed active Twitch session, history or persisted totals");
	mock.cheer("during-panel-test",50);waitUntil([&]{return core->sharedSnapshot()->channels.at(liveRoute)->daily.bits==50;});
	require(core->sharedSnapshot()->channels.at(core->displayChannel())->daily.bits==100,"Live notification contaminated panel TEST");core->endPanelTest();
	require(core->displayChannel()==liveRoute && core->sharedSnapshot()->channels.at(liveRoute)->daily.bits==50 && mock.chatWrites==0,"Panel cleanup lost received Twitch event");
	auto assets=std::make_shared<visual::AssetManager>(dir.path());service.setProfileAssets(assets);int imagesReady=0;service.setProfilesChanged([&]{++imagesReady;});mock.avatarDelay=120;
	std::optional<events::User> resolved;bool done=false;
	const auto resolve=[&](QString login){done=false;resolved.reset();service.resolvePreviewUser(login,[&](auto user){resolved=std::move(user);done=true;});waitUntil([&]{return done;});};
	resolve(" @VIEWER ");require(resolved && resolved->id=="456" && resolved->displayName=="FixtureViewer","TEST did not resolve actual actor by login");
	resolve("raider");require(resolved && resolved->id=="789","Different TEST user reused previous identity");
	resolve("missing_user");require(!resolved,"Unknown TEST user fell back to connected account");const auto calls=mock.loginLookups;
	resolve("https://twitch.tv/viewer");require(!resolved && mock.loginLookups==calls && mock.sockets==1 && mock.subscriptions==13,"Invalid TEST login requested network or duplicated EventSub");
	service.requestProfile("456");service.requestProfile("456");require(service.profileStatus("456")==twitch::ProfileStatus::Pending && !service.profileImage("456"),"Alert portrait did not remain pending until download");
	waitUntil([&]{return service.profileStatus("456")==twitch::ProfileStatus::Ready;});require(service.profileImage("456")==assets->profileImage("456") && mock.profileLookups==1 && mock.avatarDownloads==1 && !mock.avatarAuth,"Profile request dedup, shared cache or CDN credential separation failed");
	service.requestProfile("456");require(mock.profileLookups==1 && mock.avatarDownloads==1,"Cached user fetched again");
	mock.badAvatar=true;service.requestProfile("999");waitUntil([&]{return service.profileStatus("999")==twitch::ProfileStatus::Unavailable;});require(service.active() && service.status().contains("13/13"),"Malformed photo disconnected EventSub");service.requestProfile("999");require(mock.profileLookups==2,"Failed image retried without backoff");mock.badAvatar=false;
	mock.cheer("cheer-1",50);mock.cheer("cheer-1",50);waitUntil([&]{const auto found=core->sharedSnapshot()->channels.find({events::EventOrigin::Twitch,"123"});return found!=core->sharedSnapshot()->channels.end()&&found->second->daily.bits==100;});
	require(core->pendingEventCount()==1,"Automatic consumer stole simulation");
	mock.chat();mock.chat();waitUntil([&]{return core->sharedSnapshot()->channels.at({events::EventOrigin::Twitch,"123"})->delivered[state::slot(events::EventType::Chat)]==1;});
	require(core->sharedSnapshot()->channels.at({events::EventOrigin::Twitch,"123"})->daily.bits==100,"Chat changed bits");
	bool chatLogged=false;for(const auto &line:logs->lines){chatLogged=chatLogged||line.find("Twitch event submitted: Chat; engine error 0")!=std::string::npos;
		require(line.find("private-chat-content")==std::string::npos && line.find("PrivateName")==std::string::npos && line.find("authorized-access")==std::string::npos && line.find("authorized-refresh")==std::string::npos,"Diagnostics leaked private data");}
	require(chatLogged,"Chat diagnostic missing");core->setDiagnosticsEnabled(false);const auto quietFrom=logs->lines.size();
	int sharedRaids=0,sharedBegin=0,sharedEnd=0;
    otherClient.setChanged([&](uint32_t kind){if(kind==NEXUS_RAID)++sharedRaids;if(kind==NEXUS_HYPE_BEGIN)++sharedBegin;if(kind==NEXUS_HYPE_END)++sharedEnd;});
    const QJsonObject raidFixture{{"to_broadcaster_user_id","123"},{"from_broadcaster_user_id","789"},{"from_broadcaster_user_login","fixtureraider"},{"from_broadcaster_user_name","FixtureRaider"},{"viewers",3}};
    bool hypeBeginMetadata=false,hypeEndMetadata=false;bool raidMetadata=false;otherClient.setMessage([&](const NexusTwitchMessage &message){if(message.kind==NEXUS_RAID)raidMetadata=QString::fromUtf8(message.actor)=="fixtureraider"&&QString::fromUtf8(message.actor_id)=="789";if(message.kind==NEXUS_HYPE_BEGIN)hypeBeginMetadata=QString::fromUtf8(message.train_id)=="train-fixture"&&message.train_started_ms==QDateTime::fromString("2026-10-04T08:00:00Z",Qt::ISODate).toMSecsSinceEpoch();if(message.kind==NEXUS_HYPE_END)hypeEndMetadata=QString::fromUtf8(message.train_id)=="train-fixture"&&message.train_started_ms>0;});
    mock.notify("channel.raid","shared-raid",raidFixture);mock.notify("channel.raid","shared-raid",raidFixture);
    mock.notify("channel.hype_train.begin","shared-begin",{{"level",2},{"total",40},{"goal",100}});
    mock.notify("channel.hype_train.end","shared-end",{{"level",2},{"total",50}});
    waitUntil([&]{return sharedRaids==1&&sharedBegin==1&&sharedEnd==1;});
    require(raidMetadata&&hypeBeginMetadata&&hypeEndMetadata&&mock.sockets==1&&mock.subscriptions==13,"Raid login/ID missing or shared events duplicated connections");
    otherClient.setChanged({});
    mock.migrate();waitUntil([&]{return mock.sockets==2 && service.status().contains("13/13");});require(mock.subscriptions==13,"Migration duplicated subscriptions");
	require(mock.historyRequests==1,"Socket migration queried follower history again");
	mock.cheer("cheer-1",100);mock.cheer("migration-event",25);waitUntil([&]{return storage->book.at({events::EventOrigin::Twitch,"123"}).bits==125;});
	mock.ws->disconnectFromHost();waitUntil([&]{return mock.sockets==3 && mock.subscriptions==26 && service.status().contains("13/13");});
	storage->fail=true;mock.cheer("cheer-2",50);waitUntil([&]{return !service.active();});require(core->pendingEventCount()==2 && storage->book.at({events::EventOrigin::Twitch,"123"}).bits==125,"Persistence failure lost transaction");
	storage->fail=false;service.retryBatch();waitUntil([&]{return storage->book.at({events::EventOrigin::Twitch,"123"}).bits==175;});
	for(auto i=quietFrom;i<logs->lines.size();++i)require(logs->lines[i].find("Twitch event submitted")==std::string::npos && logs->lines[i].find("Twitch batch applied")==std::string::npos && logs->lines[i].find("Twitch session welcome")==std::string::npos,"Disabled diagnostics still logged");
	service.disconnectAccount();require(core->selectedChannel().origin==events::EventOrigin::Simulation,"Disconnect routing");service.stop();
	twitch::SessionStore saved(dir.filePath("session.bin"));twitch::Session session;require(saved.load(session)==twitch::SessionLoad::Loaded && !session.enabled,"Disconnect persisted");
	session.access="expired-access";session.refresh="synthetic-single-use-refresh";session.enabled=true;require(saved.save(session),"Refresh fixture save");
	{twitch::Service resumed(core,dir.filePath("session.bin"),mock.endpoints());waitUntil([&]{return resumed.status().contains("13/13");});require(mock.refreshes==1 && mock.sockets==4 && mock.subscriptions==39,"Startup validation/refresh/subscriptions");
		require(saved.load(session)==twitch::SessionLoad::Loaded && session.refresh=="rotated-refresh","Single-use refresh rotation not persisted");resumed.stop();}
	// Exercise the actual expiry timer while EventSub is already connected.
	for(int rejected=0;rejected<2;++rejected){
		const auto path=dir.filePath(QStringLiteral("active-renewal-%1.bin").arg(rejected));twitch::SessionStore fixture(path);
		require(fixture.save({"publicclient123456","synthetic-short-access","synthetic-refresh",true}),"Active renewal fixture");
		Mock renewing;renewing.accessLifetime=61;renewing.denyRefresh=rejected!=0;
		twitch::Service connected(core,path,renewing.endpoints());waitUntil([&]{return connected.status().contains("13/13");});
		if(rejected){
			waitUntil([&]{return !connected.active();});
			require(connected.status().contains(i18n::qtext("Status.RenewFailed")) && renewing.refreshes==1 && renewing.validations==1 && renewing.sockets==1 && renewing.subscriptions==13,"Rejected active renewal retried or retained the connection");
			require(fixture.load(session)==twitch::SessionLoad::Loaded && !session.enabled && session.access.isEmpty() && session.refresh.isEmpty(),"Rejected active renewal retained invalid credentials");
		}else{
			waitUntil([&]{return renewing.refreshes==1 && renewing.validations==2;});
			waitUntil([&]{return connected.canRenewAuthorization() && fixture.load(session)==twitch::SessionLoad::Loaded && session.refresh=="rotated-refresh";});
			require(connected.active() && connected.status().contains("13/13") && renewing.sockets==1 && renewing.subscriptions==13,"Active renewal duplicated the shared socket or subscriptions");
			renewing.cheer("after-active-renewal",7);waitUntil([&]{return storage->book.at({events::EventOrigin::Twitch,"123"}).bits==182;});
			require(core->sharedSnapshot()->channels.at({events::EventOrigin::Twitch,"123"})->daily.bits==182,"Active renewal stopped event delivery");
		}
		connected.stop();
	}
	const auto manualFile=dir.filePath("manual-renewal.bin");twitch::SessionStore manualStore(manualFile);
	require(manualStore.save({"publicclient123456","synthetic-valid-access","synthetic-refresh",true}),"Manual renewal fixture");
	{Mock manual;twitch::Service connected(core,manualFile,manual.endpoints());core->setDiagnosticsEnabled(true);
		waitUntil([&]{return connected.canRenewAuthorization();});connected.renewAuthorization();connected.renewAuthorization();
		require(!connected.canRenewAuthorization(),"Manual renewal remained enabled in flight");
		waitUntil([&]{return manual.refreshes==1 && connected.canRenewAuthorization();});
		require(manual.sockets==1 && manual.subscriptions==13 && connected.status().contains("13/13"),"Manual renewal duplicated the shared session");
		require(manualStore.load(session)==twitch::SessionLoad::Loaded && session.refresh=="rotated-refresh","Manual rotation not persisted");
		bool persisted=false,validated=false;for(const auto &line:logs->lines){persisted=persisted||line=="Twitch authorization renewal persisted";validated=validated||line=="Twitch renewed authorization validated";
			require(line.find("fresh-access")==std::string::npos && line.find("rotated-refresh")==std::string::npos,"Renewal diagnostics leaked credentials");}
		require(persisted && validated,"Manual renewal diagnostic evidence missing");connected.disconnectAccount();const auto attempts=manual.refreshes;
		connected.renewAuthorization();require(!connected.canRenewAuthorization() && manual.refreshes==attempts,"Disconnected renewal was allowed");connected.stop();core->setDiagnosticsEnabled(false);}
	// Repeated revocations must not subtract unrelated active subscriptions.
	const auto limitedFile=dir.filePath("limited.bin");twitch::SessionStore limitedStore(limitedFile);
	require(limitedStore.save({"publicclient123456","synthetic-valid-access","synthetic-refresh",true}),"Limited session fixture");
	Mock limited;limited.deniedType="channel.follow";
	{twitch::Service reduced(core,limitedFile,limited.endpoints());waitUntil([&]{return reduced.status().contains("12/13");});
		require(reduced.active() && limited.sockets==1 && limited.subscriptions==13,"Partial permissions stopped available subscriptions");
		limited.revoke("version_removed");limited.revoke("version_removed");
		waitUntil([&]{return reduced.status().contains("11/13");});
		limited.revoke("version_removed","channel.follow","2");limited.revoke("version_removed","channel.cheer","99");
		QEventLoop settle;QTimer::singleShot(30,&settle,&QEventLoop::quit);settle.exec();
		require(reduced.status().contains("11/13"),"Duplicate revocation subtracted an unrelated subscription");
		limited.revoke("authorization_revoked");waitUntil([&]{return !reduced.active();});
		require(limitedStore.load(session)==twitch::SessionLoad::Loaded && !session.enabled && session.access.isEmpty() && session.refresh.isEmpty(),"Revocation left enabled credentials");reduced.stop();}
	for(int scenario=0;scenario<3;++scenario){
		const auto path=dir.filePath(QStringLiteral("denied-%1.bin").arg(scenario));twitch::SessionStore fixture(path);
		require(fixture.save({"publicclient123456",scenario==2?"expired-access":"synthetic-valid-access","synthetic-refresh",true}),"Denied fixture");
		Mock denied;denied.missingScopes=scenario==0;denied.denyAll=scenario==1;denied.denyRefresh=scenario==2;
		twitch::Service rejected(core,path,denied.endpoints());waitUntil([&]{return !rejected.active() && !rejected.status().contains(i18n::qtext("Status.Disconnected"));});
		require(denied.sockets==(scenario==1?1:0) && denied.subscriptions==(scenario==1?13:0) && denied.refreshes==(scenario==2?1:0),"Denied authorization retried or opened an unexpected socket");
		require(fixture.load(session)==twitch::SessionLoad::Loaded,"Denied session lost storage");
		if(scenario!=1)require(!session.enabled && session.access.isEmpty() && session.refresh.isEmpty(),"Invalid authorization retained credentials");rejected.stop();
	}
	const auto emptyFile=dir.filePath("all-revoked.bin");twitch::SessionStore emptyStore(emptyFile);
	require(emptyStore.save({"publicclient123456","synthetic-valid-access","synthetic-refresh",true}),"All revoked fixture");
	{Mock revoked;twitch::Service emptied(core,emptyFile,revoked.endpoints());waitUntil([&]{return emptied.status().contains("13/13");});
		for(const auto &item:twitch::subscriptions("123","synthetic-session")){const auto subscription=item.toObject();revoked.revoke("version_removed",subscription.value("type").toString(),subscription.value("version").toString());}
		waitUntil([&]{return !emptied.active();});require(emptied.status().contains(i18n::qtext("Status.AllDisabled")),"Zero active subscriptions still reported connected");
		require(emptyStore.load(session)==twitch::SessionLoad::Loaded && !session.access.isEmpty(),"Version removal erased valid authorization");emptied.stop();}
	core->stop();
	// All ten notifications traverse the real transport/service/queue/store path.
	// Fixtures stay on localhost and in a temporary directory, outside OBS data.
	const auto completeDaily=dir.filePath("complete-daily.json"),completeSession=dir.filePath("complete-session.bin");
	twitch::SessionStore completeStore(completeSession);
	require(completeStore.save({"publicclient123456","synthetic-valid-access","synthetic-refresh",true}),"Complete session fixture");
	Mock complete;
	sources::BarLabels labels{{"Follow","Sub","Cheer","Gift","Bits","Subs"},"Empty","Anonymous"};
	std::array<QString,6> displayed;
	{
		auto liveCore=std::make_shared<PluginCore>(std::make_unique<Config>(),std::make_unique<Log>(),std::make_unique<JsonDailyStore>(completeDaily));liveCore->start();
		twitch::Service connected(liveCore,completeSession,complete.endpoints());waitUntil([&]{return connected.status().contains("13/13");});
		QJsonObject viewer{{"user_id","456"},{"user_name","FixtureViewer"},{"tier","1000"},{"is_gift",false},{"cumulative_months",14}};
		const std::vector<std::pair<QString,QJsonObject>> notifications{
			{"channel.follow",viewer},{"channel.subscribe",viewer},{"channel.subscription.message",viewer},
			{"channel.subscription.gift",{{"is_anonymous",true},{"total",2},{"tier","1000"}}},
			{"channel.cheer",{{"is_anonymous",true},{"bits",7}}},
			{"channel.raid",{{"to_broadcaster_user_id","123"},{"from_broadcaster_user_id","789"},{"from_broadcaster_user_name","FixtureRaider"},{"viewers",3}}},
			{"channel.chat.message",{{"chatter_user_id","456"},{"chatter_user_name","FixtureViewer"},{"message",QJsonObject{{"text","fixture chat"}}}}},
			{"channel.hype_train.begin",{{"level",2},{"total",40},{"goal",100}}},
			{"channel.hype_train.progress",{{"level",2},{"total",50},{"goal",100}}},
			{"channel.hype_train.end",{{"level",2},{"total",50}}}};
		for(const auto &[type,event]:notifications){complete.notify(type,type+"-e2e",event);complete.notify(type,type+"-e2e",event);}
		waitUntil([&]{const auto snapshot=liveCore->sharedSnapshot();const auto found=snapshot->channels.find({events::EventOrigin::Twitch,"123"});
			if(found==snapshot->channels.end())return false;std::uint64_t count=0;for(const auto n:found->second->delivered)count+=n;return count==10;});
		const auto channel=liveCore->sharedSnapshot()->channels.at({events::EventOrigin::Twitch,"123"});
		require(channel->daily.bits==7 && channel->daily.subs==3 && channel->sessionBits==7 && channel->sessionSubs==3,"Complete delivery counted duplicates or resub messages");
		require(channel->delivered[state::slot(events::EventType::Follow)]==1 && channel->delivered[state::slot(events::EventType::Sub)]==2 &&
			channel->delivered[state::slot(events::EventType::Cheer)]==1 && channel->delivered[state::slot(events::EventType::Gift)]==1 &&
			channel->delivered[state::slot(events::EventType::Raid)]==1 && channel->delivered[state::slot(events::EventType::Chat)]==1 &&
			channel->delivered[state::slot(events::EventType::HypeTrain)]==3,"Complete service delivery omitted an event type");
		displayed=sources::supporterValues(liveCore->sharedSnapshot(),state::localDay(std::chrono::system_clock::now()),labels,{},liveCore->selectedChannel());
		require(displayed[0]=="FixtureViewer" && displayed[1]=="FixtureViewer" && displayed[2].contains("Anonymous") && displayed[3].contains("Anonymous") && displayed[4]=="7" && displayed[5]=="3","Complete service delivery did not reach source values");
		require(complete.sockets==1 && complete.subscriptions==13 && liveCore->pendingEventCount()==0,"Complete service duplicated connections or left pending events");
		connected.stop();liveCore->stop();
	}
	{
		auto restoredCore=std::make_shared<PluginCore>(std::make_unique<Config>(),std::make_unique<Log>(),std::make_unique<JsonDailyStore>(completeDaily));restoredCore->start();
		twitch::Service resumed(restoredCore,completeSession,complete.endpoints());waitUntil([&]{return resumed.status().contains("13/13");});
		const auto restoredValues=sources::supporterValues(restoredCore->sharedSnapshot(),state::localDay(std::chrono::system_clock::now()),labels,{},restoredCore->selectedChannel());
		require(restoredValues==displayed,"Service restart lost latest users or daily totals");
		const auto channel=restoredCore->sharedSnapshot()->channels.at({events::EventOrigin::Twitch,"123"});
		for(const auto count:channel->delivered)require(count==0,"Service restart replayed a delivered event");
		require(channel->sessionBits==0 && channel->sessionSubs==0 && restoredCore->pendingEventCount()==0 && complete.sockets==2 && complete.subscriptions==26,"Service restart duplicated events or connections");
		resumed.stop();restoredCore->stop();
	}
	// One real service path, entirely local: role check, mandatory @, resolution,
        // dedup, committed central event and one outbound message. No live chat post.
        {
            auto core=std::make_shared<PluginCore>(std::make_unique<Config>(),std::make_unique<Log>(),std::make_unique<Daily>());core->start();
            Mock so;so.writeChat=true;
            const auto path=dir.filePath("shoutout.bin");twitch::SessionStore fixture(path);
            require(fixture.save({"publicclient123456","synthetic-so-access","synthetic-so-refresh",true}),"SO session fixture");
            twitch::Service connected(core,path,so.endpoints());connected.setProfileAssets(assets);
            waitUntil([&]{return connected.status().contains("13/13");});
            require(connected.canSendChat(),"Write scope not recognized");
            const auto command=[&](QString id,QString author,QString text){so.notify("channel.chat.message",id,{{"chatter_user_id",author},{"chatter_user_name","Actor"},{"message",QJsonObject{{"text",text}}}});};
            command("so-disabled","123","!so @viewer");
            waitUntil([&]{const auto found=core->sharedSnapshot()->channels.find({events::EventOrigin::Twitch,"123"});if(found==core->sharedSnapshot()->channels.end())return false;const auto event=found->second->latest[state::slot(events::EventType::Chat)].event;return event && event->data().id=="so-disabled";});
            require(so.loginLookups==0 && so.chatWrites==0,"Hidden/absent card did network work");
            connected.setShoutoutsEnabled(true);
            command("so-no-mention","123","!so viewer");command("so-viewer","456","!so @viewer");
            waitUntil([&]{return core->sharedSnapshot()->channels.at({events::EventOrigin::Twitch,"123"})->latest[state::slot(events::EventType::Chat)].event->data().id=="so-viewer";});
            require(so.loginLookups==0 && so.chatWrites==0,"Unprivileged or mention-free command accepted");
            const auto before=state::dailyBook(core->sharedSnapshot());
            command("so-valid","123","!so @viewer");command("so-valid","123","!so @viewer");
            command("so-burst-2","123","!so @raider");command("so-burst-3","123","!so @viewer");
            waitUntil([&]{return so.chatWrites==1;});
            const auto state=core->sharedSnapshot()->channels.at({events::EventOrigin::Twitch,"123"});
            require(so.loginLookups==1 && state->shoutouts.size()==1 && so.sentChat==twitch::shoutoutMessage("viewer") && so.sockets==1 && so.subscriptions==13,"SO duplicate/network/queue/chat contract failed");
            const auto &data=std::get<events::ShoutoutEvent>(state->shoutouts[0].event->data().payload);require(data.category=="Just Chatting" && data.followers==125000,"SO channel details missing");
            const auto lookupCount=so.loginLookups,historyCount=so.historyRequests;bool ready=false;connected.resolveShoutoutUser("@viewer",[&](auto user){require(user && user->followers==125000,"Cached SO metadata missing");ready=true;});waitUntil([&]{return ready;});require(so.loginLookups==lookupCount && so.historyRequests==historyCount,"SO metadata wasn't shared from cache");
            waitUntil([&]{return so.chatWrites==2;});waitUntil([&]{return so.chatWrites==3;});
            const auto burst=core->sharedSnapshot()->channels.at({events::EventOrigin::Twitch,"123"});
            require(burst->shoutouts.size()==3 && so.loginLookups==2 && so.sockets==1,"Burst commands were discarded while lookup/cooldown was active");
            require(std::get<events::ShoutoutEvent>(burst->shoutouts[1].event->data().payload).login=="raider" && std::get<events::ShoutoutEvent>(burst->shoutouts[2].event->data().payload).login=="viewer","Burst SO order/metadata changed");
            for(int n=0;n<256;++n){events::EventDraft full{"so-full-"+std::to_string(n),"123",std::chrono::system_clock::now(),events::EventOrigin::Twitch,events::FollowEvent{{"789","FloodViewer"}}};require(bool(core->processEvent(std::move(full))),"Full queue fixture admission failed");}
            command("so-backpressure","123","!so @viewer");waitUntil([&]{return so.chatWrites==4;});
            require(connected.active() && so.sockets==1 && core->sharedSnapshot()->channels.at({events::EventOrigin::Twitch,"123"})->shoutouts.size()==4,"Full chat queue disconnected/dropped a valid SO");
            require(state::dailyBook(core->sharedSnapshot())==before,"SO changed counters");
            connected.stop();core->stop();
        }
        // Hype recovery is demand-driven and shares this same Service/transport.
        core->start();
        const auto hypeFile=dir.filePath("hype-session.bin");twitch::SessionStore hypeStore(hypeFile);
        require(hypeStore.save({"publicclient123456","synthetic-valid-access","synthetic-refresh",true}),"Hype session fixture");
        Mock hypeMock;
        QJsonObject current{{"id","recovery-train"},{"broadcaster_user_id","123"},{"level",3},{"total",18000},{"progress",4500},{"goal",10000},
            {"started_at",QDateTime::currentDateTimeUtc().addSecs(-60).toString(Qt::ISODateWithMs)},
            {"expires_at",QDateTime::currentDateTimeUtc().addSecs(120).toString(Qt::ISODateWithMs)},{"top_contributions",QJsonArray{}}};
        hypeMock.hypeCurrent=current;
        {
            twitch::Service meter(core,hypeFile,hypeMock.endpoints());waitUntil([&]{return meter.status().contains("13/13");});
            require(hypeMock.hypeRequests==0,"No meter but Hype status queried");
            meter.setHypeMeterEnabled(true);
            waitUntil([&]{const auto c=core->sharedSnapshot()->channels.find({events::EventOrigin::Twitch,"123"});return c!=core->sharedSnapshot()->channels.end() && c->second->hypeTrain && c->second->hypeTrain->trainId=="recovery-train";});
            require(core->sharedSnapshot()->channels.at({events::EventOrigin::Twitch,"123"})->hypeTrain->progress==4500,"Recovery confused total and current level");
            meter.setHypeMeterEnabled(false);meter.setHypeMeterEnabled(true);require(hypeMock.hypeRequests==1 && hypeMock.sockets==1,"Hype meter created polling or another socket");
            hypeMock.hypeDelay=100;current.insert("progress",100);hypeMock.hypeCurrent=current;
            meter.connectAccount("publicclient123456");waitUntil([&]{return hypeMock.hypeRequests==2;});
            // Model a genuinely newer server notification. Windows Qt and
            // system_clock can sample different clock resolutions; rounding
            // down in the mock occasionally made this event older than the request.
            const auto newerMs=std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count()+1;
            current.insert("progress",8000);hypeMock.notify("channel.hype_train.progress","newer-live-hype",current,QDateTime::fromMSecsSinceEpoch(newerMs).toUTC().toString(Qt::ISODateWithMs));
            waitUntil([&]{return core->sharedSnapshot()->channels.at({events::EventOrigin::Twitch,"123"})->hypeTrain->progress==8000;});
            QEventLoop recoveryWait;QTimer::singleShot(150,&recoveryWait,&QEventLoop::quit);recoveryWait.exec();
            require(core->sharedSnapshot()->channels.at({events::EventOrigin::Twitch,"123"})->hypeTrain->progress==8000,"Late API response regressed live progress");meter.stop();
        }
        core->stop();
        // All moderation notifications traverse the existing websocket,
        // central queue and shared state; no action is sent to a real channel.
        auto moderationCore=std::make_shared<PluginCore>(std::make_unique<Config>(),std::make_unique<Log>());moderationCore->start();
        const auto moderationFile=dir.filePath("moderation.bin");twitch::SessionStore moderationStore(moderationFile);
        require(moderationStore.save({"publicclient123456","synthetic-valid-access","synthetic-refresh",true}),"Moderation session fixture");
        {Mock moderation;twitch::Service reader(moderationCore,moderationFile,moderation.endpoints());waitUntil([&]{return reader.status().contains("13/13");});
         const state::ChannelKey room{events::EventOrigin::Twitch,"123"};const auto clock=QDateTime::currentDateTimeUtc();
         const auto at=[&](int second){return clock.addSecs(second).toString(Qt::ISODateWithMs);};
         const auto send=[&](QString id,QString user,int second){moderation.notify("channel.chat.message","transport-"+id,{{"message_id",id},{"chatter_user_id",user},{"chatter_user_name",user},{"message",QJsonObject{{"text","Moderation fixture"}}}},at(second));};
         const auto count=[&]{const auto snapshot=moderationCore->sharedSnapshot();const auto found=snapshot->channels.find(room);return found==snapshot->channels.end()?0:found->second->chat.size();};
         send("message-a","456",0);send("message-b","789",0);waitUntil([&]{return count()==2;});
         moderation.notify("channel.chat.message_delete","delete-a",{{"message_id","message-a"}},at(1));waitUntil([&]{return count()==1;});
         require(std::get<events::ChatEvent>(moderationCore->sharedSnapshot()->channels.at(room)->chat.front().event->data().payload).messageId=="message-b","Delete used EventSub transport ID");
         moderation.notify("channel.chat.clear_user_messages","timeout-b",{{"target_user_id","789"}},at(2));waitUntil([&]{return count()==0;});
         send("future-b","789",3);waitUntil([&]{return count()==1;});
         moderation.notify("channel.chat.clear","clear-room",{},at(4));waitUntil([&]{return count()==0;});
         send("late-message","456",0);moderation.notify("channel.chat.message_delete","duplicate-delete",{{"message_id","never-arrived"}},at(5));
         waitUntil([&]{return moderationCore->sharedSnapshot()->channels.at(room)->delivered[state::slot(events::EventType::ChatModeration)]==4;});
         require(count()==0 && moderation.sockets==1 && moderation.subscriptions==13 && moderation.chatWrites==0 && moderationCore->sharedSnapshot()->channels.at(room)->daily.bits==0 && moderationCore->sharedSnapshot()->channels.at(room)->daily.subs==0,"Moderation resurrected chat or duplicated connection/writes/totals");reader.stop();}
        moderationCore->stop();
        std::cout<<"PASS: device pending, authorization, shared session, subscriptions, migration, reconnect, dedup, persistence retry, disconnect, startup/active refresh rotation/rejection, delivery after renewal, partial/missing permissions, repeated/unknown revocations, zero active subscriptions, activity notifications through transport and full persistence restart; all three chat moderation notifications on shared transport\n";return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}




