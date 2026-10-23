// SPDX-License-Identifier: GPL-2.0-or-later
#include "twitch/transport.hpp"
#include "twitch/session-store.hpp"
#include <QCoreApplication>
#include <QTcpServer>
#include <QTcpSocket>
#include <QCryptographicHash>
#include <QEventLoop>
#include <QTemporaryDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <iostream>
#include <stdexcept>
using namespace oto::twitch;
namespace {
void require(bool ok,const char *why){if(!ok)throw std::runtime_error(why);}
QByteArray frame(unsigned char type,QByteArray bytes,bool fin=true){
	QByteArray out;out+=char(type|(fin?128:0));
	if(bytes.size()<126)out+=char(bytes.size());else if(bytes.size()<65536){out+=char(126);out+=char(bytes.size()>>8);out+=char(bytes.size());}
	else{out+=char(127);for(int i=7;i>=0;--i)out+=char(quint64(bytes.size())>>(i*8));}return out+bytes;
}
void waitUntil(std::function<bool()> ready){QEventLoop loop;QTimer check;QObject::connect(&check,&QTimer::timeout,&loop,[&]{if(ready())loop.quit();});check.start(5);QTimer::singleShot(3000,&loop,&QEventLoop::quit);loop.exec();require(ready(),"Network test timed out");}
QByteArray pongPayload(const QByteArray &bytes){
	require(bytes.size()>=6 && static_cast<unsigned char>(bytes[0])==0x8a,"Automatic pong missing");
	const auto second=static_cast<unsigned char>(bytes[1]);const int length=second&127;
	require((second&128) && length<=125 && bytes.size()==length+6,"Invalid masked pong");
	QByteArray result;for(int i=0;i<length;++i)result+=char(bytes[i+6]^bytes[2+i%4]);return result;
}
struct Server:QTcpServer {
	std::function<void(QTcpSocket *)> upgraded;int requests=0;QByteArray received;QByteArray response="{}";
	Server(){require(listen(QHostAddress::LocalHost,0),"Loopback listen failed");
		QObject::connect(this,&QTcpServer::newConnection,this,[this]{auto *s=nextPendingConnection();auto bytes=std::make_shared<QByteArray>();
			QObject::connect(s,&QTcpSocket::readyRead,this,[this,s,bytes]{
				if(s->property("upgraded").toBool()){received+=s->readAll();return;}
				*bytes+=s->readAll();if(!bytes->contains("\r\n\r\n"))return;++requests;
				if(bytes->contains("Upgrade: websocket") || bytes->contains("Upgrade: WebSocket")){
					QByteArray key;for(const auto &line:bytes->split('\n'))if(line.toLower().startsWith("sec-websocket-key:"))key=line.mid(line.indexOf(':')+1).trimmed();
					const auto accept=QCryptographicHash::hash(key+"258EAFA5-E914-47DA-95CA-C5AB0DC85B11",QCryptographicHash::Sha1).toBase64();
					s->setProperty("upgraded",true);s->write("HTTP/1.1 101 Switching Protocols\r\nUpgrade: websocket\r\nConnection: Upgrade\r\nSec-WebSocket-Accept: "+accept+"\r\n\r\n");
					if(upgraded)upgraded(s);
				}else{s->write("HTTP/1.1 200 OK\r\nContent-Length: "+QByteArray::number(response.size())+"\r\nConnection: close\r\n\r\n"+response);s->disconnectFromHost();}
			});
		});
	}
	QByteArray url(bool ws=false)const{return QByteArray(ws?"ws://127.0.0.1:":"http://127.0.0.1:")+QByteArray::number(serverPort())+"/";}
};
}
int main(int argc,char **argv){QCoreApplication app(argc,argv);try{
	if(app.arguments().contains("--twitch-welcome")){
		Transport network;bool welcome=false,failed=false;QEventLoop loop;
		network.websocket("wss://eventsub.wss.twitch.tv/ws",[]{},[&](QByteArray bytes){const auto object=QJsonDocument::fromJson(bytes).object();welcome=object.value("metadata").toObject().value("message_type").toString()=="session_welcome" && !object.value("payload").toObject().value("session").toObject().value("id").toString().isEmpty();loop.quit();},[&](SocketClose){failed=true;loop.quit();});
		QTimer::singleShot(20000,&loop,&QEventLoop::quit);loop.exec();network.stop();require(welcome && !failed,"Public Twitch WSS welcome failed");
		std::cout<<"PASS: real Twitch TLS, verified WebSocket upgrade, session welcome; no credentials or subscriptions used\n";return 0;
	}
	QTemporaryDir directory;require(directory.isValid(),"Temp path");SessionStore store(directory.filePath("session.bin"));Session loaded;
	require(store.load(loaded)==SessionLoad::Missing,"Missing store");Session fake{"publicclient123456","synthetic-access-token","synthetic-refresh-token",true};
	require(store.save(fake),"DPAPI save failed");require(store.load(loaded)==SessionLoad::Loaded,"DPAPI load failed");
	require(loaded.access==fake.access && loaded.refresh==fake.refresh,"DPAPI round trip values");
	QFile file(directory.filePath("session.bin"));require(file.open(QIODevice::ReadOnly),"Read protected file");const auto encrypted=file.readAll();file.close();
	require(!encrypted.contains(fake.access.toUtf8()) && !encrypted.contains(fake.refresh.toUtf8()),"Plaintext credential stored");
	require(file.open(QIODevice::WriteOnly),"Tamper fixture");file.write("OTO-TWITCH-DPAPI-1\ninvalid");file.close();require(store.load(loaded)==SessionLoad::Unavailable,"Damaged session accepted");
	Transport network;Server http;bool replied=false;
	network.request(http.url(),{}, {},[&](HttpReply r){replied=r.status==200 && !r.networkError && r.body=="{}";});waitUntil([&]{return replied;});
	http.response=QByteArray(1536*1024,'m');replied=false;
	network.request(http.url(),{}, {},[&](HttpReply r){replied=!r.networkError && r.body==http.response;},2*1024*1024);waitUntil([&]{return replied;});
	bool limited=false;network.request(http.url(),{}, {},[&](HttpReply r){limited=r.networkError && r.body.size()<=1024*1024;});waitUntil([&]{return limited;});
	bool invalidLimit=false;try{network.request(http.url(),{}, {},[](HttpReply){},2*1024*1024+1);}catch(const std::invalid_argument &){invalidLimit=true;}require(invalidLimit,"Unbounded media receive limit accepted");
	Server ws;QStringList messages;bool opened=false,closed=false;
	ws.upgraded=[](QTcpSocket *s){s->write(frame(1,"hel",false)+frame(9,"ping")+frame(0,"lo",true)+frame(1,"second"));};
	const auto id=network.websocket(ws.url(true),[&]{opened=true;},[&](QByteArray data){messages.append(QString::fromUtf8(data));},[&](SocketClose){closed=true;});
	waitUntil([&]{return messages.size()==2;});require(opened && !closed && messages==QStringList{"hello","second"},"Fragmented text/ping interleave");
	waitUntil([&]{return ws.received.size()==10;});require(pongPayload(ws.received)=="ping","Pong payload differs");
	ws.received.clear();const QByteArray subscription(180,'s');require(network.sendText(id,subscription),"Text subscription not queued");waitUntil([&]{return ws.received.size()==188;});require(static_cast<unsigned char>(ws.received[0])==0x81 && static_cast<unsigned char>(ws.received[1])==0xfe,"Text frame not masked/extended");QByteArray decoded;for(int i=0;i<180;++i)decoded+=char(ws.received[i+8]^ws.received[4+i%4]);require(decoded==subscription,"Subscription text changed");require(!network.sendText(id,QByteArray(2049,'x')),"Unbounded text accepted");
	network.cancel(id);
	require(!network.sendText(id,"late"),"Stopped socket accepts text");
	Server idle;bool idleReady=false,idleClosed=false;int closeCode=0;QTcpSocket *idlePeer=nullptr;
	idle.upgraded=[&](QTcpSocket *s){idlePeer=s;s->write(frame(1,"ready"));QTimer::singleShot(100,s,[s]{s->write(frame(9,"delayed-ping"));});};
	network.websocket(idle.url(true),[]{},[&](QByteArray data){idleReady=data=="ready";},[&](SocketClose close){idleClosed=close.cause==SocketCloseCause::Peer;closeCode=close.peerCode;});
	waitUntil([&]{return idleReady;});waitUntil([&]{return idle.received.size()==18;});
	require(!idleClosed && pongPayload(idle.received)=="delayed-ping","Idle socket did not echo ping");
	idlePeer->write(frame(8,QByteArray::fromHex("0fa2")+"synthetic reason"));waitUntil([&]{return idleClosed;});require(closeCode==4002,"Peer close code lost");
	Server huge;bool bounded=false;huge.upgraded=[](QTcpSocket *s){s->write(frame(1,QByteArray(129*1024,'x')));};
	network.websocket(huge.url(true),[]{},[](QByteArray){},[&](SocketClose close){bounded=close.cause==SocketCloseCause::InvalidFrame;});waitUntil([&]{return bounded;});
	QTcpServer silent;require(silent.listen(QHostAddress::LocalHost,0),"Silent server");bool unwanted=false;
	network.request("http://127.0.0.1:"+QByteArray::number(silent.serverPort())+"/",{}, {},[&](HttpReply){unwanted=true;});network.stop();
	QEventLoop settle;QTimer::singleShot(30,&settle,&QEventLoop::quit);settle.exec();require(!unwanted,"Cancelled callback escaped stop");
	std::cout<<"PASS: protected credentials, damaged store, async HTTP, WebSocket fragments, pong, bounded payload and cancellation\n";return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
