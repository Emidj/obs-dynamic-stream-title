// SPDX-License-Identifier: GPL-2.0-or-later
#include "emoji-picker.hpp"
#include "ui-language.hpp"
#include <QAbstractListModel>
#include <QComboBox>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QListView>
#include <QVBoxLayout>
static void initEmojiResources(){Q_INIT_RESOURCE(emoji);}
namespace dst {
const QList<EmojiEntry> &emojiCatalog()
{
 static const auto entries=[]{
  initEmojiResources();QFile file(":/dst/emoji/catalog.json");QList<EmojiEntry> result;
  if(!file.open(QIODevice::ReadOnly))return result;
  for(const auto &value:QJsonDocument::fromJson(file.readAll()).array()){
   const auto obj=value.toObject();result.append({obj["emoji"].toString(),obj["group"].toString(),obj["name"].toString(),obj["keywords"].toString(),obj["names"].toObject()});
  }
  return result;
 }();return entries;
}
static QString searchable(QString value)
{
 value=value.normalized(QString::NormalizationForm_D).toCaseFolded();QString result;
 for(auto c:value)if(c.category()!=QChar::Mark_NonSpacing)result+=c;
 return result;
}
class EmojiModel final:public QAbstractListModel {
public:
 explicit EmojiModel(QObject *parent):QAbstractListModel(parent){}
 QList<int> indices;
 int rowCount(const QModelIndex &parent={})const override{return parent.isValid()?0:indices.size();}
 QVariant data(const QModelIndex &index,int role)const override{
  if(!index.isValid()||index.row()<0||index.row()>=indices.size())return {};
  const auto &entry=emojiCatalog()[indices[index.row()]];
  if(role==Qt::DisplayRole||role==Qt::UserRole)return entry.emoji;
  if(role==Qt::FontRole){QFont font("Segoe UI Emoji");font.setPixelSize(30);return font;}
  if(role==Qt::ToolTipRole||role==Qt::AccessibleTextRole)return entry.emoji+" · "+entry.names.value(uiLanguage()).toString(entry.name);
  if(role==Qt::TextAlignmentRole)return int(Qt::AlignCenter);
  return {};
 }
 void replace(QList<int> values){beginResetModel();indices=std::move(values);endResetModel();}
};
EmojiPicker::EmojiPicker(QStringList recent,std::function<void(QString)> selected,QWidget *parent)
 :QDialog(parent),recent_(std::move(recent)),selected_(std::move(selected))
{
 setAttribute(Qt::WA_DeleteOnClose);setWindowTitle(dst::uiText(QString::fromUtf8("Emojis · Unicode 17.0")));setMinimumSize(240,300);resize(370,470);
 auto *layout=new QVBoxLayout(this);
 auto *hint=new QLabel(dst::uiText(QString::fromUtf8("Selecciona un emoji para insertarlo en Base Title.")),this);hint->setWordWrap(true);layout->addWidget(hint);
 search_=new QLineEdit(this);search_->setObjectName("emojiSearch");search_->setPlaceholderText(dst::uiText(QString::fromUtf8("Buscar: corazón, fiesta, music…")));search_->setAccessibleName(dst::uiText(QString::fromUtf8("Buscar emoji")));layout->addWidget(search_);
 category_=new QComboBox(this);category_->setObjectName("emojiCategory");category_->setAccessibleName(dst::uiText(QString::fromUtf8("Categoría de emojis")));
 category_->addItem(dst::uiText(QString::fromUtf8("Todos")),QString());category_->addItem(dst::uiText(QString::fromUtf8("Recientes")),"recent");
 const QList<QPair<QString,QString>> groups={
  {"Smileys & Emotion",dst::uiText(QString::fromUtf8("Caras y emociones"))},{"People & Body",dst::uiText(QString::fromUtf8("Personas y cuerpo"))},
  {"Component",dst::uiText(QString::fromUtf8("Componentes"))},{"Animals & Nature",dst::uiText(QString::fromUtf8("Animales y naturaleza"))},
  {"Food & Drink",dst::uiText(QString::fromUtf8("Comida y bebida"))},{"Travel & Places",dst::uiText(QString::fromUtf8("Viajes y lugares"))},
  {"Activities",dst::uiText(QString::fromUtf8("Actividades"))},{"Objects",dst::uiText(QString::fromUtf8("Objetos"))},{"Symbols",dst::uiText(QString::fromUtf8("Símbolos"))},{"Flags",dst::uiText(QString::fromUtf8("Banderas"))}};
 for(const auto &group:groups)category_->addItem(dst::uiText(group.second),group.first);
 layout->addWidget(category_);
 list_=new QListView(this);list_->setObjectName("emojiList");list_->setAccessibleName(dst::uiText(QString::fromUtf8("Emojis disponibles")));
 list_->setViewMode(QListView::IconMode);list_->setResizeMode(QListView::Adjust);list_->setMovement(QListView::Static);
 list_->setGridSize(QSize(48,48));list_->setSpacing(0);list_->setUniformItemSizes(true);list_->setWordWrap(false);list_->setTextElideMode(Qt::ElideNone);list_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
 QFont font("Segoe UI Emoji");font.setPixelSize(30);list_->setFont(font);
 model_=new EmojiModel(this);list_->setModel(model_);layout->addWidget(list_,1);
 count_=new QLabel(this);count_->setObjectName("emojiCount");layout->addWidget(count_);
 auto *note=new QLabel(dst::uiText(QString::fromUtf8("Los recientes duran esta sesión. La apariencia depende de las fuentes de Windows.")),this);note->setWordWrap(true);layout->addWidget(note);
 connect(search_,&QLineEdit::textChanged,this,[this]{filter();});connect(category_,&QComboBox::currentIndexChanged,this,[this]{filter();});
 auto choose=[this](const QModelIndex &index){if(!index.isValid())return;selected_(model_->data(index,Qt::UserRole).toString());close();};
 connect(list_,&QListView::clicked,this,choose);connect(list_,&QListView::activated,this,choose);
 filter();search_->setFocus();
}
void EmojiPicker::filter()
{
 const auto group=category_->currentData().toString();const auto query=searchable(search_->text().trimmed());
 const auto words=query.split(' ',Qt::SkipEmptyParts);QList<int> result;const auto &catalog=emojiCatalog();
 auto matches=[&](int i){const auto &entry=catalog[i];
  if(!group.isEmpty()&&group!="recent"&&entry.group!=group)return false;
  const auto text=searchable(entry.emoji+" "+entry.name+" "+entry.keywords);
  for(const auto &word:words)if(!text.contains(word))return false;return true;
 };
 if(group=="recent"){
  for(const auto &emoji:recent_)for(int i=0;i<catalog.size();++i)if(catalog[i].emoji==emoji&&matches(i)){result.append(i);break;}
 }else for(int i=0;i<catalog.size();++i)if(matches(i))result.append(i);
 model_->replace(result);count_->setText(QString::number(result.size())+QString::fromUtf8(" emojis · Unicode 17.0"));
}
}
