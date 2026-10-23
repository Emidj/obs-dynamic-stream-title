// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <QWidget>
#include <QPointer>
#include <QStringList>
#include <functional>
#include "channel-fields.hpp"
#include <QVector>
#include "title-composer.hpp"
#include "raid-session.hpp"
#include "hype-session.hpp"
class QPlainTextEdit;
class QLabel;
class QPushButton;
class QLineEdit;
class QComboBox;
class QCheckBox;
class QTimer;
namespace dst {
class EmojiPicker;
class Dock final : public QWidget {
public:
 explicit Dock(QWidget *parent=nullptr);
 ~Dock() override;
 void enableProtection(const QString &path,std::function<void()> reload);
 ChannelFields publishedFields()const{return published_;}
 void automaticPreview(const QString &title);
 QString manualDraft() const;
 void enableLive(std::function<void()> connect,std::function<void()> disconnect);
 void liveState(bool active,bool authorized,bool canManage,const QString &status);
 void channelInformation(const QString &text);
 void enableEditing(std::function<bool(const ChannelFields &)> publish,std::function<bool(QString)> search);
 void channelLoaded(const ChannelFields &fields);
 void categoriesLoaded(const QVector<QPair<QString,QString>> &categories);
 void publicationFinished(int http);
 void requestFailed(const QString &message);
 // Preview only in phase 6. EventSub subscriptions cannot call publication here.
 void automaticStatePreview(const AutomaticBlocks &blocks);
 ComposedTitle automaticTitle()const{return composer_.automatic(blocks_);}
 void enableRaiders(std::function<bool(QString)> publishTitle);
 void enableHype();
 void receiveHype(const QString &channel,const QString &train,qint64 started,bool begin);
 void setRaidAccount(const QString &account);
 void receiveRaid(const QString &channel,const QString &event,const QString &user,const QString &login);
 void automaticPublicationFinished(int http);
 void setDiagnostics(std::function<void(QString)> log){diagnostic_=std::move(log);}
 void setCoreIdentity(const QString &text,const QString &path);
private:
 QLabel *coreIdentity_=nullptr;
 ChannelFields draftFields()const;
 void refresh();
 void openEmojis();
 QPlainTextEdit *editor_;
 QLabel *preview_;
 QLabel *status_;
 QLabel *connection_;
 QLabel *applied_;
 QLineEdit *tags_;
 QComboBox *category_;
 QCheckBox *raiders_;
 QCheckBox *hype_;
 QCheckBox *raidSample_;
 QCheckBox *hypeSample_;
 QPushButton *apply_;
 QPushButton *connect_;
 QPointer<EmojiPicker> picker_;
 QStringList recent_;
 QString appliedBase_="NEXUS Live Session";
 QString appliedCategory_="Music";
 QString appliedTags_="DJ, Live";
 bool connected_=false;
 bool live_=false,liveActive_=false;
 std::function<void()> liveConnect_,liveDisconnect_;
 bool editing_=false,authorized_=false,manage_=false,loaded_=false,edited_=false,loading_=false,publishing_=false,searching_=false;
 ChannelFields published_,sent_;
 ChannelFields appliedManual_,sentManual_;
 TitleComposer composer_;
 AutomaticBlocks blocks_;
 QLabel *draftCount_=nullptr,*composerPreview_=nullptr,*composerCount_=nullptr;
 QString feedback_;
 std::function<bool(const ChannelFields &)> publish_;
 std::function<bool(QString)> search_;
 QPushButton *searchButton_=nullptr;
 void pumpAutomatic();
 RaidSession raids_;
 HypeSession hypeSession_;
 bool hypeFeature_=false;
 QString raidAccount_,automaticSent_;
 bool raidFeature_=false,automaticPublishing_=false,automaticFault_=false;
 std::function<bool(QString)> publishTitle_;
 void saveState();
 void restoreState(const ChannelFields &remote);
 QString statePath_;
 QJsonObject savedState_;
 bool stateHealthy_=true,restorePending_=false;
 QTimer *saveTimer_=nullptr;
 bool manualConflict_=false;
 std::function<void(QString)> diagnostic_;
 void trace(const QString &text){if(diagnostic_)diagnostic_(text);}
 QPushButton *reload_=nullptr;
 std::function<void()> reloadChannel_;
};
}
