// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>
namespace dst {
inline bool readState(const QString &path,QJsonObject &out){
 QFile file(path);if(!file.exists()){out={{"version",1},{"accounts",QJsonObject{}}};return true;}
 if(file.size()>1024*1024||!file.open(QIODevice::ReadOnly))return false;
 QJsonParseError error;const auto doc=QJsonDocument::fromJson(file.readAll(),&error);
 if(error.error!=QJsonParseError::NoError||!doc.isObject()||doc.object().value("version").toInt()!=1||!doc.object().value("accounts").isObject())return false;
 out=doc.object();return true;
}
inline bool writeState(const QString &path,const QJsonObject &state){
 const auto bytes=QJsonDocument(state).toJson(QJsonDocument::Compact);if(bytes.size()>1024*1024)return false;
 if(!QDir().mkpath(QFileInfo(path).absolutePath()))return false;
 QSaveFile file(path);return file.open(QIODevice::WriteOnly)&&file.write(bytes)==bytes.size()&&file.commit();
}
}
