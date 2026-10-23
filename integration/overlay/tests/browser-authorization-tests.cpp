// SPDX-License-Identifier: GPL-2.0-or-later
#include "twitch/browser-authorization.hpp"
#include "ui/authorization-launch.hpp"
#include <QCoreApplication>
#include <QEventLoop>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QUrlQuery>
#include <iostream>
#include <stdexcept>
using namespace oto;
namespace {
void require(bool value,const char *why){if(!value)throw std::runtime_error(why);}
void waitUntil(std::function<bool()> ready){QEventLoop loop;QTimer timer;QObject::connect(&timer,&QTimer::timeout,&loop,[&]{if(ready())loop.quit();});timer.start(2);QTimer::singleShot(2000,&loop,&QEventLoop::quit);loop.exec();require(ready(),"Receiver test timeout");}
QByteArray request(const QUrl &uri,QByteArray method,QByteArray path,QByteArray body={},QByteArray extra={}){
	QTcpSocket client;QByteArray response;bool done=false;
	QObject::connect(&client,&QTcpSocket::connected,&client,[&]{client.write(method+" "+path+" HTTP/1.1\r\nHost: localhost:"+QByteArray::number(uri.port())+"\r\nContent-Type: application/json\r\nContent-Length: "+QByteArray::number(body.size())+"\r\n"+extra+"\r\n"+body);});
	QObject::connect(&client,&QTcpSocket::readyRead,&client,[&]{response+=client.readAll();});
	QObject::connect(&client,&QTcpSocket::disconnected,&client,[&]{response+=client.readAll();done=true;});
	client.connectToHost(QHostAddress::LocalHost,uri.port());waitUntil([&]{return done;});return response;
}
QByteArray completion(const QByteArray &page){const auto found=QRegularExpression("fetch\\('(/complete/[a-f0-9]{64})'").match(QString::fromUtf8(page));require(found.hasMatch(),"Fragment bridge missing");return found.captured(1).toUtf8();}
QByteArray grant(const QString &state,QString token="fixture-access"){return QJsonDocument(QJsonObject{{"state",state},{"access_token",token},{"token_type","bearer"}}).toJson(QJsonDocument::Compact);}
}
int main(int argc,char **argv){QCoreApplication app(argc,argv);try{
	twitch::BrowserAuthorization receiver;QString token;int completed=0;
	const auto begin=[&](int timeout=300000){return receiver.start("fixtureclient123",{"channel:read:subscriptions","user:read:chat"},[&](QString value){token=value;++completed;},0,timeout);};
	const auto url=begin();QUrlQuery query(url);const auto redirect=receiver.redirectUri();const auto state=query.queryItemValue("state");
	require(ui::AuthorizationLaunch::allowed(url) && url.host()=="id.twitch.tv" && query.queryItemValue("response_type")=="token" && query.queryItemValue("redirect_uri")==redirect.toString() && state.size()==64 && !query.hasQueryItem("client_secret"),"Direct OAuth request incorrect");
	const auto page=request(redirect,"GET","/");const auto path=completion(page);
	require(page.startsWith("HTTP/1.1 200") && page.contains("history.replaceState") && page.contains("Cache-Control: no-store") && page.contains("frame-ancestors 'none'") && !page.contains("fixture-access"),"Fragment/history/privacy bridge failed");
	require(request(redirect,"POST",path,grant("wrong-state")).startsWith("HTTP/1.1 403") && receiver.active() && completed==0,"Wrong state completed authorization");
	require(request(redirect,"POST",path,grant(state),"Origin: https://evil.example\r\n").startsWith("HTTP/1.1 403") && completed==0,"Foreign origin completed authorization");
	require(request(redirect,"POST",path,grant(state),"Host: evil.example\r\n").startsWith("HTTP/1.1 400") && completed==0,"Duplicate Host accepted");
	require(request(redirect,"POST",path,grant(state),"Transfer-Encoding: chunked\r\n").startsWith("HTTP/1.1 403"),"Ambiguous framing accepted");
	require(request(redirect,"POST",path,grant(state,"invalid\r\ntoken")).startsWith("HTTP/1.1 400"),"Invalid bearer token accepted");
	require(request(redirect,"POST",path,grant(state),"Origin: "+redirect.toEncoded()+"\r\n").startsWith("HTTP/1.1 200"),"Valid browser response rejected");
	waitUntil([&]{return completed==1;});require(token=="fixture-access" && !receiver.active(),"Valid response not delivered once or listener retained");
	const auto next=begin();const auto nextUri=receiver.redirectUri();const auto nextPage=request(nextUri,"GET","/");
	require(QUrlQuery(next).queryItemValue("state")!=state && completion(nextPage)!=path,"Authorization secrets reused");
	require(request(nextUri,"POST",path,grant(state)).startsWith("HTTP/1.1 404") && completed==1,"Old attempt accepted by new listener");
	const auto denied=QJsonDocument(QJsonObject{{"state",QUrlQuery(next).queryItemValue("state")},{"error","access_denied"}}).toJson();
	require(request(nextUri,"POST",completion(nextPage),denied).startsWith("HTTP/1.1 200"),"Denial response not handled");waitUntil([&]{return completed==2;});require(token.isEmpty() && !receiver.active(),"Denial authenticated or kept listener");
	begin(20);waitUntil([&]{return completed==3;});require(!receiver.active() && token.isEmpty(),"Authorization timeout failed");
	begin();receiver.stop();QCoreApplication::processEvents();require(completed==3 && !receiver.active(),"Stop invoked stale completion");
	QTcpServer occupied;require(occupied.listen(QHostAddress::LocalHost,0),"Occupied port fixture");require(receiver.start("fixtureclient123",{},[&](QString){++completed;},occupied.serverPort()).isEmpty() && !receiver.active() && completed==3,"Occupied loopback port not rejected");
	std::cout<<"Browser OAuth receiver: success, denial, timeout, replay, origin and framing checks passed\n";return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
