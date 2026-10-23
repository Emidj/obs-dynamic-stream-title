// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <QString>
#include <QStringList>
#include <QRegularExpression>
#include <QSet>
#include <utility>
namespace dst {
struct AutomaticBlocks {bool hype=false;QStringList raiders;};
struct ComposedTitle {
 QString text;bool valid=true,hypeOmitted=false;
 int raidersOmitted=0,invalidRaiders=0;
 int characters()const{return text.toUcs4().size();}
};
class TitleComposer final {
public:
 static constexpr int limit=140;
 void setAppliedBase(QString base){appliedBase_=std::move(base);}
 const QString &appliedBase()const{return appliedBase_;}
 ComposedTitle automatic(const AutomaticBlocks &blocks)const{return compose(appliedBase_,blocks);}
 static ComposedTitle compose(const QString &base,const AutomaticBlocks &blocks){
  ComposedTitle result;result.text=base;
  if(base.trimmed().isEmpty()||result.characters()>limit){result.valid=false;return result;}
  const QString hype="| HYPE TRAIN ON | ";
  if(blocks.hype){if((hype+result.text).toUcs4().size()<=limit)result.text=hype+result.text;else result.hypeOmitted=true;}
  static const QRegularExpression login("^[A-Za-z0-9_]{1,25}$");
  QSet<QString> seen;QStringList unique;
  for(auto name:blocks.raiders){name=name.trimmed();if(name.startsWith('@'))name.remove(0,1);
   if(!login.match(name).hasMatch()){++result.invalidRaiders;continue;}
   const auto key=name.toCaseFolded();if(seen.contains(key))continue;seen.insert(key);unique.append(name);
  }
  QString suffix;
  for(int index=0;index<unique.size();++index){
   const auto candidate=(suffix.isEmpty()?QString(" | Thanks Raiders"):suffix)+" @"+unique[index];
   if((result.text+candidate).toUcs4().size()>limit){result.raidersOmitted=unique.size()-index;break;}
   suffix=candidate;
  }
  result.text+=suffix;return result;
 }
private:
 QString appliedBase_;
};
}
