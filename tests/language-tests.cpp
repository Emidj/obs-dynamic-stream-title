// SPDX-License-Identifier: GPL-2.0-or-later
#include "dock.hpp"
#include "ui-language.hpp"
#include "emoji-picker.hpp"
#include <QApplication>
#include <QCheckBox>
#include <QPushButton>
#include <QPlainTextEdit>
#include <QRegularExpression>
#include <QListView>
#include <QLineEdit>
#include <iostream>
#define CHECK(x) do{if(!(x)){std::cerr<<"FAIL "<<__LINE__<<"\n";return 1;}}while(false)
int main(int argc,char **argv){
 QApplication app(argc,argv);CHECK(dst::uiCatalog().size()>=137);
 const QStringList languages={"es","en","it","fr","de"};
 for(auto entry=dst::uiCatalog().begin();entry!=dst::uiCatalog().end();++entry)for(const auto &language:languages)CHECK(!entry.value().toObject().value(language).toString().isEmpty());
 for(const auto &language:languages){
  dst::setUiLanguage(language);dst::Dock dock;dock.enableLive([]{},[]{});dock.enableEditing([](const dst::ChannelFields&){return false;},[](QString){return false;});dock.enableRaiders([](QString){return false;});dock.enableHype();
  dock.liveState(true,true,true,"LOCAL");dock.setRaidAccount("123");dst::ChannelFields base;base.title="User data unchanged";base.gameId="1";base.category="User category";base.tags={"UserTag"};dock.channelLoaded(base);
  CHECK(dock.manualDraft()==base.title&&dock.findChild<QPushButton*>("apply")->text()==dst::uiText("Apply to Twitch"));
  CHECK(dock.findChild<QPushButton*>("searchCategory")->text()==dst::uiText(QString::fromUtf8("Buscar categoría en Twitch")));
  dock.resize(270,700);dock.show();app.processEvents();
  dst::EmojiPicker picker({},[](QString){},&dock);auto *search=picker.findChild<QLineEdit*>("emojiSearch");auto *list=picker.findChild<QListView*>("emojiList");
  const QString query=language=="es"?"corazon":language=="it"?"cuore":language=="fr"?QString::fromUtf8("cœur"):language=="de"?"Herz":"heart";
  search->setText(query);CHECK(list->model()->rowCount()>0&&list->model()->rowCount()<3953);
  const auto heart=std::find_if(dst::emojiCatalog().begin(),dst::emojiCatalog().end(),[](const auto &e){return e.emoji==QString::fromUtf8("❤️");});
  CHECK(heart!=dst::emojiCatalog().end()&&!heart->names.value(language).toString().isEmpty());
 }
 dst::setUiLanguage("pt-BR");CHECK(dst::uiLanguage()=="en");dst::setUiLanguage("fr-CA");CHECK(dst::uiLanguage()=="fr");
 std::cout<<"PASS: 5 complete UI catalogs, locale variants, user data preserved, multilingual emoji search and names\n";
}
