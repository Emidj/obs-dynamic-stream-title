// SPDX-License-Identifier: GPL-2.0-or-later
#include "dock.hpp"
#include "ui-language.hpp"
#include <QApplication>
#include <QTemporaryDir>
#include <QCheckBox>
int main(int argc,char **argv){
 QApplication app(argc,argv);dst::setUiLanguage(argc>1?QString::fromUtf8(argv[1]):"es");dst::Dock dock;
 dock.setWindowTitle("Dynamic Stream Title · "+dst::uiLanguage()+" · LOCAL");
 dock.enableLive([]{},[]{});dock.enableEditing([](const dst::ChannelFields &){return false;},[](QString){return false;});dock.enableRaiders([](QString){return false;});dock.enableHype();
 QTemporaryDir state;dock.enableProtection(state.filePath("state.json"),[]{});
 dock.liveState(true,true,true,"LOCAL · OFFLINE");dock.setRaidAccount("123");dst::ChannelFields base;base.title="NEXUS Live Session";base.gameId="26936";base.category="Music";base.tags={"DJ","Live"};dock.channelLoaded(base);
 dock.findChild<QCheckBox*>("raidersEnabled")->setChecked(true);dock.findChild<QCheckBox*>("hypeEnabled")->setChecked(true);
 dst::AutomaticBlocks blocks;blocks.hype=true;blocks.raiders={"DemoRaider"};dock.automaticStatePreview(blocks);
 dock.resize(350,850);dock.show();return app.exec();
}
