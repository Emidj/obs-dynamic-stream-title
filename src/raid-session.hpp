// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <QStringList>
#include <QSet>
#include <QRegularExpression>
#include <utility>
#include <QJsonArray>
#include <QJsonObject>
namespace dst {
class RaidSession final {
public:
 void reset(QString account={}){account_=std::move(account);events_.clear();users_.clear();logins_.clear();names_.clear();saved_=QJsonArray{};}
 QJsonArray saved()const{return saved_;}
 bool restore(const QString &account,const QJsonArray &rows){reset(account);if(rows.size()>128)return false;for(auto value:rows){const auto row=value.toObject();if(!add(account,"restored-"+row.value("id").toString(),row.value("id").toString(),row.value("login").toString())){reset(account);return false;}}return true;}
 const QStringList &names()const{return names_;}
 bool add(const QString &channel,const QString &event,const QString &user,QString login){
  static const QRegularExpression id("^[0-9]{1,128}$"),name("^[a-z0-9_]{1,25}$");
  login=login.toLower();
  if(channel!=account_||!id.match(channel).hasMatch()||!id.match(user).hasMatch()||event.isEmpty()||event.size()>256||!name.match(login).hasMatch())return false;
  if(events_.contains(event)||users_.contains(user)||logins_.contains(login)||names_.size()>=128)return false;
  // Unique users cap event storage as well: at most 128 accepted events/session.
  events_.insert(event);users_.insert(user);logins_.insert(login);names_.append(login);saved_.append(QJsonObject{{"id",user},{"login",login}});return true;
 }
private:
 QString account_;QStringList names_;QSet<QString> events_,users_,logins_;
 QJsonArray saved_;
};
}
