#include "dock.hpp"
#include "ui-language.hpp"
#include "state-store.hpp"
#include <QTemporaryDir>
#include "emoji-picker.hpp"
#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QLabel>
#include <QLineEdit>
#include <QListView>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QScrollArea>
#include <QScrollBar>
#include <QTextCursor>
#include <QSet>
#include <iostream>
#define CHECK(test) do {if(!(test)){std::cerr<<"FAIL at line "<<__LINE__<<"\n";return 1;}} while(false)
int main(int argc,char **argv){
 QApplication app(argc,argv);dst::setUiLanguage("es");dst::Dock dock;
 auto *editor=dock.findChild<QPlainTextEdit*>("manualDraft");
 auto *apply=dock.findChild<QPushButton*>("apply");
 auto *preview=dock.findChild<QLabel*>("preview");
 auto *applied=dock.findChild<QLabel*>("appliedPreview");
 auto *connection=dock.findChild<QPushButton*>("connectDemo");
 auto *category=dock.findChild<QComboBox*>("category");
 auto *tags=dock.findChild<QLineEdit*>("tags");
 CHECK(editor&&apply&&preview&&applied&&connection&&category&&tags);
 auto initialApplied=applied->text();
 QString draft=QString::fromUtf8("Directo 🎧 <b>sin HTML</b>");editor->setPlainText(draft);
 category->setCurrentText("Art");tags->setText("Spanish, DJ");
 CHECK(dock.manualDraft()==draft&&preview->text()==draft&&preview->textFormat()==Qt::PlainText);
 CHECK(applied->text()==initialApplied&&!apply->isEnabled());
 for(const auto &name:{"raidersEnabled","hypeEnabled","raidSample","hypeSample"})dock.findChild<QCheckBox*>(name)->setChecked(true);
 CHECK(applied->text().contains("HYPE TRAIN ON")&&applied->text().contains("@DemoRaider"));
 CHECK(!applied->text().contains(draft)&&editor->toPlainText()==draft);
 connection->click();CHECK(apply->isEnabled());apply->click();
 CHECK(applied->text().contains(draft)&&applied->text().contains("Art")&&applied->text().contains("Spanish, DJ")&&!apply->isEnabled());
 editor->setPlainText("   ");CHECK(!apply->isEnabled());editor->setPlainText(draft);
 connection->click();CHECK(!apply->isEnabled());
 dock.automaticPreview("Otro titulo");CHECK(dock.manualDraft()==draft);
 dock.resize(250,460);dock.show();app.processEvents();
 auto *scroll=dock.findChild<QScrollArea*>("dockScroll");
 CHECK(scroll&&scroll->verticalScrollBar()->maximum()>0&&scroll->horizontalScrollBar()->maximum()==0);
 CHECK(scroll->widget()->width()<=scroll->viewport()->width());
 CHECK(editor->width()>0&&editor->width()<dock.width());
 const auto &catalog=dst::emojiCatalog();CHECK(catalog.size()==3953);
 QSet<QString> unique,groups;for(const auto &entry:catalog){unique.insert(entry.emoji);groups.insert(entry.group);}
 CHECK(unique.size()==3953&&groups.size()==10);
 auto *emojiButton=dock.findChild<QPushButton*>("emojiButton");CHECK(emojiButton);
 auto cursor=editor->textCursor();cursor.movePosition(QTextCursor::Start);editor->setTextCursor(cursor);
 emojiButton->click();app.processEvents();
 auto *picker=static_cast<dst::EmojiPicker*>(dock.findChild<QDialog*>());CHECK(picker);
 auto *search=picker->findChild<QLineEdit*>("emojiSearch");auto *list=picker->findChild<QListView*>("emojiList");
 auto *group=picker->findChild<QComboBox*>("emojiCategory");CHECK(search&&list&&group);
 CHECK(list->model()->rowCount()==3953);
 search->setText("corazon");CHECK(list->model()->rowCount()>0&&list->model()->rowCount()<3953);
 search->setText("zzzz-no-emoji");CHECK(list->model()->rowCount()==0);
 search->clear();group->setCurrentIndex(group->findData("Flags"));CHECK(list->model()->rowCount()>200);
 group->setCurrentIndex(group->findData("recent"));CHECK(list->model()->rowCount()==0);
 group->setCurrentIndex(0);search->setText("headphone");CHECK(list->model()->rowCount()==1);
 auto chosen=list->model()->data(list->model()->index(0,0),Qt::UserRole).toString();
 auto previousApplied=applied->text();
 CHECK(QMetaObject::invokeMethod(list,"clicked",Qt::DirectConnection,Q_ARG(QModelIndex,list->model()->index(0,0))));
 CHECK(dock.manualDraft()==chosen+draft&&applied->text()==previousApplied);
 app.sendPostedEvents(nullptr,QEvent::DeferredDelete);app.processEvents();
 emojiButton->click();app.processEvents();picker=static_cast<dst::EmojiPicker*>(dock.findChild<QDialog*>());CHECK(picker);
 group=picker->findChild<QComboBox*>("emojiCategory");list=picker->findChild<QListView*>("emojiList");
 group->setCurrentIndex(group->findData("recent"));CHECK(list->model()->rowCount()==1);
 CHECK(list->model()->data(list->model()->index(0,0),Qt::UserRole).toString()==chosen);
 picker->close();
 int liveConnect=0,liveDisconnect=0;
 dock.enableLive([&]{++liveConnect;},[&]{++liveDisconnect;});
 dock.channelInformation("Canal real de prueba");
 editor->setPlainText("Borrador pendiente");
 CHECK(applied->text()=="Canal real de prueba"&&!apply->isEnabled());
 dock.liveState(false,false,false,"Desconectado");connection->click();CHECK(liveConnect==1);
 dock.liveState(true,true,true,"Conectado");connection->click();CHECK(liveDisconnect==1);
 dock.liveState(true,true,false,"Falta permiso");connection->click();CHECK(liveConnect==2);
 apply->click();CHECK(dock.manualDraft()=="Borrador pendiente"&&applied->text()=="Canal real de prueba");
 dst::Dock manual;manual.enableLive([]{},[]{});int writes=0,searches=0;dst::ChannelFields sent;
 manual.enableEditing([&](const dst::ChannelFields &fields){++writes;sent=fields;return true;},[&](QString query){++searches;return query=="Art";});
 auto *draftEditor=manual.findChild<QPlainTextEdit*>("manualDraft");auto *publish=manual.findChild<QPushButton*>("apply");auto *games=manual.findChild<QComboBox*>("category");auto *tagEdit=manual.findChild<QLineEdit*>("tags");auto *remote=manual.findChild<QLabel*>("appliedPreview");
 manual.liveState(true,true,true,"Conectado");
 dst::ChannelFields original;original.title="Canal inicial";original.category="Music";original.gameId="26936";original.tags={"DJ"};manual.channelLoaded(original);
 CHECK(draftEditor->toPlainText()==original.title&&games->currentData()==original.gameId&&tagEdit->text()=="DJ"&&!publish->isEnabled());
 draftEditor->setPlainText("Primer borrador");CHECK(writes==0&&publish->isEnabled());publish->click();CHECK(writes==1&&!publish->isEnabled()&&sent.title=="Primer borrador");
 draftEditor->setPlainText("Nuevo borrador durante envío");manual.publicationFinished(204);
 CHECK(draftEditor->toPlainText()=="Nuevo borrador durante envío"&&remote->text().startsWith("Primer borrador")&&publish->isEnabled());
 publish->click();manual.publicationFinished(503);CHECK(writes==2&&remote->text().startsWith("Primer borrador")&&publish->isEnabled());
 games->setEditText("Art");CHECK(!publish->isEnabled());manual.findChild<QPushButton*>("searchCategory")->click();CHECK(searches==1&&writes==2);
 manual.categoriesLoaded({{"509660","Art"}});games->setCurrentIndex(0);CHECK(publish->isEnabled());
 tagEdit->setText("con espacios");CHECK(!publish->isEnabled());tagEdit->setText("DJ,dj");CHECK(!publish->isEnabled());tagEdit->setText("");CHECK(publish->isEnabled());
 draftEditor->setPlainText(QString(141,'x'));CHECK(!publish->isEnabled());draftEditor->setPlainText(QString::fromUtf8("🎧").repeated(140));CHECK(publish->isEnabled());
 manual.liveState(false,false,false,"Desconectado");CHECK(!publish->isEnabled());
 CHECK(writes==2&&manual.manualDraft()==QString::fromUtf8("🎧").repeated(140));
 dst::Dock composed;composed.enableLive([]{},[]{});int composedWrites=0;dst::ChannelFields composedSent;
 composed.enableEditing([&](const dst::ChannelFields &fields){++composedWrites;composedSent=fields;return true;},[](QString){return false;});
 composed.liveState(true,true,true,"Conectado");composed.channelLoaded(original);
 auto *composedEditor=composed.findChild<QPlainTextEdit*>("manualDraft");auto *composedApply=composed.findChild<QPushButton*>("apply");
 composedEditor->setPlainText("Borrador pendiente");dst::AutomaticBlocks blocks;blocks.hype=true;blocks.raiders={"Raider","RAIDER"};
 composed.automaticStatePreview(blocks);
 CHECK(composedWrites==0&&composed.automaticTitle().text=="| HYPE TRAIN ON | Canal inicial | Thanks Raiders @Raider");
 CHECK(!composed.automaticTitle().text.contains("Borrador")&&composed.findChild<QLabel*>("composerPreview")->text()==composed.automaticTitle().text);
 composedApply->click();CHECK(composedWrites==1&&composedSent.title=="| HYPE TRAIN ON | Borrador pendiente | Thanks Raiders @Raider");
 composedEditor->setPlainText("Otro borrador");composed.publicationFinished(204);
 CHECK(composed.automaticTitle().text==composedSent.title&&composed.manualDraft()=="Otro borrador");
 composed.automaticStatePreview({});CHECK(composed.automaticTitle().text=="Borrador pendiente"&&composedWrites==1);
 composedApply->click();composed.publicationFinished(500);CHECK(composed.automaticTitle().text=="Borrador pendiente"&&composed.manualDraft()=="Otro borrador");
 dst::Dock raidDock;raidDock.enableLive([]{},[]{});raidDock.enableEditing([](const dst::ChannelFields&){return true;},[](QString){return false;});
 QStringList automaticWrites;raidDock.enableRaiders([&](QString title){automaticWrites.append(title);return true;});
 raidDock.liveState(true,true,true,"Conectado");raidDock.setRaidAccount("123");raidDock.channelLoaded(original);
 auto *raidSwitch=raidDock.findChild<QCheckBox*>("raidersEnabled");CHECK(raidSwitch->isEnabled()&&!raidSwitch->isChecked());raidSwitch->setChecked(true);CHECK(automaticWrites.isEmpty());
 raidDock.findChild<QPlainTextEdit*>("manualDraft")->setPlainText("BORRADOR SECRETO");raidDock.findChild<QLineEdit*>("tags")->setText("Pending");
 raidDock.receiveRaid("123","wave1","789","raider");CHECK(automaticWrites.size()==1&&automaticWrites.last()=="Canal inicial | Thanks Raiders @raider");
 raidDock.receiveRaid("123","wave2","789","RAIDER");raidDock.receiveRaid("123","wave3","789","renamed");CHECK(automaticWrites.size()==1);
 raidDock.receiveRaid("123","second","456","second");CHECK(automaticWrites.size()==1);raidDock.automaticPublicationFinished(204);
 CHECK(automaticWrites.size()==2&&automaticWrites.last()=="Canal inicial | Thanks Raiders @raider @second");raidDock.automaticPublicationFinished(204);
 CHECK(raidDock.manualDraft()=="BORRADOR SECRETO"&&raidDock.findChild<QLineEdit*>("tags")->text()=="Pending");
 raidDock.receiveRaid("999","wrong","555","foreign");CHECK(automaticWrites.size()==2);
 raidSwitch->setChecked(false);CHECK(automaticWrites.size()==3&&automaticWrites.last()=="Canal inicial");raidDock.automaticPublicationFinished(204);
 raidSwitch->setChecked(true);raidDock.receiveRaid("123","error","555","newuser");CHECK(automaticWrites.size()==4);raidDock.automaticPublicationFinished(429);
 raidDock.receiveRaid("123","after-error","556","another");CHECK(automaticWrites.size()==4);
 raidDock.liveState(false,false,false,"Desconectado");CHECK(!raidSwitch->isChecked()&&!raidSwitch->isEnabled());
 dst::Dock automatic;automatic.enableLive([]{},[]{});automatic.enableEditing([](const dst::ChannelFields&){return true;},[](QString){return false;});
 QStringList mixedWrites;automatic.enableRaiders([&](QString title){mixedWrites.append(title);return true;});automatic.enableHype();
 automatic.liveState(true,true,true,"Conectado");automatic.setRaidAccount("123");automatic.channelLoaded(original);
 auto *hypeSwitch=automatic.findChild<QCheckBox*>("hypeEnabled");auto *mixedRaid=automatic.findChild<QCheckBox*>("raidersEnabled");
 CHECK(hypeSwitch->isEnabled()&&mixedRaid->isEnabled());hypeSwitch->setChecked(true);mixedRaid->setChecked(true);
 automatic.findChild<QPlainTextEdit*>("manualDraft")->setPlainText("NO PUBLICAR");
 automatic.receiveHype("999","foreign",100,true);CHECK(mixedWrites.isEmpty());
 automatic.receiveHype("123","train",100,true);CHECK(mixedWrites.size()==1&&mixedWrites.last()=="| HYPE TRAIN ON | Canal inicial");
 automatic.receiveHype("123","train",100,true);automatic.receiveRaid("123","raid","789","raider");CHECK(mixedWrites.size()==1);
 automatic.automaticPublicationFinished(204);CHECK(mixedWrites.size()==2&&mixedWrites.last()=="| HYPE TRAIN ON | Canal inicial | Thanks Raiders @raider");
 automatic.receiveHype("123","train",100,false);automatic.automaticPublicationFinished(204);
 CHECK(mixedWrites.size()==3&&mixedWrites.last()=="Canal inicial | Thanks Raiders @raider");automatic.automaticPublicationFinished(204);
 automatic.receiveHype("123","train",100,true);CHECK(mixedWrites.size()==3);
 automatic.receiveHype("123","next",200,true);automatic.automaticPublicationFinished(204);CHECK(mixedWrites.size()==4);
 hypeSwitch->setChecked(false);CHECK(mixedWrites.size()==5&&mixedWrites.last()=="Canal inicial | Thanks Raiders @raider");automatic.automaticPublicationFinished(204);
 CHECK(automatic.manualDraft()=="NO PUBLICAR");
 automatic.liveState(false,false,false,"Desconectado");CHECK(!hypeSwitch->isChecked()&&!hypeSwitch->isEnabled());
 QTemporaryDir sessionDirectory;const auto statePath=sessionDirectory.filePath("title-state.json");dst::ChannelFields restoredRemote;
 {
  dst::Dock stored;stored.enableLive([]{},[]{});stored.enableEditing([](const dst::ChannelFields&){return true;},[](QString){return false;});
  stored.enableRaiders([](QString){return true;});stored.enableHype();stored.enableProtection(statePath,[]{});
  stored.liveState(true,true,true,"Conectado");stored.setRaidAccount("123");stored.channelLoaded(original);
  stored.findChild<QCheckBox*>("raidersEnabled")->setChecked(true);stored.findChild<QCheckBox*>("hypeEnabled")->setChecked(true);
  stored.receiveRaid("123","first","789","raider");stored.automaticPublicationFinished(204);
  stored.receiveHype("123","persisted-train",100,true);stored.automaticPublicationFinished(204);
  stored.findChild<QPlainTextEdit*>("manualDraft")->setPlainText("Borrador conservado");stored.findChild<QLineEdit*>("tags")->setText("pending tag");restoredRemote=stored.publishedFields();
 }
 {
  dst::Dock restored;int resumedWrites=0;QString resumedTitle;
  restored.enableLive([]{},[]{});restored.enableEditing([](const dst::ChannelFields&){return true;},[](QString){return false;});
  restored.enableRaiders([&](QString title){++resumedWrites;resumedTitle=title;return true;});restored.enableHype();restored.enableProtection(statePath,[]{});
  restored.liveState(true,true,true,"Conectado");restored.setRaidAccount("123");restored.channelLoaded(restoredRemote);
  CHECK(!restored.findChild<QCheckBox*>("publicationLock")&&resumedWrites==0&&restored.manualDraft()=="Borrador conservado");
  CHECK(restored.findChild<QLineEdit*>("tags")->text()=="pending tag"&&restored.automaticTitle().text==restoredRemote.title);
  restored.receiveRaid("123","second-wave","789","raider");CHECK(resumedWrites==0);
  restored.receiveHype("123","persisted-train",100,false);CHECK(resumedWrites==1&&resumedTitle=="Canal inicial | Thanks Raiders @raider");restored.automaticPublicationFinished(204);
  restored.channelLoaded(restored.publishedFields());CHECK(restored.automaticTitle().text==resumedTitle&&resumedWrites==1);
  auto outside=original;outside.title="Cambio externo";restored.channelLoaded(outside);CHECK(restored.manualDraft()=="Borrador conservado"&&restored.findChild<QCheckBox*>("raidersEnabled")->isChecked());
 }
 {
 dst::Dock offline;offline.enableLive([]{},[]{});offline.enableEditing([](const dst::ChannelFields&){return true;},[](QString){return false;});
 QStringList writes;offline.enableRaiders([&](QString title){writes.append(title);return true;});offline.enableHype();offline.liveState(true,true,true,"Connected");offline.setRaidAccount("123");offline.channelLoaded(original);
 offline.findChild<QPlainTextEdit*>("manualDraft")->setPlainText("New applied base");offline.findChild<QPushButton*>("apply")->click();offline.publicationFinished(204);
 offline.findChild<QPlainTextEdit*>("manualDraft")->setPlainText("NEVER SEND THIS DRAFT");
 offline.findChild<QCheckBox*>("raidersEnabled")->setChecked(true);
 offline.receiveRaid("123","off","111","offlineuser");CHECK(writes.size()==1);offline.automaticPublicationFinished(412);CHECK(writes.size()==1);
 offline.receiveRaid("123","on","222","liveuser");CHECK(writes.size()==2&&writes.last().startsWith("New applied base")&&!writes.last().contains("NEVER SEND")&&writes.last().contains("@liveuser")&&!writes.last().contains("@offlineuser"));offline.automaticPublicationFinished(204);
 offline.receiveRaid("123","conflict","333","pending");offline.automaticPublicationFinished(409);auto count=writes.size();offline.receiveRaid("123","paused","444","later");CHECK(writes.size()==count);
 auto external=original;external.title="External base";offline.channelLoaded(external);offline.receiveRaid("123","after-reload","555","resumed");CHECK(writes.size()==count+1&&writes.last()=="External base | Thanks Raiders @resumed");offline.automaticPublicationFinished(204);
 }
 QJsonObject restoredState;CHECK(dst::readState(statePath,restoredState)&&restoredState.value("accounts").toObject().contains("123"));
 QFile corrupted(sessionDirectory.filePath("bad.json"));CHECK(corrupted.open(QIODevice::WriteOnly));corrupted.write("invalid-json");corrupted.close();
 {dst::Dock bad;int manual=0;bad.enableLive([]{},[]{});bad.enableEditing([&](const dst::ChannelFields &){++manual;return true;},[](QString){return false;});bad.enableProtection(corrupted.fileName(),[]{});bad.liveState(true,true,true,"Connected");bad.setRaidAccount("123");bad.channelLoaded(original);bad.findChild<QPlainTextEdit*>("manualDraft")->setPlainText("Publishing remains available");auto *apply=bad.findChild<QPushButton*>("apply");CHECK(!bad.findChild<QCheckBox*>("publicationLock")&&apply->isEnabled());apply->click();CHECK(manual==1);bad.publicationFinished(204);}
 CHECK(corrupted.open(QIODevice::ReadOnly)&&corrupted.readAll()=="invalid-json");
 std::cout<<"PASS: publication, draft isolation, emoji picker, mixed events, persisted state, automatic recovery without lock and external reconciliation\n";
}




