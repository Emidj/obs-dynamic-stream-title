// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <QString>
#include <QtGlobal>
#include <QJsonObject>
namespace dst {
class HypeSession final {
public:
 void reset(){id_.clear();started_=0;active_=false;ended_=false;}
 bool active()const{return active_;}
 QJsonObject saved()const{return {{"id",id_},{"started",QString::number(started_)},{"active",active_},{"ended",ended_}};}
 bool restore(const QJsonObject &row){reset();const auto id=row.value("id").toString();const auto started=row.value("started").toString().toLongLong();if(id.isEmpty())return started==0;if(id.size()>128||started<=0)return false;id_=id;started_=started;ended_=row.value("ended").toBool();active_=!ended_&&row.value("active").toBool();return true;}
 bool update(const QString &id,qint64 started,bool begin){
  if(id.isEmpty()||id.size()>128||started<=0||started<started_)return false;
  if(started==started_&&id!=id_)return false;
  if(started>started_){id_=id;started_=started;ended_=false;active_=false;}
  const bool before=active_;
  if(!begin){ended_=true;active_=false;}else if(!ended_)active_=true;
  return before!=active_;
 }
private:
 QString id_;qint64 started_=0;bool active_=false,ended_=false;
};
}
