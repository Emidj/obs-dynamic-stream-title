// SPDX-License-Identifier: GPL-2.0-or-later
#include "title-composer.hpp"
#include "raid-session.hpp"
#include "hype-session.hpp"
#include <iostream>
#define CHECK(x) do{if(!(x)){std::cerr<<"FAIL "<<__LINE__<<"\n";return 1;}}while(false)
int main(){
 dst::HypeSession hype;
 CHECK(hype.update("one",100,true)&&hype.active());
 CHECK(!hype.update("one",100,true));
 CHECK(hype.update("one",100,false)&&!hype.active());
 CHECK(!hype.update("one",100,true)&&!hype.active());
 CHECK(!hype.update("two",200,false)&&!hype.update("two",200,true));
 CHECK(hype.update("three",300,true));
 CHECK(!hype.update("one",100,false)&&hype.active());
 CHECK(!hype.update("",400,true)&&!hype.update("four",0,true));
 hype.reset();CHECK(!hype.active());
 dst::RaidSession raids;raids.reset("123");
 CHECK(raids.add("123","wave-1","789","Raider"));
 CHECK(!raids.add("123","wave-2","789","raider")); // A second raid to bring remaining viewers.
 CHECK(!raids.add("123","wave-3","789","renamed_raider"));
 CHECK(!raids.add("123","wave-1","456","other")&&!raids.add("999","foreign","456","other"));
 CHECK(!raids.add("123","bad","456","bad login")&&raids.names()==QStringList{"raider"});
 CHECK(raids.add("123","unique","456","Other")&&raids.names().size()==2);
 raids.reset("321");CHECK(raids.names().isEmpty()&&!raids.add("123","old","789","raider"));
 dst::TitleComposer composer;composer.setAppliedBase("Base aplicada");
 dst::AutomaticBlocks events;events.hype=true;events.raiders={"Alice","@ALICE","Bob","bad user","https://evil.example"};
 const auto first=composer.automatic(events);
 CHECK(first.valid&&first.text=="| HYPE TRAIN ON | Base aplicada | Thanks Raiders @Alice @Bob"&&first.invalidRaiders==2);
 CHECK(composer.automatic(events).text==first.text&&composer.appliedBase()=="Base aplicada");
 CHECK(composer.automatic({}).text=="Base aplicada");
 CHECK(dst::TitleComposer::compose("Borrador",events).text.contains("Borrador")&&composer.automatic(events).text==first.text);
 events.raiders={"A","B"};events.hype=false;
 const auto exactSuffix=QString(" | Thanks Raiders @A");
 composer.setAppliedBase(QString(140-exactSuffix.size(),'x'));
 auto edge=composer.automatic(events);CHECK(edge.characters()==140&&edge.text.endsWith("@A")&&edge.raidersOmitted==1);
 events.hype=true;events.raiders.clear();const QString prefix="| HYPE TRAIN ON | ";
 composer.setAppliedBase(QString(140-prefix.size(),'x'));CHECK(composer.automatic(events).characters()==140&&!composer.automatic(events).hypeOmitted);
 composer.setAppliedBase(QString(141-prefix.size(),'x'));CHECK(composer.automatic(events).hypeOmitted&&composer.automatic(events).text==composer.appliedBase());
 events.raiders={"A","B"};composer.setAppliedBase(QString(140-prefix.size()-exactSuffix.size(),'x'));
 auto combined=composer.automatic(events);CHECK(combined.characters()==140&&!combined.hypeOmitted&&combined.text.endsWith("@A")&&combined.raidersOmitted==1);
 composer.setAppliedBase(QString(140-prefix.size(),'x'));combined=composer.automatic(events);CHECK(!combined.hypeOmitted&&combined.raidersOmitted==2&&combined.text.startsWith(prefix));
 composer.setAppliedBase(QString::fromUtf8("🎧").repeated(140));events.raiders={"A"};
 auto unicode=composer.automatic(events);CHECK(unicode.characters()==140&&unicode.hypeOmitted&&unicode.raidersOmitted==1&&unicode.text==composer.appliedBase());
 composer.setAppliedBase(QString::fromUtf8("👩🏽‍💻 DJ"));events.hype=false;CHECK(composer.automatic(events).text.startsWith(composer.appliedBase()));
 CHECK(!dst::TitleComposer::compose(QString(141,'x'),{}).valid&&!dst::TitleComposer::compose(" ",{}).valid);
 // Stable rebuilding, changing Hype state and repeated batches never append twice.
 composer.setAppliedBase("Live");events.hype=true;events.raiders={"Raider","RAIDER"};
 for(int i=0;i<1000;++i){auto title=composer.automatic(events);CHECK(title.text.count("HYPE TRAIN ON")==1&&title.text.count("Thanks Raiders")==1&&title.text.count("@Raider")==1);}
 events.hype=false;CHECK(!composer.automatic(events).text.contains("HYPE TRAIN ON")&&composer.automatic(events).text.contains("Thanks Raiders"));
 std::cout<<"PASS: applied-base isolation, deterministic blocks, priority, exact limits, Unicode integrity and unique valid handles\n";
}
