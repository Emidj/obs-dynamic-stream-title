// SPDX-License-Identifier: GPL-2.0-or-later
#include "dock.hpp"
#include "ui-language.hpp"
#include <QApplication>
#include <QCheckBox>
#include <QPlainTextEdit>
#include <QElapsedTimer>
#include <iostream>
int main(int argc,char **argv){
 QApplication app(argc,argv);dst::setUiLanguage("es");dst::Dock dock;int writes=0;
 dock.enableLive([]{},[]{});dock.enableEditing([](const dst::ChannelFields &){return false;},[](QString){return false;});
 dock.enableRaiders([&](QString title){++writes;if(title.count("HYPE TRAIN ON")>1||title.count("@raider")>1||title.contains("SECRET"))std::abort();dock.automaticPublicationFinished(204);return true;});dock.enableHype();
 dock.liveState(true,true,true,"LOCAL");dock.setRaidAccount("123");dst::ChannelFields base;base.title="Applied base";base.gameId="26936";base.category="Music";dock.channelLoaded(base);
 dock.findChild<QCheckBox*>("raidersEnabled")->setChecked(true);dock.findChild<QCheckBox*>("hypeEnabled")->setChecked(true);
 dock.findChild<QPlainTextEdit*>("manualDraft")->setPlainText("SECRET DRAFT");
 dock.receiveRaid("123","first","789","raider");QElapsedTimer timer;timer.start();
 for(int i=1;i<=10000;++i){const auto train=QString::number(i);dock.receiveRaid("123",train,"789","raider");dock.receiveHype("123",train,i,true);dock.receiveHype("123",train,i,true);dock.receiveHype("123",train,i,false);dock.receiveHype("123",train,i,true);}
 if(writes!=20001||dock.automaticTitle().text!="Applied base | Thanks Raiders @raider"||dock.manualDraft()!="SECRET DRAFT")return 1;
 std::cout<<"PASS 50000 notifications, 20001 confirmed local writes, zero duplicate raiders, no draft leak; elapsed_ms="<<timer.elapsed()<<"\n";
}
