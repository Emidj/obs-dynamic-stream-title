// SPDX-License-Identifier: GPL-2.0-or-later
#include "presets/preset-registry.hpp"
#include "visual/asset-manager.hpp"
#include "sources/supporter-frame.hpp"
#include <QGuiApplication>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QTemporaryDir>
#include <QFile>
#include <QDir>
#include <QPainter>
#include <QFileInfo>
#include <QCryptographicHash>
#include <set>
#include <algorithm>
#include <thread>
#include <iostream>
#include <stdexcept>
using namespace oto;
namespace {
void require(bool ok,const char *why){if(!ok)throw std::runtime_error(why);}
template<class F> void rejects(F operation,const char *why){bool rejected=false;try{operation();}catch(...){rejected=true;}require(rejected,why);}
QByteArray document(QJsonArray definitions,int version=1){return QJsonDocument(QJsonObject{{"schemaVersion",version},{"presets",definitions}}).toJson(QJsonDocument::Compact);}
QJsonObject preset(QString id,QJsonObject style={}){return {{"id",id},{"name",id},{"style",style}};}
}
int main(int argc,char **argv){QGuiApplication app(argc,argv);try{
	QTemporaryDir dir;require(dir.isValid(),"Temporary assets");
	visual::AssetManager assets(dir.path(),1,1024);
	const auto factory=presets::Registry::fromFile(QString::fromUtf8(OTO_PRESET_FILE));
	require(factory.entries().size()==10 && factory.find("minimal") && factory.resolve("removed").id=="minimal","Factory/fallback");
	require(factory.resolve("minimal").style.cards,"Reference Minimal lacks individual cards");
	std::set<int> shoutoutPatterns;for(const auto &item:factory.entries()){require(item.shoutoutStyle && item.shoutoutStyle->motionPattern>=0 && item.shoutoutStyle->motionPattern<=9,"Native Shoutout pattern missing");shoutoutPatterns.insert(item.shoutoutStyle->motionPattern);}require(shoutoutPatterns.size()==10,"Factory Shoutouts share the wrong motion pattern");
	QJsonObject motionStyle{{"atlas","test.png"},{"family","Segoe UI"}};
	for(const auto bad:{QJsonValue(10),QJsonValue(-2),QJsonValue(.5),QJsonValue("1")}){auto value=motionStyle;value["motionPattern"]=bad;auto item=preset("minimal");item["shoutoutStyle"]=value;rejects([&]{presets::Registry::fromJson(document({item}));},"Invalid native Shoutout pattern accepted");}
	{auto item=preset("minimal");item["shoutoutStyle"]=motionStyle;require(presets::Registry::fromJson(document({item})).resolve("minimal").shoutoutStyle->motionPattern==-1,"Old custom artwork enabled native motion implicitly");}
	require(factory.resolve("urban-city").chatStyle->riveted && !factory.resolve("minimal").chatStyle->riveted,"Industrial chat defaults leak to other presets");
	auto invalidRails=preset("minimal");invalidRails.insert("chatStyle",QJsonObject{{"riveted","true"}});
	rejects([&]{presets::Registry::fromJson(document({invalidRails}));},"String industrial chat flag accepted");
	require(factory.resolve("futuristic-hologram").chatStyle->holographic && !factory.resolve("urban-city").chatStyle->holographic,"Holographic chat defaults leak to other presets");
	auto invalidGlass=preset("minimal");invalidGlass.insert("chatStyle",QJsonObject{{"holographic",1}});
	rejects([&]{presets::Registry::fromJson(document({invalidGlass}));},"Non-boolean holographic chat flag accepted");
	const std::array<QString,10> expected{"minimal","neon-dj","cyberpunk","retro-crt","space","horror","love","graffiti","urban-city","futuristic-hologram"};
	for(const auto &id:expected)require(factory.find(id),"Agreed factory preset missing");
	visual::AssetManager factoryAssets(QFileInfo(QString::fromUtf8(OTO_PRESET_FILE)).dir().filePath("assets"));
	require(factoryAssets.stats().loads==0,"Factory registry eagerly loaded images");
	visual::VisualEngine factoryEngine;
	sources::BarLabels factoryLabels{{"LAST FOLLOW","LAST SUB","LAST CHEER","LAST GIFTED","DAILY BITS","DAILY SUBS"},"Empty","Anonymous"};
	std::array<QString,6> factoryValues{QString::fromUtf8(u8"José_日本語_مرحبا_👩‍💻_long_name"),"Subscriber","Cheerer","Gifter","100","6"};
	const std::array<QString,10> galleryOrder{"neon-dj","cyberpunk","retro-crt","space","horror","love","graffiti","minimal","urban-city","futuristic-hologram"};
	QImage gallery(2440,800,QImage::Format_RGBA8888_Premultiplied);gallery.fill(QColor(8,10,16));QPainter galleryPainter(&gallery);
	std::set<QByteArray> backgrounds;
	const auto factoryNow=visual::MotionClock::time_point(std::chrono::seconds(1));
	for(const auto &definition:factory.entries()) {
		visual::ImagePtr image,icons;
		if(!definition.style.backgroundImage.isEmpty()) {
			image=factoryAssets.load(definition.style.backgroundImage);
			require(image->image.width()<=1024 && image->image.height()<=1024 && factoryAssets.load(definition.style.backgroundImage)==image,"Factory asset dimensions or sharing");
		}
		if(!definition.style.iconImage.isEmpty())icons=factoryAssets.load(definition.style.iconImage);
		sources::BarSettings sample;sample.presetId=definition.id;sample.style=definition.style;sample.width=1200;sample.height=120;sample.motion=visual::Motion::None;
		auto displayValues=factoryValues;
		if(definition.style.cards)displayValues={"MariaDJ","TechnoAlex","Pepito 500","Carlos 5","4850","27"};
		const auto frame=sources::prepareSupporterFrame(factoryEngine,sample,factoryLabels,displayValues,{},factoryNow,image,icons);
		for(const auto &layer:frame->layers)require(!layer.frame->textMetrics.front().elided,"Reference sample name unnecessarily elided");
		const auto digest=QCryptographicHash::hash(QByteArray(reinterpret_cast<const char *>(frame->base->image.constBits()),int(frame->base->image.sizeInBytes())),QCryptographicHash::Sha256);
		require(backgrounds.insert(digest).second,"Factory backgrounds are indistinguishable");
		const auto stats=factoryEngine.stats();
		require(sources::prepareSupporterFrame(factoryEngine,sample,factoryLabels,displayValues,frame,factoryNow,image,icons)==frame && factoryEngine.stats().renders==stats.renders && factoryEngine.stats().hits==stats.hits,"Idle factory style rerendered");
		const auto twin=sources::prepareSupporterFrame(factoryEngine,sample,factoryLabels,displayValues,{},factoryNow,image,icons);
		require(twin->base==frame->base && twin->layers[0].frame==frame->layers[0].frame,"Factory style instances failed to share frames");
		auto updatedValues=displayValues;updatedValues[4]="101";
		const auto selective=sources::prepareSupporterFrame(factoryEngine,sample,factoryLabels,updatedValues,frame,factoryNow,image,icons);
		require(selective->base==frame->base && selective->layers[0].frame==frame->layers[0].frame && selective->layers[4].frame!=frame->layers[4].frame && factoryEngine.stats().renders<=stats.renders+1,"Factory style rebuilt unrelated layers");
		sample.motion=visual::Motion::StyleChange;sample.durationMs=2000;
		const auto animatedBase=sources::prepareSupporterFrame(factoryEngine,sample,factoryLabels,displayValues,{},factoryNow,image,icons);
		for(size_t field=0;field<6;++field){
			auto changedValues=displayValues;changedValues[field]+=" changed";
			const auto changed=sources::prepareSupporterFrame(factoryEngine,sample,factoryLabels,changedValues,animatedBase,factoryNow,image,icons);
			require(changed->base==animatedBase->base && changed->layers[field].previousValue==animatedBase->layers[field].frame,"Factory change did not retain outgoing value");
			for(size_t other=0;other<6;++other)if(other!=field)require(!changed->layers[other].previousValue && changed->layers[other].started==visual::MotionClock::time_point{},"Factory animation affected another card");
		}
		sample.motion=visual::Motion::None;
		galleryPainter.setPen(Qt::white);QFont caption("Segoe UI");caption.setPixelSize(16);galleryPainter.setFont(caption);
		const auto position=int(std::find(galleryOrder.begin(),galleryOrder.end(),definition.id)-galleryOrder.begin());
		const int gx=20+(position%2)*1220,gy=(position/2)*160;
		galleryPainter.drawText(QRect(gx,gy+4,1200,24),definition.name);
		galleryPainter.drawImage(gx,gy+32,frame->base->image);
		for(const auto &layer:frame->layers)if(layer.frame){galleryPainter.save();galleryPainter.setClipRect(QRect(gx+layer.x,gy+32+layer.y,layer.viewportWidth,layer.frame->image.height()));galleryPainter.drawImage(gx+layer.x,gy+32+layer.y,layer.frame->image);galleryPainter.restore();}
		for(const auto dimensions:{QSize(600,80),QSize(1920,240)}) {
			sample.width=dimensions.width();sample.height=dimensions.height();
			const auto bounded=sources::prepareSupporterFrame(factoryEngine,sample,factoryLabels,factoryValues,{},factoryNow,image,icons);
			require(bounded->base->image.size()==dimensions && bounded->layers[5].frame,"Factory style failed at supported size bounds");
			for(const int pixels:{8,24,128}){
				sample.labelPixelSize=pixels;
				const auto sized=sources::prepareSupporterFrame(factoryEngine,sample,factoryLabels,factoryValues,bounded,factoryNow,image,icons);
				for(size_t i=0;i<6;++i){
					require(sized->base->textMetrics[i].height<=sources::supporterLabelLayout(sample).height,"Custom label size escaped its band");
					require(sized->layers[i].y+sized->layers[i].frame->image.height()<=sized->layers[i].card.bottom()+1 && sized->layers[i].frame->image.height()>0,"Custom label size clipped values outside card");
				}
			}
			sample.labelPixelSize=0;
			sample.fields.fill(false);sample.fields[0]=true;
			const auto single=sources::prepareSupporterFrame(factoryEngine,sample,factoryLabels,factoryValues,bounded,factoryNow,image,icons);
			require(single->layers[0].frame && !single->layers[1].frame,"Factory style ignored independent field selection");
			sample.fields.fill(true);
		}
	}
	galleryPainter.end();
	require(factoryAssets.stats().loads==13 && factoryAssets.stats().entries<=8 && factoryAssets.stats().bytes<=8*1024*1024 && factoryAssets.stats().evictions==factoryAssets.stats().loads-factoryAssets.stats().entries,"Factory resources not loaded lazily within budget");
	if(argc==3 && QString::fromLocal8Bit(argv[1])=="--gallery")require(gallery.save(QString::fromLocal8Bit(argv[2])),"Factory gallery save");
	QJsonArray many{preset("minimal")};
	for(int n=0;n<29;++n)many.append(preset(QStringLiteral("style-%1").arg(n),{{"backgroundImage",QStringLiteral("unused-%1.png").arg(n)}}));
	const auto registry=presets::Registry::fromJson(document(many));
	require(registry.entries().size()==30 && assets.stats().loads==0 && assets.stats().entries==0,"Installed presets loaded inactive resources");
	rejects([&]{presets::Registry::fromJson(document({preset("minimal"),preset("minimal")}));},"Duplicate preset accepted");
	rejects([&]{presets::Registry::fromJson(document({preset("other")}));},"Missing fallback accepted");
	rejects([&]{presets::Registry::fromJson(document({preset("minimal")},2));},"Future schema accepted");
	rejects([&]{presets::Registry::fromJson(document({preset("minimal",{{"padding",24.5}})}));},"Fractional layout accepted");
	rejects([&]{presets::Registry::fromJson(document({preset("minimal",{{"valueSize",400}})}));},"Oversized font accepted");
	rejects([&]{presets::Registry::fromJson(document({preset("minimal",{{"value","red"}})}));},"Invalid color accepted");
	rejects([&]{presets::Registry::fromJson(document({preset("minimal",{{"backgroundImage","../outside.png"}})}));},"Escaping asset accepted");
	rejects([&]{presets::Registry::fromJson(document({preset("minimal",{{"backgroundCrop",QJsonArray{900,0,200,1000}}})}));},"Out of bounds crop accepted");
	rejects([&]{presets::Registry::fromJson(document({preset("minimal",{{"iconCrop",QJsonArray{0,0,0,1000}}})}));},"Empty atlas crop accepted");
	rejects([&]{presets::Registry::fromJson(document({preset("minimal",{{"cards","true"}})}));},"String card flag accepted");
	rejects([&]{presets::Registry::fromJson(document({preset("minimal",{{"titleColors",QJsonArray{"#ffffffff"}}})}));},"Incomplete title palette accepted");
	rejects([&]{presets::Registry::fromJson(document({preset("minimal",{{"cardTop",40},{"cardHeight",80}})}));},"Cards outside bar accepted");
	rejects([&]{presets::Registry::fromJson(document({preset("minimal",{{"changeParticles",13}})}));},"Unbounded particles accepted");
	rejects([&]{presets::Registry::fromJson(document({preset("minimal",{{"changeGlow",101}})}));},"Unbounded change intensity accepted");
	rejects([&]{presets::Registry::fromJson(document({preset("minimal",{{"changePattern",8}})}));},"Unknown effect pattern accepted");
	rejects([&]{presets::Registry::fromJson(document({preset("minimal",{{"changeParticleShape",6}})}));},"Unknown particle shape accepted");
	rejects([&]{presets::Registry::fromJson(document({preset("minimal",{{"backgroundBands",6}})}));},"Unbounded atlas rows accepted");
	rejects([&]{presets::Registry::fromJson(document({preset("minimal",{{"badgeBox",QJsonArray{950,0,100,100}}})}));},"Badge outside row accepted");
	rejects([&]{presets::Registry::fromJson(document({preset("minimal",{{"cardGlow",9}})}));},"Unbounded card glow accepted");
	rejects([&]{presets::Registry::fromJson(document({preset("minimal",{{"changeSweep","true"}})}));},"String sweep accepted");
	rejects([&]{presets::Registry::fromJson(document({preset("minimal",{{"changeColors",QJsonArray{"#ffffffff"}}})}));},"Incomplete change palette accepted");
	rejects([&]{presets::Registry::fromJson(QByteArray(65537,' '));},"Oversized registry accepted");
	QJsonArray tooMany=many;for(int n=30;n<66;++n)tooMany.append(preset(QStringLiteral("extra-%1").arg(n)));
	rejects([&]{presets::Registry::fromJson(document(tooMany));},"Unbounded preset count accepted");
	QImage red(8,8,QImage::Format_RGBA8888_Premultiplied);red.fill(Qt::red);
	QImage blue(8,8,QImage::Format_RGBA8888_Premultiplied);blue.fill(Qt::blue);
	require(red.save(dir.filePath("red.png")) && blue.save(dir.filePath("blue.png")),"PNG fixture save");
	const auto first=assets.load("red.png");require(assets.stats().loads==1 && assets.load("red.png")==first && assets.stats().hits==1,"Asset not loaded once/shared");
	const auto second=assets.load("blue.png");require(assets.stats().loads==2 && assets.stats().entries==1 && assets.stats().bytes==256 && assets.stats().evictions==1,"Asset budget or LRU");
	require(first->image.pixelColor(0,0)==Qt::red,"Eviction invalidated held asset");
	visual::AssetManager byteLimited(dir.path(),8,256);byteLimited.load("red.png");byteLimited.load("blue.png");
	require(byteLimited.stats().entries==1 && byteLimited.stats().bytes==256 && byteLimited.stats().evictions==1,"Asset byte budget not enforced");
	visual::AssetManager tooSmall(dir.path(),8,128);require(tooSmall.load("red.png") && tooSmall.stats().entries==0,"Oversized asset cached beyond byte budget");
	for(const auto &name:{"../red.png","/red.png","C:/red.png","red.jpg","nested/../red.png","https://example.com/red.png"})
		rejects([&]{assets.load(QString::fromLatin1(name));},"Invalid asset path accepted");
	rejects([&]{assets.load("missing.png");},"Missing asset accepted");
	QImage huge(4097,1,QImage::Format_RGBA8888_Premultiplied);huge.fill(Qt::red);require(huge.save(dir.filePath("huge.png")),"Oversize fixture");
	rejects([&]{assets.load("huge.png");},"Oversized PNG decoded");
	QFile corrupt(dir.filePath("corrupt.png"));require(corrupt.open(QIODevice::WriteOnly),"Corrupt fixture");corrupt.write("not a PNG");corrupt.close();
	rejects([&]{assets.load("corrupt.png");},"Corrupt PNG accepted");
	QFile tooLarge(dir.filePath("large.png"));require(tooLarge.open(QIODevice::WriteOnly),"Large fixture");tooLarge.write(QByteArray(4*1024*1024+1,'x'));tooLarge.close();
	rejects([&]{assets.load("large.png");},"Oversized encoded file accepted");
	bool offThread=false;std::thread worker([&]{try{assets.load("blue.png");}catch(const std::logic_error &){offThread=true;}});worker.join();require(offThread,"Asset loading allowed outside GUI thread");
	visual::VisualEngine engine;
	visual::Scene imageScene{QSize(16,16),{visual::ImageNode{QRect(0,0,16,16),first}}};
	const auto rendered=engine.prepare(imageScene);require(rendered->image.pixelColor(15,15)==Qt::red && engine.prepare(imageScene)==rendered,"Image draw/cache");
	imageScene.nodes={visual::ImageNode{QRect(0,0,16,16),second}};require(engine.prepare(imageScene)!=rendered && engine.prepare(imageScene)->image.pixelColor(0,0)==Qt::blue,"Different asset reused stale image");
	rejects([&]{engine.prepare({QSize(16,16),{visual::ImageNode{QRect(0,0,16,16),{}}}});},"Null asset accepted by renderer");
	QImage atlas(60,20,QImage::Format_RGBA8888_Premultiplied);atlas.fill(Qt::blue);
	{QPainter p(&atlas);p.fillRect(QRect(0,0,10,20),Qt::red);p.fillRect(QRect(50,0,10,20),Qt::green);}
	const auto atlasPtr=std::make_shared<const visual::ImageData>(visual::ImageData{atlas});
	const auto capped=engine.prepare({QSize(300,40),{visual::ImageNode{QRect(0,0,300,40),atlasPtr,{},10}}});
	require(capped->image.pixelColor(19,20)==Qt::red && capped->image.pixelColor(21,20)==Qt::blue && capped->image.pixelColor(280,20)==Qt::green,"End caps distorted by middle stretch");
	const auto cropped=engine.prepare({QSize(300,40),{visual::ImageNode{QRect(0,0,300,40),atlasPtr,QRect(10,0,40,20)}}});
	require(cropped!=capped && cropped->image.pixelColor(0,0)==Qt::blue,"Crop omitted from draw or cache key");
	rejects([&]{engine.prepare({QSize(16,16),{visual::ImageNode{QRect(0,0,16,16),first,QRect(7,0,3,8)}}});},"Invalid pixel crop accepted");
	rejects([&]{engine.prepare({QSize(16,16),{visual::ImageNode{QRect(0,0,16,16),first,{},5}}});},"Invalid end caps accepted");
	visual::TextNode weight;weight.box=QRect(0,0,100,40);weight.text="Sample";
	const auto regular=engine.prepare({QSize(100,40),{weight}});weight.bold=true;
	require(engine.prepare({QSize(100,40),{weight}})!=regular,"Font weight absent from cache key");
	// Preset-free callers retain the legacy layout. The factory Minimal's visual
	// redesign is intentional; existing OBS settings and selected fields survive.
	sources::BarSettings settings;
	sources::BarLabels labels{{"Follow","Sub","Cheer","Gift","Bits","Subs"},"Empty","Anonymous"};std::array<QString,6> values{"One","Two","Three","Four","100","6"};
	const auto now=visual::MotionClock::time_point(std::chrono::seconds(1));
	const auto minimal=sources::prepareSupporterFrame(engine,settings,labels,values,{},now);
	const auto originalMinimal=minimal->base->image.copy();
	const auto baseline=sources::prepareSupporterFrame(engine,sources::BarSettings{},labels,values,{},now);
	require(minimal->base==baseline->base,"Factory Minimal did not reuse legacy background");
	for(size_t i=0;i<6;++i)require(minimal->layers[i].frame==baseline->layers[i].frame,"Factory Minimal changed existing value layout");
	const auto alternate=presets::Registry::fromJson(document({preset("minimal"),preset("alternate",{{"background","#ff101030"},{"value","#ff00ff80"},{"family","Consolas"},{"padding",20},{"gap",8},{"backgroundImage","blue.png"}})}));
	settings.presetId="alternate";settings.style=alternate.resolve("alternate").style;
	const auto changed=sources::prepareSupporterFrame(engine,settings,labels,values,minimal,now,second);
	require(changed->base!=minimal->base && changed->layers[0].frame!=minimal->layers[0].frame && changed->layers[0].x==20,"Data-driven style not applied");
	require(minimal->settings.style==presets::Style{} && minimal->base->image==originalMinimal,"One source style changed another");
	const auto twin=sources::prepareSupporterFrame(engine,settings,labels,values,{},now,second);
	require(twin->base==changed->base && twin->layers[0].frame==changed->layers[0].frame,"Preset instances did not share renderer/cache");
	const auto stats=engine.stats();require(sources::prepareSupporterFrame(engine,settings,labels,values,changed,now,second)==changed && engine.stats().renders==stats.renders && engine.stats().hits==stats.hits,"Unchanged preset touched renderer");
	values[4]="101";const auto selective=sources::prepareSupporterFrame(engine,settings,labels,values,changed,now,second);
	require(selective->base==changed->base && engine.stats().renders==stats.renders+1,"Preset value update rebuilt unrelated resources");
	settings.fields.fill(false);const auto empty=sources::prepareSupporterFrame(engine,settings,labels,values,selective,now);
	require(!empty->base && !empty->background,"Hidden fields retained active background");
	visual::AssetManager uncached(dir.path(),0,0);require(uncached.load("red.png") && uncached.stats().entries==0,"Disabled asset cache retained data");
	// Atlas selection must retain the selected cell's resolution, rather than downscale all ten styles.
	require(atlas.save(dir.filePath("regions.png")),"Region fixture save");visual::AssetManager regions(dir.path());
	const auto redRegion=regions.loadRegion("regions.png",{0,0,167,1000}),greenRegion=regions.loadRegion("regions.png",{833,0,167,1000});
	require(redRegion->image.size()==QSize(10,20) && redRegion->image.pixelColor(0,0)==Qt::red && greenRegion->image.pixelColor(0,0)==Qt::green,"Wrong authored cell or lost resolution");
	require(regions.loadRegion("regions.png",{0,0,167,1000})==redRegion && regions.stats().loads==2,"Selected cell not cached independently");
	rejects([&]{regions.loadRegion("regions.png",{990,0,100,1000});},"Out-of-bounds authored region accepted");
	assets.clear();require(assets.stats().entries==0 && first->image.pixelColor(0,0)==Qt::red,"Asset clear invalidated retained image");
	require(rendered->assets.size()==1 && rendered->assets[0]==first,"Cached frame did not retain immutable image identity");
	std::cout<<"PASS: ten reference-based factory styles at min/max size, Unicode, independent fields, idle/shared/selective frames; ornamental frames/cards/shared atlases, bounded authored PNG decoding, crop/end caps/weight keys; registry limits/fallback, immutable assets, legacy layout compatibility and cleanup\n";return 0;
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}}
