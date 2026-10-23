// SPDX-License-Identifier: GPL-2.0-or-later
#include "localization.hpp"
#include "sources/supporter-language.hpp"
#include "sources/supporter-frame.hpp"
#include <QGuiApplication>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QTemporaryDir>
#include <QPainter>
#include <QFileInfo>
#include <set>
#include <iostream>
#include <stdexcept>
using namespace oto;
namespace {
void require(bool value,const char *why){if(!value)throw std::runtime_error(why);}
std::multiset<QString> placeholders(const QString &value){
    std::multiset<QString> result;auto matches=QRegularExpression("%[1-9][0-9]*").globalMatch(value);
    while(matches.hasNext())result.insert(matches.next().captured());return result;
}
}
int main(int argc,char **argv){QGuiApplication app(argc,argv);try{
    using namespace nexus::localization;
    for(const auto &item:std::initializer_list<std::pair<const char *,const char *>>{
        {"ES_mx","es-ES"},{"it-CH","it-IT"},{"fr-CA","fr-FR"},{"de-AT","de-DE"},
        {"en-GB","en-US"},{"pt-BR","en-US"},{"ja-JP","en-US"},{"C","en-US"},{"","en-US"}})
        require(languageFor(QString::fromLatin1(item.first))==item.second,"Wrong system-language fallback");
    require(selection("es-MX")=="es-ES" && selection("auto")=="auto" && selection("pt-BR")=="auto","Invalid manual preference");
    const auto &catalog=i18n::catalog();const auto &base=catalog.entries("en-US");
    const auto presets=presets::Registry::fromFile(QString::fromUtf8(OTO_PRESET_FILE));
    require(base.size()>=320,"Embedded catalogs missing");QJsonObject exported;
    for(const auto &language:languages){
        const auto code=QString::fromLatin1(language.code);const auto &entries=catalog.entries(code);
        require(entries.size()==base.size(),"Translation keys missing");QJsonObject values;
        for(const auto &[key,value]:base){
            const auto it=entries.find(key);require(it!=entries.end() && !it->second.isEmpty(),"Translation missing or empty");
            require(placeholders(QString::fromUtf8(value))==placeholders(QString::fromUtf8(it->second)),"Translation lost a placeholder");
            require(!QString::fromUtf8(it->second).contains(QChar::ReplacementCharacter),"Translation damaged UTF-8");
            values.insert(QString::fromUtf8(key),QString::fromUtf8(it->second));
        }
        require(i18n::qtext("Setup.Welcome",code).contains('\n'),"INI newline not decoded");exported.insert(code,values);
        sources::BarSettings settings;obs_data_t *data=obs_data_create();obs_data_set_string(data,"language",language.code);
        const std::string serialized=obs_data_get_json(data);obs_data_release(data);data=obs_data_create_from_json(serialized.c_str());
        sources::readSupporterLanguage(settings,data);obs_data_release(data);require(settings.language==code,"Source language not persisted");
        visual::VisualEngine engine;std::array<QString,6> numbers{"Follower","Subscriber","Cheerer","Gifter","500","2"};
        const auto now=visual::MotionClock::time_point(std::chrono::seconds(10));
        sources::BarContext context;context.generation=1;context.preview=true;
        auto initialSettings=settings;initialSettings.language="en-US";initialSettings.motion=visual::Motion::StyleChange;
        auto frame=sources::prepareSupporterFrame(engine,initialSettings,sources::supporterLabels("en-US"),numbers,{},now,{},{},context);
        numbers[0]="New follower";frame=sources::prepareSupporterFrame(engine,initialSettings,sources::supporterLabels("en-US"),numbers,frame,now,{},{},context);
        require(frame->layers[0].started!=visual::MotionClock::time_point{},"Live-change fixture failed");
        settings.motion=visual::Motion::StyleChange;
        if(code!="en-US"){
            frame=sources::prepareSupporterFrame(engine,settings,sources::supporterLabels(code),numbers,frame,now,{},{},context);
            for(const auto &layer:frame->layers)require(layer.started==visual::MotionClock::time_point{} && !layer.previousValue,"Changing language replayed event motion");
        }
        auto unchanged=sources::prepareSupporterFrame(engine,settings,sources::supporterLabels(code),numbers,frame,now+std::chrono::seconds(1),{},{},context);
        require(unchanged==frame,"Idle localized source recreated frames");
        QImage gallery(1080,int(presets.entries().size())*120,QImage::Format_ARGB32_Premultiplied);gallery.fill(QColor(7,8,12));int y=0;
        for(const auto &preset:presets.entries()){
            auto sample=settings;sample.style=preset.style;sample.presetId=preset.id;sample.width=1080;sample.height=120;
            const auto display=sources::prepareSupporterFrame(engine,sample,sources::supporterLabels(code),numbers,{},now);
            require(display->base && display->base->textMetrics.size()==6,"Factory label preparation missing");
            for(const auto &metric:display->base->textMetrics)if(metric.elided || metric.overflow)
                throw std::runtime_error(("Translated title clipped: "+code+"/"+preset.id+" width="+QString::number(metric.width)+" height="+QString::number(metric.height)+" size="+QString::number(metric.pixelSize)).toStdString());
            {QPainter painter(&gallery);painter.drawImage(0,y,display->base->image);for(const auto &layer:display->layers){
                painter.save();painter.setClipRect(QRect(layer.x,y+layer.y,layer.viewportWidth,layer.frame->image.height()));
                painter.drawImage(layer.x,y+layer.y,layer.frame->image);painter.restore();
            }}y+=120;
        }
        if(argc>1)require(gallery.save(QFileInfo(QString::fromLocal8Bit(argv[1])).absolutePath()+"/supporter-"+code+".png"),"Localized source gallery save failed");
    }
    require(sources::supporterLabels("it-IT").titles!=sources::supporterLabels("de-DE").titles,"Source languages interfere");
    obs_data_t *empty=obs_data_create();sources::BarSettings legacy;sources::readSupporterLanguage(legacy,empty);
    require(legacy.language=="auto","Legacy source no longer follows OS");obs_data_set_string(empty,"language","unknown");
    sources::readSupporterLanguage(legacy,empty);require(legacy.language=="auto","Invalid stored locale not normalized");obs_data_release(empty);
    QTemporaryDir temp;require(temp.isValid(),"Temporary fallback fixture unavailable");
    QFile english(temp.filePath("en-US.ini"));require(english.open(QIODevice::WriteOnly),"Fallback fixture unavailable");english.write("OnlyEnglish=\"Fallback\"\n");english.close();
    Catalog fallback(temp.path());require(QString::fromUtf8(fallback.text("OnlyEnglish","it-IT"))=="Fallback","Missing key did not fall back to English");
    require(QString::fromUtf8(fallback.text("Unknown","fr-FR"))=="Unknown" && !fallback.contains("Unknown"),"Unknown key not handled safely");
    if(argc>1){QFile output(QString::fromLocal8Bit(argv[1]));require(output.open(QIODevice::WriteOnly),"Catalog export unavailable");output.write(QJsonDocument(exported).toJson());}
    std::cout<<"Five languages: "<<base.size()<<" keys each; OS fallback, placeholders, persistence, isolation and no replay passed\n";return 0;
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}}
