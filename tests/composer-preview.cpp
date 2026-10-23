// SPDX-License-Identifier: GPL-2.0-or-later
// Local visual fixture only; no Core, network or publication callbacks.
#include "dock.hpp"
#include "ui-language.hpp"
#include <QApplication>
#include <QLabel>
#include <QPlainTextEdit>
int main(int argc,char **argv){
 QApplication app(argc,argv);dst::setUiLanguage("es");dst::Dock dock;
 dock.setWindowTitle(QString::fromUtf8("Compositor · prueba local sin Twitch"));
 dock.enableLive([]{},[]{});dock.enableEditing([](const dst::ChannelFields &){return false;},[](QString){return false;});
 dock.liveState(true,true,true,QString::fromUtf8("SIMULACIÓN LOCAL · SIN RED"));
 dst::ChannelFields fields;fields.title="NEXUS Live Session";fields.gameId="26936";fields.category="Music";fields.tags={"DJ","Live"};dock.channelLoaded(fields);
 dst::AutomaticBlocks blocks;blocks.hype=true;blocks.raiders={"DemoRaider","AnotherRaider","DEMORAIDER"};dock.automaticStatePreview(blocks);
 dock.findChild<QPlainTextEdit*>("manualDraft")->setPlainText("BORRADOR PENDIENTE - no publicado");
 dock.findChild<QLabel*>("demoNotice")->setText(QString::fromUtf8("COMPOSITOR · SIMULACIÓN LOCAL · SIN RED"));
 dock.resize(340,950);dock.show();return app.exec();
}
