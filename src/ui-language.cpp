// SPDX-License-Identifier: GPL-2.0-or-later
#include "ui-language.hpp"
#include <QFile>
#include <QJsonDocument>
#include <QLocale>
#include <QStringList>
static void initUiResources(){Q_INIT_RESOURCE(emoji);}
namespace dst {
namespace {QString selected;}
void setUiLanguage(QString language){language=language.toLower().replace('_','-').section('-',0,0);selected=QStringList{"es","en","it","fr","de"}.contains(language)?language:QString("en");}
QString uiLanguage(){if(selected.isEmpty())setUiLanguage(QLocale::system().name());return selected;}
const QJsonObject &uiCatalog(){static const auto catalog=[]{initUiResources();QFile file(":/dst/locale/strings.json");if(!file.open(QIODevice::ReadOnly))return QJsonObject{};return QJsonDocument::fromJson(file.readAll()).object();}();return catalog;}
QString uiText(const QString &source){if(source.isEmpty())return source;const auto value=uiCatalog().value(source).toObject().value(uiLanguage());return value.isString()?value.toString():source;}
}
