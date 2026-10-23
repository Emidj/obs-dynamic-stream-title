// SPDX-License-Identifier: GPL-2.0-or-later
#include "dock.hpp"
#include "emoji-picker.hpp"
#include "ui-language.hpp"
#include "state-store.hpp"
#include <QTimer>
#include <QJsonArray>
#include <QCheckBox>
#include <QComboBox>
#include <QFrame>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QScrollArea>
#include <QTextCursor>
#include <QVBoxLayout>
#include <QSignalBlocker>
#include <QHBoxLayout>
#include <QImage>
#include <QPixmap>
namespace dst {
static QLabel *label(const QString &text,QWidget *parent)
{
 auto *widget=new QLabel(text,parent);widget->setTextFormat(Qt::PlainText);widget->setWordWrap(true);return widget;
}
Dock::Dock(QWidget *parent):QWidget(parent)
{
 setObjectName("dstDock");setMinimumWidth(240);setFont(QFont("Segoe UI",9));
 auto *outer=new QVBoxLayout(this);outer->setContentsMargins(0,0,0,0);outer->setSpacing(0);
 auto *scroll=new QScrollArea(this);scroll->setObjectName("dockScroll");scroll->setWidgetResizable(true);
 scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);scroll->setFrameShape(QFrame::NoFrame);
 auto *body=new QWidget(scroll);body->setObjectName("dstBody");auto *layout=new QVBoxLayout(body);layout->setContentsMargins(12,12,12,12);layout->setSpacing(10);
 auto *header=new QWidget(body);header->setObjectName("brandHeader");
 auto *headerLayout=new QHBoxLayout(header);headerLayout->setContentsMargins(0,0,0,0);headerLayout->setSpacing(10);
 auto *brand=new QLabel(header);brand->setObjectName("brandLogo");brand->setAccessibleName("NEXUS");brand->setFixedSize(56,64);brand->setAlignment(Qt::AlignCenter);
 // Trim transparent margins for display; keep the supplied PNG unmodified.
 const QImage logo(":/dst/branding/nexus-logo.png");
 if(!logo.isNull()){
  int left=logo.width(),top=logo.height(),right=-1,bottom=-1;
  const auto rgba=logo.convertToFormat(QImage::Format_ARGB32);
  for(int y=0;y<rgba.height();++y){const auto *row=reinterpret_cast<const QRgb*>(rgba.constScanLine(y));for(int x=0;x<rgba.width();++x)if(qAlpha(row[x])>0){left=qMin(left,x);top=qMin(top,y);right=qMax(right,x);bottom=qMax(bottom,y);}}
  if(right>=left){auto pixmap=QPixmap::fromImage(logo.copy(left,top,right-left+1,bottom-top+1)).scaled(112,128,Qt::KeepAspectRatio,Qt::SmoothTransformation);pixmap.setDevicePixelRatio(2);brand->setPixmap(pixmap);}
 }
 headerLayout->addWidget(brand);
 auto *wordmark=new QWidget(header);auto *wordmarkLayout=new QVBoxLayout(wordmark);wordmarkLayout->setContentsMargins(0,0,0,0);wordmarkLayout->setSpacing(0);wordmarkLayout->setAlignment(Qt::AlignVCenter);
 auto *name=label("Dynamic Stream Title",wordmark);name->setObjectName("productName");name->setMinimumWidth(0);name->setSizePolicy(QSizePolicy::Ignored,QSizePolicy::Preferred);wordmarkLayout->addWidget(name);
 auto *subtitle=label("for Twitch",wordmark);subtitle->setObjectName("productSubtitle");wordmarkLayout->addWidget(subtitle);headerLayout->addWidget(wordmark,1);
 layout->addWidget(header);
 auto *notice=label(dst::uiText(QString::fromUtf8("PROTOTIPO · SIMULACIÓN LOCAL · "))+QStringLiteral(DST_VERSION),body);notice->setObjectName("demoNotice");layout->addWidget(notice);
 auto section=[&](const QString &title){auto *heading=label(title,body);heading->setObjectName("section");layout->addWidget(heading);};
 section(dst::uiText(QString::fromUtf8("Base Title")));
 editor_=new QPlainTextEdit(body);editor_->setObjectName("manualDraft");editor_->setAccessibleName(dst::uiText(QString::fromUtf8("Base Title")));
 editor_->setPlaceholderText(dst::uiText(QString::fromUtf8("Escribe el título base…")));editor_->setFixedHeight(84);editor_->setPlainText(appliedBase_);layout->addWidget(editor_);
 auto *emoji=new QPushButton(dst::uiText(QString::fromUtf8("☺  Añadir emoji")),body);emoji->setObjectName("emojiButton");layout->addWidget(emoji);
 section(dst::uiText(QString::fromUtf8("Category")));category_=new QComboBox(body);category_->setObjectName("category");category_->setAccessibleName(dst::uiText(QString::fromUtf8("Category")));
 category_->addItems({"Music","Just Chatting","Art","Games + Demos"});category_->setEditable(true);category_->setMinimumWidth(0);category_->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);layout->addWidget(category_);
 auto *categoryHelp=label(dst::uiText(QString::fromUtf8("Categorías de muestra; sin búsqueda en Twitch.")),body);categoryHelp->setObjectName("categoryHelp");layout->addWidget(categoryHelp);
 section(dst::uiText(QString::fromUtf8("Tags")));tags_=new QLineEdit(appliedTags_,body);tags_->setObjectName("tags");tags_->setAccessibleName(dst::uiText(QString::fromUtf8("Tags")));tags_->setPlaceholderText("DJ, Live, Spanish");layout->addWidget(tags_);
 section(dst::uiText(QString::fromUtf8("Automatic Events")));
 raiders_=new QCheckBox("Thanks Raiders",body);raiders_->setObjectName("raidersEnabled");layout->addWidget(raiders_);
 hype_=new QCheckBox("Hype Train ON",body);hype_->setObjectName("hypeEnabled");layout->addWidget(hype_);
 auto *demo=new QFrame(body);demo->setObjectName("simulation");auto *events=new QVBoxLayout(demo);events->setContentsMargins(8,8,8,8);
 events->addWidget(label(dst::uiText(QString::fromUtf8("Probar eventos simulados")),demo));
 raidSample_=new QCheckBox("Raid: @Demo",demo);raidSample_->setObjectName("raidSample");events->addWidget(raidSample_);
 hypeSample_=new QCheckBox(dst::uiText(QString::fromUtf8("Hype activo")),demo);hypeSample_->setObjectName("hypeSample");events->addWidget(hypeSample_);layout->addWidget(demo);
 section(dst::uiText(QString::fromUtf8("Title Preview")));
 layout->addWidget(label(dst::uiText(QString::fromUtf8("Borrador manual")),body));preview_=label({},body);preview_->setObjectName("preview");preview_->setTextInteractionFlags(Qt::TextSelectableByMouse);preview_->setSizePolicy(QSizePolicy::Ignored,QSizePolicy::Preferred);layout->addWidget(preview_);
 draftCount_=label({},body);draftCount_->setObjectName("draftCharacterCount");layout->addWidget(draftCount_);
 auto *composerHeading=label(dst::uiText(QString::fromUtf8("Composición automática · base aplicada")),body);composerHeading->setObjectName("composerHeading");layout->addWidget(composerHeading);
 composerPreview_=label({},body);composerPreview_->setObjectName("composerPreview");composerPreview_->setSizePolicy(QSizePolicy::Ignored,QSizePolicy::Preferred);layout->addWidget(composerPreview_);
 composerCount_=label({},body);composerCount_->setObjectName("composerCharacterCount");layout->addWidget(composerCount_);
 auto *appliedHeading=label(dst::uiText(QString::fromUtf8("Resultado aplicado · simulación")),body);appliedHeading->setObjectName("appliedHeading");layout->addWidget(appliedHeading);applied_=label({},body);applied_->setObjectName("appliedPreview");applied_->setSizePolicy(QSizePolicy::Ignored,QSizePolicy::Preferred);layout->addWidget(applied_);
 apply_=new QPushButton(dst::uiText(QString::fromUtf8("Apply to Twitch")),body);apply_->setObjectName("apply");apply_->setSizePolicy(QSizePolicy::Ignored,QSizePolicy::Fixed);apply_->setToolTip(dst::uiText(QString::fromUtf8("Simulación local: no envía datos a Twitch.")));layout->addWidget(apply_);
 status_=label({},body);status_->setObjectName("status");layout->addWidget(status_);
 auto *manualHelp=label(dst::uiText(QString::fromUtf8("Editar o insertar emojis no aplica cambios. Los eventos usan la base aplicada de la demo.")),body);manualHelp->setObjectName("manualHelp");layout->addWidget(manualHelp);layout->addStretch();
 scroll->setWidget(body);outer->addWidget(scroll,1);
 auto *footer=new QWidget(this);footer->setObjectName("connectionBar");auto *bottom=new QVBoxLayout(footer);bottom->setContentsMargins(10,8,10,8);bottom->setSpacing(5);
 coreIdentity_=label({},footer);coreIdentity_->setObjectName("twitchCoreIdentity");coreIdentity_->setVisible(false);bottom->addWidget(coreIdentity_);
 connection_=label({},footer);connection_->setObjectName("connectionStatus");bottom->addWidget(connection_);
 connect_=new QPushButton(dst::uiText(QString::fromUtf8("Conectar demo")),footer);connect_->setObjectName("connectDemo");connect_->setSizePolicy(QSizePolicy::Ignored,QSizePolicy::Fixed);bottom->addWidget(connect_);outer->addWidget(footer);
 connect(editor_,&QPlainTextEdit::textChanged,this,[this]{refresh();});
 connect(category_,&QComboBox::currentTextChanged,this,[this]{refresh();});connect(tags_,&QLineEdit::textChanged,this,[this]{refresh();});
 for(auto *check:{raiders_,hype_,raidSample_,hypeSample_})connect(check,&QCheckBox::toggled,this,[this]{refresh();});
 connect(emoji,&QPushButton::clicked,this,[this]{openEmojis();});
 connect(connect_,&QPushButton::clicked,this,[this]{if(live_){if(liveActive_){if(liveDisconnect_)liveDisconnect_();}else if(liveConnect_)liveConnect_();return;}connected_=!connected_;refresh();});
 connect(apply_,&QPushButton::clicked,this,[this]{
  if(live_){
   if(!editing_||!apply_->isEnabled())return;
   sentManual_=draftFields();sent_=sentManual_;sent_.title=TitleComposer::compose(sentManual_.title,blocks_).text;publishing_=true;feedback_.clear();refresh();
   if(!publish_||!publish_(sent_)){publishing_=false;feedback_=dst::uiText(QString::fromUtf8("No se pudo iniciar el envío. El borrador se conserva."));refresh();}return;
  }
  if(live_||!connected_||manualDraft().trimmed().isEmpty())return;
  appliedBase_=manualDraft();appliedCategory_=category_->currentText();appliedTags_=tags_->text();refresh();
 });
 setStyleSheet(QString::fromUtf8(R"(
 QWidget#dstDock,QWidget#dstBody{background:#141622;color:#e9e9f4;}
 QLabel{color:#dddfee;} QLabel#productName{color:#eae4ff;font-size:20px;font-weight:600;}
 QLabel#productSubtitle{color:#ad91ed;font-size:14px;font-weight:400;}
 QLabel#demoNotice{color:#65d8f3;font-size:10px;} QLabel#section{color:#bea7ff;font-weight:600;font-size:14px;}
 QLabel#composerHeading,QLabel#appliedHeading{font-size:14px;font-weight:600;}
 QPlainTextEdit,QLineEdit,QComboBox{background:#202335;color:#f2f0ff;border:1px solid #45435f;border-radius:6px;padding:7px;selection-background-color:#7353b7;}
 QPlainTextEdit:focus,QLineEdit:focus,QComboBox:focus{border:1px solid #ac83ff;}
 QPushButton{background:#292d43;color:#eae4ff;border:1px solid #514775;border-radius:6px;padding:7px;}
 QPushButton:hover{background:#393453;border-color:#a78af0;} QPushButton#apply{background:#7150c5;color:white;font-weight:600;}
 QPushButton:disabled,QPushButton#apply:disabled{background:#242633;color:#818394;border-color:#363849;}
 QFrame#simulation{background:#1c2031;border:1px solid #343851;border-radius:6px;}
 QLabel#preview,QLabel#appliedPreview,QLabel#composerPreview{background:#1c2031;border-left:2px solid #9d79e6;padding:8px;}
 QWidget#connectionBar{background:#1c2031;border-top:1px solid #3a3c55;} QLabel#connectionStatus{color:#71d4e4;}
 QCheckBox{color:#deddf0;spacing:7px;} QCheckBox::indicator{width:16px;height:16px;}
 )"));
 refresh();
}
QString Dock::manualDraft() const{return editor_->toPlainText();}
void Dock::refresh()
{
 if(saveTimer_&&loaded_&&!loading_)saveTimer_->start();
 preview_->setText(manualDraft().isEmpty()?dst::uiText(QString::fromUtf8("El borrador está vacío.")):manualDraft());
 const auto draftComposition=TitleComposer::compose(manualDraft(),blocks_);
 if(!manualDraft().isEmpty())preview_->setText(draftComposition.text);
 draftCount_->setText(dst::uiText(QString::fromUtf8("Base: %1/140 · título final: %2/140")).arg(manualDraft().toUcs4().size()).arg(draftComposition.characters()));
 if(draftComposition.hypeOmitted||draftComposition.raidersOmitted)draftCount_->setText(draftCount_->text()+dst::uiText(QString::fromUtf8(" · bloques omitidos por espacio")));
 const auto automatic=composer_.automatic(blocks_);
 composerPreview_->setText(loaded_?automatic.text:dst::uiText(QString::fromUtf8("Esperando base aplicada…")));
 QString omissions;
 if(automatic.hypeOmitted)omissions+=dst::uiText(QString::fromUtf8(" · Hype omitido por espacio"));
 if(automatic.raidersOmitted)omissions+=dst::uiText(QString::fromUtf8(" · %1 raiders sin espacio")).arg(automatic.raidersOmitted);
 if(automatic.invalidRaiders)omissions+=dst::uiText(QString::fromUtf8(" · %1 usuarios no válidos")).arg(automatic.invalidRaiders);
 composerCount_->setText(dst::uiText(QString::fromUtf8("%1/140 caracteres")).arg(automatic.characters())+omissions);
 // Fixed sample blocks only: production composition and Twitch events are later phases.
 QString sample=appliedBase_;
 if(raiders_->isChecked()&&raidSample_->isChecked())sample+=" | Thanks Raiders @DemoRaider";
 if(hype_->isChecked()&&hypeSample_->isChecked())sample="| HYPE TRAIN ON | "+sample;
 if(!live_)applied_->setText(sample+"\n"+appliedCategory_+" · "+appliedTags_);
 const bool dirty=manualDraft()!=appliedBase_||category_->currentText()!=appliedCategory_||tags_->text()!=appliedTags_;
 status_->setText(dirty?dst::uiText(QString::fromUtf8("Cambios pendientes · solo Apply los aplica en la demo.")):dst::uiText(QString::fromUtf8("Sin cambios manuales pendientes · demo local.")));
 apply_->setEnabled(!live_&&connected_&&!manualDraft().trimmed().isEmpty()&&dirty);
 if(live_){
  if(!editing_){status_->setText(dst::uiText(QString::fromUtf8("Lectura de Twitch · publicación disponible en la fase 5.")));return;}
  const auto draft=draftFields();const auto error=dst::uiText(draft.error());const bool changed=!(draft==appliedManual_);
  apply_->setEnabled(authorized_&&manage_&&loaded_&&!manualConflict_&&!publishing_&&!automaticPublishing_&&!searching_&&changed&&error.isEmpty());
  if(reload_)reload_->setEnabled(authorized_&&!publishing_&&!automaticPublishing_&&!searching_);
  if(searchButton_)searchButton_->setEnabled(authorized_&&!publishing_&&!automaticPublishing_&&!searching_);
  if(raidFeature_)raiders_->setEnabled(authorized_&&manage_&&loaded_);
  if(hypeFeature_)hype_->setEnabled(authorized_&&manage_&&loaded_);
  status_->setText(automaticPublishing_?dst::uiText(QString::fromUtf8("Actualizando los bloques automáticos del título aplicado…")):
   publishing_?dst::uiText(QString::fromUtf8("Enviando a Twitch… Los cambios nuevos quedarán pendientes.")):
   !feedback_.isEmpty()?feedback_:!loaded_?dst::uiText(QString::fromUtf8("Esperando lectura del canal…")):
   !error.isEmpty()?error:changed?dst::uiText(QString::fromUtf8("Cambios pendientes · solo Apply publica.")):dst::uiText(QString::fromUtf8("Sin cambios pendientes.")));return;
 }
 connection_->setText(connected_?dst::uiText(QString::fromUtf8("● Cuenta demo conectada · sin red")):dst::uiText(QString::fromUtf8("○ Sin conectar · simulación local")));
 connect_->setText(connected_?dst::uiText(QString::fromUtf8("Desconectar demo")):dst::uiText(QString::fromUtf8("Conectar demo")));
}
void Dock::enableLive(std::function<void()> connect,std::function<void()> disconnect){
 live_=true;liveConnect_=std::move(connect);liveDisconnect_=std::move(disconnect);
 for(auto *check:{raiders_,hype_,raidSample_,hypeSample_}){check->setChecked(false);check->setEnabled(false);}
 if(auto *notice=findChild<QLabel*>("demoNotice"))notice->setText(dst::uiText(QString::fromUtf8("TWITCH CORE · CONEXIÓN Y LECTURA · "))+QStringLiteral(DST_VERSION));
 for(auto *text:findChildren<QLabel*>()){
  if(text->objectName()=="appliedHeading")text->setText(dst::uiText(QString::fromUtf8("Información actual del canal")));
  if(text->objectName()=="manualHelp")text->setText(dst::uiText(QString::fromUtf8("La lectura no modifica tu borrador. Apply y los eventos se habilitarán en sus fases correspondientes.")));
 }
 applied_->setText(dst::uiText(QString::fromUtf8("Conecta para leer el canal. No se publican cambios.")));
 apply_->setToolTip(dst::uiText(QString::fromUtf8("Publicación pendiente de la fase 5.")));refresh();
 liveState(false,false,false,dst::uiText(QString::fromUtf8("Twitch Core disponible · sin conectar")));
}
void Dock::liveState(bool active,bool authorized,bool canManage,const QString &status){
 liveActive_=active;
 authorized_=authorized;manage_=canManage;
 if(!authorized){saveState();loaded_=false;publishing_=false;searching_=false;automaticPublishing_=false;automaticFault_=false;raids_.reset();hypeSession_.reset();blocks_.hype=false;{QSignalBlocker blocker(hype_);hype_->setChecked(false);}raidAccount_.clear();blocks_.raiders.clear();if(raidFeature_){QSignalBlocker blocker(raiders_);raiders_->setChecked(false);}}
 connection_->setText((authorized?QString::fromUtf8("● "):QString::fromUtf8("○ "))+status);
 // An authorized shared account can grant the additional broadcast permission.
 if(authorized&&!canManage){liveActive_=false;connect_->setText(dst::uiText(QString::fromUtf8("Autorizar edición del canal")));}
 else connect_->setText(active?dst::uiText(QString::fromUtf8("Desconectar Twitch (cuenta compartida)")):dst::uiText(QString::fromUtf8("Conectar a Twitch")));
 apply_->setEnabled(false);
 if(editing_)refresh();
}
void Dock::channelInformation(const QString &text){applied_->setText(text);}
ChannelFields Dock::draftFields()const{
 ChannelFields fields;fields.title=manualDraft();fields.category=category_->currentText();
 const auto index=category_->currentIndex();
 if(index>=0&&category_->itemText(index)==fields.category)fields.gameId=category_->itemData(index).toString();
 if(tags_->text().trimmed().isEmpty())return fields;
 for(auto tag:tags_->text().split(','))fields.tags.append(tag.trimmed());return fields;
}
void Dock::enableEditing(std::function<bool(const ChannelFields &)> publish,std::function<bool(QString)> search){
 editing_=true;publish_=std::move(publish);search_=std::move(search);
 category_->clear();category_->addItem(dst::uiText(QString::fromUtf8("Sin categoría")),"0");category_->setInsertPolicy(QComboBox::NoInsert);
 editor_->clear();tags_->clear();editor_->setEnabled(false);category_->setEnabled(false);tags_->setEnabled(false);
 searchButton_=new QPushButton(dst::uiText(QString::fromUtf8("Buscar categoría en Twitch")),this);searchButton_->setObjectName("searchCategory");searchButton_->setSizePolicy(QSizePolicy::Ignored,QSizePolicy::Fixed);
 auto *body=findChild<QWidget*>("dstBody");auto *layout=qobject_cast<QVBoxLayout*>(body->layout());
 layout->insertWidget(layout->indexOf(category_)+1,searchButton_);
 connect(searchButton_,&QPushButton::clicked,this,[this]{
  const auto query=category_->currentText().trimmed();if(query.isEmpty()){requestFailed(dst::uiText(QString::fromUtf8("Escribe el nombre de una categoría para buscar.")));return;}
  searching_=true;feedback_=dst::uiText(QString::fromUtf8("Buscando categorías…"));refresh();
  if(!search_||!search_(query))requestFailed(dst::uiText(QString::fromUtf8("No se pudo iniciar la búsqueda. Inténtalo de nuevo.")));
 });
 auto edit=[this]{if(!loading_){edited_=true;feedback_.clear();}refresh();};
 connect(editor_,&QPlainTextEdit::textChanged,this,edit);connect(tags_,&QLineEdit::textChanged,this,edit);connect(category_,&QComboBox::currentTextChanged,this,edit);
 connect(category_,qOverload<int>(&QComboBox::currentIndexChanged),this,edit);
 for(auto *text:findChildren<QLabel*>()){
  if(text->objectName()=="categoryHelp")text->setText(dst::uiText(QString::fromUtf8("Escribe un nombre, busca y selecciona un resultado.")));
  if(text->objectName()=="manualHelp")text->setText(dst::uiText(QString::fromUtf8("Editar o insertar emojis nunca publica. Solo Apply envía el borrador.")));
 }
 if(auto *notice=findChild<QLabel*>("demoNotice"))notice->setText(dst::uiText(QString::fromUtf8("TWITCH · EDICIÓN MANUAL · "))+QStringLiteral(DST_VERSION));
 if(auto *demo=findChild<QFrame*>("simulation"))demo->hide();
 apply_->setToolTip(dst::uiText(QString::fromUtf8("Publica título, categoría y tags en la cuenta conectada.")));refresh();
}
void Dock::channelLoaded(const ChannelFields &fields){
 const bool retainBase=loaded_&&fields.title==published_.title;
 if(loaded_&&!retainBase){raids_.reset(raidAccount_);hypeSession_.reset();blocks_={};trace("External title reconciled; previous automatic blocks reset");feedback_=dst::uiText(QString::fromUtf8("Cambio externo leído. Base actualizada y borrador conservado."));}
 const auto base=retainBase?composer_.appliedBase():fields.title;
 loading_=true;published_=fields;appliedManual_=fields;appliedManual_.title=base;composer_.setAppliedBase(base);loaded_=true;manualConflict_=false;
 if(category_->findData(fields.gameId)<0){const auto text=category_->currentText();QSignalBlocker blocker(category_);category_->addItem(fields.category,fields.gameId);if(edited_)category_->setEditText(text);}
 if(!edited_){editor_->setPlainText(fields.title);category_->setCurrentIndex(category_->findData(fields.gameId));tags_->setText(fields.tags.join(", "));}
 if(restorePending_){restoreState(fields);restorePending_=false;}
 loading_=false;applied_->setText(fields.title+"\n"+fields.category+" · "+fields.tags.join(", "));refresh();
 editor_->setEnabled(true);category_->setEnabled(true);tags_->setEnabled(true);
 if(raidFeature_){automaticFault_=false;pumpAutomatic();}
}
void Dock::categoriesLoaded(const QVector<QPair<QString,QString>> &categories){
 const auto text=category_->currentText();loading_=true;
 {QSignalBlocker blocker(category_);category_->clear();for(const auto &entry:categories)category_->addItem(entry.second,entry.first);category_->setCurrentIndex(-1);category_->setEditText(text);}
 loading_=false;searching_=false;feedback_=categories.isEmpty()?dst::uiText(QString::fromUtf8("No se encontraron categorías.")):dst::uiText(QString::fromUtf8("Selecciona un resultado de la lista."));refresh();pumpAutomatic();
}
void Dock::publicationFinished(int http){
 if(!publishing_)return;publishing_=false;
 if(http==204){published_=sent_;appliedManual_=sentManual_;composer_.setAppliedBase(sentManual_.title);automaticFault_=false;manualConflict_=false;trace("Manual publication confirmed HTTP 204");applied_->setText(published_.title+"\n"+published_.category+" · "+published_.tags.join(", "));feedback_=draftFields()==appliedManual_?dst::uiText(QString::fromUtf8("Cambios publicados en Twitch.")):dst::uiText(QString::fromUtf8("Cambios publicados. Hay cambios nuevos pendientes."));}
 else {manualConflict_=http==409;trace(QString("Manual publication paused HTTP %1").arg(http));feedback_=http==409?dst::uiText(QString::fromUtf8("Cambio externo detectado. Borrador conservado; pulsa Recargar canal y vuelve a aplicar.")):dst::uiText(QString::fromUtf8("Twitch no confirmó la publicación (HTTP %1). Borrador conservado; comprueba el canal antes de reintentar.")).arg(http);}
 refresh();pumpAutomatic();
}
void Dock::requestFailed(const QString &message){searching_=false;feedback_=message;refresh();pumpAutomatic();}
void Dock::automaticStatePreview(const AutomaticBlocks &blocks){blocks_=blocks;refresh();}
void Dock::enableRaiders(std::function<bool(QString)> publishTitle){
 raidFeature_=true;publishTitle_=std::move(publishTitle);
 raiders_->setToolTip(dst::uiText(QString::fromUtf8("Al recibir un raid, actualiza solo el título aplicado. No publica tu borrador. Desactivar retira el bloque de esta sesión.")));
 connect(raiders_,&QCheckBox::toggled,this,[this](bool enabled){
  if(enabled&&composer_.appliedBase().contains(" | Thanks Raiders @")){
   QSignalBlocker blocker(raiders_);raiders_->setChecked(false);feedback_=dst::uiText(QString::fromUtf8("Hay un bloque Raiders anterior sin identificar. Retíralo de la base y pulsa Apply antes de activar."));refresh();return;
  }
  if(!enabled)raids_.reset(raidAccount_);
  blocks_.raiders=raiders_->isChecked()?raids_.names():QStringList{};feedback_.clear();refresh();pumpAutomatic();
 });refresh();
}
void Dock::enableHype(){
 hypeFeature_=true;
 hype_->setToolTip(dst::uiText(QString::fromUtf8("Añade o retira el prefijo al comenzar o terminar un Hype Train observado. No publica el borrador.")));
 connect(hype_,&QCheckBox::toggled,this,[this](bool enabled){
  if(enabled&&composer_.appliedBase().startsWith("| HYPE TRAIN ON | ")){
   QSignalBlocker blocker(hype_);hype_->setChecked(false);feedback_=dst::uiText(QString::fromUtf8("Hay un prefijo Hype anterior sin identificar. Retíralo de la base y pulsa Apply antes de activar."));refresh();return;
  }
  blocks_.hype=hype_->isChecked()&&hypeSession_.active();feedback_.clear();refresh();pumpAutomatic();
 });refresh();
}
void Dock::receiveHype(const QString &channel,const QString &train,qint64 started,bool begin){
 if(!hypeFeature_||!authorized_||!loaded_||channel!=raidAccount_||channel.isEmpty())return;
 if(!hypeSession_.update(train,started,begin))return;
 blocks_.hype=hype_->isChecked()&&hypeSession_.active();feedback_.clear();refresh();pumpAutomatic();
}
void Dock::setRaidAccount(const QString &account){
 if(account==raidAccount_)return;
 saveState();restorePending_=!statePath_.isEmpty();edited_=false;loaded_=false;publishing_=false;automaticPublishing_=false;searching_=false;automaticFault_=false;
 raidAccount_=account;raids_.reset(account);hypeSession_.reset();blocks_.hype=false;{QSignalBlocker blocker(hype_);hype_->setChecked(false);}blocks_.raiders.clear();
 QSignalBlocker blocker(raiders_);raiders_->setChecked(false);refresh();
}
void Dock::receiveRaid(const QString &channel,const QString &event,const QString &user,const QString &login){
 if(!raidFeature_||!raiders_->isChecked()||!authorized_||!loaded_)return;
 if(!raids_.add(channel,event,user,login))return;
 blocks_.raiders=raids_.names();feedback_.clear();refresh();pumpAutomatic();
}
void Dock::pumpAutomatic(){
 if(!raidFeature_||automaticFault_||!authorized_||!manage_||!loaded_||publishing_||automaticPublishing_||searching_)return;
 if((raiders_->isChecked()&&composer_.appliedBase().contains(" | Thanks Raiders "))||(hype_->isChecked()&&composer_.appliedBase().startsWith("| HYPE TRAIN ON | "))){automaticFault_=true;trace("Automatic operation paused: unrecognized existing block");feedback_=dst::uiText(QString::fromUtf8("La base contiene un bloque automático anterior. Revisa la base y pulsa Apply para evitar duplicados."));refresh();return;}
 const auto title=composer_.automatic(blocks_);if(!title.valid||title.text==published_.title)return;
 automaticSent_=title.text;automaticPublishing_=true;refresh();
 if(!publishTitle_||!publishTitle_(automaticSent_)){
  automaticPublishing_=false;automaticFault_=true;feedback_=dst::uiText(QString::fromUtf8("Eventos automáticos pausados: no se pudo iniciar el envío. Reconecta o confirma la base con Apply."));refresh();
 }
}
void Dock::automaticPublicationFinished(int http){
 if(!automaticPublishing_)return;automaticPublishing_=false;
 if(http==204){trace("Automatic publication confirmed HTTP 204");published_.title=automaticSent_;applied_->setText(published_.title+"\n"+published_.category+" · "+published_.tags.join(", "));feedback_=dst::uiText(QString::fromUtf8("Bloques automáticos actualizados. El borrador manual no se publicó."));}
 else if(http==412){raids_.reset(raidAccount_);hypeSession_.reset();blocks_={};trace("Automatic event ignored: channel offline");feedback_=dst::uiText(QString::fromUtf8("Canal fuera de directo. Evento automático ignorado; no se publicó el título."));refresh();return;}
 else {automaticFault_=true;trace(QString("Automatic publication paused HTTP %1").arg(http));feedback_=http==409?dst::uiText(QString::fromUtf8("Cambio externo detectado. Evento pendiente; pulsa Recargar canal para reconciliar.")):dst::uiText(QString::fromUtf8("Eventos automáticos pausados (HTTP %1). Recarga el canal o aplica la base para reintentar.")).arg(http);}

 refresh();pumpAutomatic();
}
Dock::~Dock(){saveState();}
void Dock::enableProtection(const QString &path,std::function<void()> reload){
 statePath_=path;reloadChannel_=std::move(reload);stateHealthy_=readState(path,savedState_);restorePending_=true;
 saveTimer_=new QTimer(this);saveTimer_->setSingleShot(true);saveTimer_->setInterval(350);
 connect(saveTimer_,&QTimer::timeout,this,[this]{saveState();});
 auto *layout=qobject_cast<QVBoxLayout*>(findChild<QWidget*>("dstBody")->layout());
 reload_=new QPushButton(dst::uiText(QString::fromUtf8("Recargar canal")),this);reload_->setObjectName("reloadChannel");reload_->setSizePolicy(QSizePolicy::Ignored,QSizePolicy::Fixed);
 layout->insertWidget(layout->indexOf(apply_),reload_);
 connect(reload_,&QPushButton::clicked,this,[this]{if(reloadChannel_)reloadChannel_();});
 if(!stateHealthy_){trace("Persistence unavailable: original file preserved; publishing remains available");feedback_=dst::uiText(QString::fromUtf8("Configuración dañada o ilegible. No se sobrescribirá; los cambios se mantienen en memoria."));}
 refresh();
}
void Dock::saveState(){
 if(statePath_.isEmpty()||!stateHealthy_||!loaded_||raidAccount_.isEmpty())return;
 if(saveTimer_)saveTimer_->stop();
 auto records=savedState_.value("accounts").toObject();
 auto field=[](const ChannelFields &f){auto row=f.payload();row.insert("category",f.category);return row;};
 records.insert(raidAccount_,QJsonObject{{"published",field(published_)},{"applied",field(appliedManual_)},
  {"draft_title",manualDraft()},{"draft_category",category_->currentText()},{"draft_game",category_->currentData().toString()},{"draft_tags",tags_->text()},
  {"raiders",raids_.saved()},{"raid_enabled",raiders_->isChecked()},{"hype_enabled",hype_->isChecked()},{"hype",hypeSession_.saved()}});
 savedState_.insert("accounts",records);
 if(!writeState(statePath_,savedState_)){stateHealthy_=false;trace("Persistence write failed; publishing remains available");feedback_=dst::uiText(QString::fromUtf8("No se pudo guardar el estado. Puedes publicar; los cambios solo se conservan durante esta sesión."));status_->setText(feedback_);}

}
void Dock::restoreState(const ChannelFields &remote){
 const auto row=savedState_.value("accounts").toObject().value(raidAccount_).toObject();if(row.isEmpty())return;
 auto field=[](const QJsonObject &r){ChannelFields f;f.title=r.value("title").toString();f.gameId=r.value("game_id").toString();f.category=r.value("category").toString();for(auto t:r.value("tags").toArray())f.tags.append(t.toString());return f;};
 const auto last=field(row.value("published").toObject()),base=field(row.value("applied").toObject());
 QSignalBlocker raid(raiders_),hype(hype_);
 raiders_->setChecked(row.value("raid_enabled").toBool());hype_->setChecked(row.value("hype_enabled").toBool());
 RaidSession restoredRaids;HypeSession restoredHype;
 if(last.title==remote.title&&base.error().isEmpty()&&restoredRaids.restore(raidAccount_,row.value("raiders").toArray())&&restoredHype.restore(row.value("hype").toObject())){
  AutomaticBlocks restored;restored.raiders=row.value("raid_enabled").toBool()?restoredRaids.names():QStringList{};restored.hype=row.value("hype_enabled").toBool()&&restoredHype.active();
  if(TitleComposer::compose(base.title,restored).text==remote.title){appliedManual_=base;composer_.setAppliedBase(base.title);raids_=std::move(restoredRaids);hypeSession_=std::move(restoredHype);blocks_=restored;raiders_->setChecked(row.value("raid_enabled").toBool());hype_->setChecked(row.value("hype_enabled").toBool());}
 }
 editor_->setPlainText(row.value("draft_title").toString());tags_->setText(row.value("draft_tags").toString());
 const auto game=row.value("draft_game").toString(),category=row.value("draft_category").toString();
 if(!game.isEmpty()&&category_->findData(game)<0)category_->addItem(category,game);
 category_->setCurrentIndex(category_->findData(game));category_->setEditText(category);edited_=true;
 trace("Session restored without publication lock");
 feedback_=dst::uiText(QString::fromUtf8("Sesión recuperada. Las automatizaciones continúan con la base aplicada; el borrador no se publica."));
}
void Dock::automaticPreview(const QString &title)
{
 status_->setText(dst::uiText(QString::fromUtf8("Evento de prueba: "))+title);
}
void Dock::openEmojis()
{
 if(picker_){picker_->raise();picker_->activateWindow();return;}
 picker_=new EmojiPicker(recent_,[this](QString emoji){
  auto cursor=editor_->textCursor();cursor.insertText(emoji);editor_->setTextCursor(cursor);
  recent_.removeAll(emoji);recent_.prepend(emoji);while(recent_.size()>32)recent_.removeLast();editor_->setFocus();
 },this);picker_->show();
}
}

void dst::Dock::setCoreIdentity(const QString &text,const QString &path){coreIdentity_->setText(text);coreIdentity_->setToolTip(path);coreIdentity_->setVisible(true);}
