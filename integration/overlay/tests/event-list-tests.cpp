// SPDX-License-Identifier: GPL-2.0-or-later
#include "sources/event-list-model.hpp"
#include "sources/event-list-properties.hpp"
#include "core/plugin-core.hpp"
#include "events/simulation.hpp"
#include <QGuiApplication>
#include <iostream>
#include <stdexcept>
#include <QFileInfo>
#include <QDir>
#include "visual/asset-manager.hpp"
using namespace oto;
namespace {
void require(bool ok,const char *why){if(!ok)throw std::runtime_error(why);}
struct Config:IConfigStore {ConfigLoadResult load() override{return {ConfigStatus::Loaded,{false}};}bool save(Settings)override{return true;}};
struct Logger:ILogger {void write(LogLevel,std::string_view)const noexcept override{}};
struct Disk:IDailyStore {state::DailyBook book;int writes=0;bool fail=false;state::DailyBook load()override{return book;}bool save(const state::DailyBook &b)override{if(fail)return false;book=b;++writes;return true;}};
const sources::ListLabels labels{"Anonymous","%1 Bits","%1 Subs","%1 viewers","Sub ×1","%1 months"};
const state::ChannelKey channel{events::EventOrigin::Simulation,"sim-channel"};
void checkRightDetails(visual::VisualEngine &engine,const sources::ListSettings &settings,const sources::ListView &view,visual::ImagePtr artwork,visual::MotionClock::time_point clock)
{
	auto blankView=view;blankView.items[0].detail.clear();
	const auto blank=sources::prepareEventListFrame(engine,settings,labels,blankView,{},clock,false,artwork);
	for(const auto *value:{"500 Bits","Sub ×1","999999 viewers"}) {
		auto quantityView=view;quantityView.items[0].detail=QString::fromUtf8(value);
		const auto frame=sources::prepareEventListFrame(engine,settings,labels,quantityView,{},clock,false,artwork);
		QRect changed;
		const auto &pixels=frame->rows[0].base->image;const auto &emptyPixels=blank->rows[0].base->image;
		for(int y=0;y<pixels.height();++y)for(int x=0;x<pixels.width();++x)
			if(pixels.pixel(x,y)!=emptyPixels.pixel(x,y))changed|=QRect(x,y,1,1);
		require(!changed.isEmpty() && changed.right()<settings.width-28 && changed.right()>=settings.width-36,"Quantity is not aligned to the right inner edge");
		require(changed.left()>=frame->rows[0].nameX+frame->rows[0].nameWidth+12,"Quantity overlaps the name viewport");
		require(blank->rows[0].nameWidth>frame->rows[0].nameWidth,"Empty quantity column still consumes name space");
	}
}
}
int main(int argc,char **argv)
{
	QGuiApplication app(argc,argv);
	try {
		auto disk=std::make_unique<Disk>();auto *storage=disk.get();PluginCore core(std::make_unique<Config>(),std::make_unique<Logger>(),std::move(disk));core.start();
		const auto now=std::chrono::system_clock::now();int sequence=1000;
		auto submit=[&](events::EventType type,int age){auto draft=events::makeSimulation(type,++sequence,now-std::chrono::seconds(age));auto event=core.processEvent(std::move(draft));require(bool(event),"Fixture admission failed");return event.event;};
		sources::ListSettings settings;visual::VisualEngine engine;const auto clock=visual::MotionClock::now();
		const auto registry=presets::Registry::fromFile(QString::fromUtf8(OTO_PRESET_FILE));
		require(registry.resolve("minimal").eventListStyle.has_value() && registry.resolve("neon-dj").eventListStyle.has_value() && registry.resolve("cyberpunk").eventListStyle.has_value() && registry.resolve("retro-crt").eventListStyle.has_value() && registry.resolve("space").eventListStyle.has_value() && registry.resolve("horror").eventListStyle.has_value() && registry.resolve("love").eventListStyle.has_value() && registry.resolve("graffiti").eventListStyle.has_value() && registry.resolve("urban-city").eventListStyle.has_value() && registry.resolve("futuristic-hologram").eventListStyle.has_value(),"Event List preset availability wrong");
		settings.style=*registry.resolve("minimal").eventListStyle;settings.family=settings.style.family;
		const auto empty=core.sharedSnapshot();const auto followA=submit(events::EventType::Follow,25),followB=submit(events::EventType::Follow,2),raid=submit(events::EventType::Raid,10);
		require(core.sharedSnapshot()==empty,"History published before commit");core.processPendingEvents();
		const auto snapshot=core.sharedSnapshot();const auto &history=snapshot->channels.at(channel)->activity;
		require(history.size()==3 && history[0].event==followB && history[1].event==raid && history[2].event==followA,"Repeated same-type events lost or queue priority used as chronology");
		require(history[0].displayName==snapshot->channels.at(channel)->latest[0].displayName,"History copied or re-interned actor");
		require(!core.processEvent(followB->data()) && core.processPendingEvents().snapshot==snapshot,"Duplicate entered history");
		auto view=sources::eventListView(snapshot,settings,labels,channel);require(view.items.size()==3,"Initial history wrong");
		const auto quiet=sources::prepareEventListFrame(engine,settings,labels,view,{},clock);require(quiet->rows[0].entered==visual::MotionClock::time_point{},"Scene load replayed history");
		require(sources::prepareEventListFrame(engine,settings,labels,view,quiet,clock)==quiet,"Idle rebuilt plan");
		{
		 auto *data=obs_data_create();sources::fonts::defaults(data,"Segoe UI");
		 obs_data_set_bool(data,"use_style_font",false);obs_data_set_bool(data,"use_style_label_font",false);
		 obs_data_set_bool(data,"use_style_name_color",true);obs_data_set_bool(data,"use_style_label_color",true);
		 auto *font=obs_data_create();obs_data_set_string(font,"face","Segoe UI");
		 for(const int points:{6,12,18,24,36,96}){
		  obs_data_set_int(font,"size",points);obs_data_set_obj(data,"name_font",font);obs_data_set_obj(data,"label_font",font);
		  sources::ListSettings custom=settings;sources::readListTypography(custom,data);
		  require(custom.namePixelSize==points*4/3 && custom.labelPixelSize==points*4/3,"Native font point size not applied to Event List");
		  auto sample=view;sample.items.resize(1);sample.items[0].name="Hg";sample.items[0].badge="Hg";sample.items[0].detail="Hg";
		  for(const auto &definition:registry.entries())for(const int height:{48,64,96}){
		   auto skin=custom;skin.style=*definition.eventListStyle;skin.rowHeight=height;
		   visual::AssetManager assets(QDir(QFileInfo(OTO_PRESET_FILE).absolutePath()).filePath("assets"));
		   const auto art=skin.style.backgroundImage.isEmpty()?visual::ImagePtr{}:assets.load(skin.style.backgroundImage);
		   const auto result=sources::prepareEventListFrame(engine,skin,labels,sample,{},clock,false,art);
		   const auto &row=result->rows[0];const auto &image=row.name->image;QRect ink;
		   for(int y=0;y<image.height();++y)for(int x=0;x<image.width();++x)if(image.pixelColor(x,y).alpha()>100)ink|=QRect(x,y,1,1);
		   if(ink.isEmpty() || std::abs((ink.top()+ink.bottom()+1)/2.-height/2.)>6)std::cerr<<definition.id.toStdString()<<" points="<<points<<" height="<<height<<" ink="<<ink.top()<<","<<ink.bottom()<<" metrics="<<row.name->textMetrics[0].height<<" pixel="<<row.name->textMetrics[0].pixelSize<<'\n';
		   require(!ink.isEmpty() && std::abs((ink.top()+ink.bottom()+1)/2.-height/2.)<=6,"Name text not vertically centred");
		   require(row.name->textMetrics[0].height<=height && row.base->image.height()==height,"Custom fonts clip the cell vertically");
		   auto blank=sample;blank.items[0].badge.clear();blank.items[0].detail.clear();
		   const auto under=sources::prepareEventListFrame(engine,skin,labels,blank,{},clock,false,art);
		   QRect badgeInk,detailInk;const int split=skin.width-28-skin.width/4;
		   for(int y=0;y<height;++y)for(int x=0;x<skin.width;++x)if(row.base->image.pixel(x,y)!=under->rows[0].base->image.pixel(x,y)){
		    if(x>=split)detailInk|=QRect(x,y,1,1);else badgeInk|=QRect(x,y,1,1);
		   }
		   for(const auto &bounds:{badgeInk,detailInk})require(!bounds.isEmpty() && std::abs((bounds.top()+bounds.bottom()+1)/2.-height/2.)<=6,"Label or quantity not vertically centred");
		  }
		 }
		 obs_data_set_int(font,"size",12);obs_data_set_obj(data,"name_font",font);obs_data_set_obj(data,"label_font",font);
		 sources::ListSettings small=settings;sources::readListTypography(small,data);const auto first=sources::prepareEventListFrame(engine,small,labels,view,quiet,clock);
		 obs_data_set_int(font,"size",24);obs_data_set_obj(data,"name_font",font);obs_data_set_obj(data,"label_font",font);
		 sources::ListSettings big=settings;sources::readListTypography(big,data);const auto second=sources::prepareEventListFrame(engine,big,labels,view,first,clock);
		 require(second!=first && second->rows[0].name->textMetrics[0].pixelSize>first->rows[0].name->textMetrics[0].pixelSize && second->rows[0].base!=first->rows[0].base,"Size-only changes reuse old Event List text");
		 for(const auto &row:second->rows)require(row.entered==visual::MotionClock::time_point{},"Font preference replays an event");
		 obs_data_set_int(data,"name_color",0x00ff00);obs_data_set_int(data,"label_color",0xff00ff);
		 auto *properties=obs_properties_create();auto *nc=obs_properties_add_color(properties,"name_color","Names");auto *lc=obs_properties_add_color(properties,"label_color","Labels");
		 sources::listColorInheritanceModified(properties,nullptr,data);require(!obs_property_enabled(nc) && !obs_property_enabled(lc) && obs_data_get_bool(data,"use_style_name_color"),"Opening properties disables style colors");
		 obs_data_set_bool(data,"use_style_name_color",false);obs_data_set_bool(data,"use_style_label_color",false);sources::listColorInheritanceModified(properties,nullptr,data);require(obs_property_enabled(nc) && obs_property_enabled(lc),"Custom color selectors remain disabled");
		 sources::ListSettings colored=big;sources::readListTypography(colored,data);const auto customColor=sources::prepareEventListFrame(engine,colored,labels,view,second,clock);
		 require(!colored.inheritNameColor && !colored.inheritLabelColor && customColor->rows[0].name->image!=second->rows[0].name->image && customColor->rows[0].base->image!=second->rows[0].base->image,"Choosing colors ignored or stale");
		 require(customColor->rows[0].accent==second->rows[0].accent,"Text colors changed the style's icon or animation");
		 obs_data_set_bool(data,"use_style_font",true);obs_data_set_bool(data,"use_style_label_font",true);obs_data_set_bool(data,"use_style_name_color",true);obs_data_set_bool(data,"use_style_label_color",true);
		 sources::ListSettings inherited=settings;sources::readListTypography(inherited,data);const auto restored=sources::prepareEventListFrame(engine,inherited,labels,view,customColor,clock);
		 require(!inherited.namePixelSize && !inherited.labelPixelSize && inherited.inheritNameColor && inherited.inheritLabelColor && restored->rows[0].name->image==quiet->rows[0].name->image && restored->rows[0].base->image==quiet->rows[0].base->image,"Restoring the style changed original typography");
		 require(sources::prepareEventListFrame(engine,inherited,labels,view,restored,clock)==restored,"Typography changes cause continuous reraster");
		 obs_properties_destroy(properties);obs_data_release(font);obs_data_release(data);
		}
		const auto renders=engine.stats().renders;
		const auto unchanged=sources::prepareEventListFrame(engine,settings,labels,sources::eventListView(snapshot,settings,labels,channel),quiet,clock+std::chrono::hours(24));
		require(unchanged==quiet && engine.stats().renders==renders,"Passing time rebuilt rows or replayed entrance");
		settings.types[0]=false;view=sources::eventListView(snapshot,settings,labels,channel);require(view.items.size()==1 && view.items[0].event==raid,"Per-source filter wrong");settings.types[0]=true;
		const auto newest=submit(events::EventType::Cheer,0);core.processPendingEvents();view=sources::eventListView(core.sharedSnapshot(),settings,labels,channel);
		const auto changed=sources::prepareEventListFrame(engine,settings,labels,view,quiet,clock);
		require(changed->rows[0].item.event==newest && changed->rows[0].entered==clock && changed->rows[1].base==quiet->rows[0].base,"New entry or retained row sharing failed");
		require(sources::listRowY(changed->rows[1],clock)==quiet->rows[0].y && sources::listRowY(changed->rows[1],clock+std::chrono::milliseconds(500))==changed->rows[1].y,"Row shift endpoints failed");
		const auto rapidEvent=submit(events::EventType::Gift,0);core.processPendingEvents();const auto rapid=sources::prepareEventListFrame(engine,settings,labels,sources::eventListView(core.sharedSnapshot(),settings,labels,channel),changed,clock+std::chrono::milliseconds(200));
		require(rapid->rows[2].item.event==followB && rapid->rows[2].fromY==sources::listRowY(changed->rows[1],clock+std::chrono::milliseconds(200)),"Rapid update jumped rows or lost chronological tie order");
		const auto late=submit(events::EventType::Follow,100);core.processPendingEvents();require(core.sharedSnapshot()->channels.at(channel)->activity.back().event==late && snapshot->channels.at(channel)->activity.size()==3,"Late event/order/immutable snapshot failed");
		for(int i=0;i<100;++i){submit(events::EventType::Chat,0);if(i%25==24)core.processPendingEvents();}
		require(core.sharedSnapshot()->channels.at(channel)->activity.size()==6,"Chat evicted activity");
		for(int i=0;i<80;++i){submit(events::EventType::Follow,i);if(i%25==24)core.processPendingEvents();}while(core.pendingEventCount())core.processPendingEvents();
		require(core.sharedSnapshot()->channels.at(channel)->activity.size()==state::ChannelState::activityCapacity,"History unbounded");
		const auto book=storage->book;const auto writes=storage->writes;const auto real=core.sharedSnapshot()->channels.at(channel);
		core.simulateEvent(events::EventType::Sub);const auto demo=core.createSourcePreview(false,true),longDemo=core.createSourcePreview(true,true);
		require(storage->book==book && storage->writes==writes && core.pendingEventCount()==1 && core.sharedSnapshot()->channels.at(channel)==real,"Demo touched persistence/real channel/unrelated pending queue");
		const auto demoView=sources::eventListView(core.sharedSnapshot(),settings,labels,{events::EventOrigin::Preview,demo});
		require(demoView.items.size()==8 && demoView.items[0].badge=="BITS" && demoView.items[0].name.startsWith("TEST") && demoView.items[0].detail=="500 Bits","Eight-row sample wrong");
		const auto demoFrame=sources::prepareEventListFrame(engine,settings,labels,demoView,{},clock,true);
		for(int opacity:{0,50,100}) {
			auto alphaSettings=settings;alphaSettings.backgroundOpacity=opacity;
			const auto alpha=sources::prepareEventListFrame(engine,alphaSettings,labels,demoView,{},clock);
			require(std::abs(alpha->rows[0].base->image.pixelColor(400,32).alpha()-239*opacity/100)<=1,"Minimal fill opacity incorrect");
			require(alpha->rows[0].name==demoFrame->rows[0].name && alpha->rows[0].entered==visual::MotionClock::time_point{},"Minimal opacity changed names/replayed events");
		}
		for(int opacity:{-1,101}) {auto invalid=settings;invalid.backgroundOpacity=opacity;bool rejected=false;try{sources::prepareEventListFrame(engine,invalid,labels,demoView,{},clock);}catch(const std::invalid_argument &){rejected=true;}require(rejected,"Invalid opacity accepted");}
		events::EventEngine rapidFixtures;rapidFixtures.start();
		for(int count:{1,3,8,10}) {
			auto fullSettings=settings;fullSettings.rows=count;
			const auto previewView=sources::eventListView(core.sharedSnapshot(),fullSettings,labels,{events::EventOrigin::Preview,demo},true);
			require(previewView.items.size()==std::size_t(count+1),"TEST lacks a full list plus its new event");
			auto previousView=previewView;previousView.items.erase(previousView.items.begin());
			const auto previousFull=sources::prepareEventListFrame(engine,fullSettings,labels,previousView,{},clock);
			const auto inserted=sources::prepareEventListFrame(engine,fullSettings,labels,previewView,{},clock,true);
			require(inserted->rows.size()==std::size_t(count) && inserted->exiting.size()==1 && inserted->exiting[0].base==previousFull->rows.back().base,"Bottom card vanished instead of exiting with shared texture");
			require(inserted->exiting[0].fromY==previousFull->rows.back().y && inserted->exiting[0].y==previousFull->rows.back().y+fullSettings.rowHeight+8,"Bottom card exit direction wrong");
			for(int row=1;row<count;++row) {
				require(inserted->rows[row].base==previousFull->rows[row-1].base && sources::listRowY(inserted->rows[row],clock)==previousFull->rows[row-1].y,"TEST did not retain and move older rows");
				require(sources::listRowY(inserted->rows[row],clock+std::chrono::milliseconds(250))>previousFull->rows[row-1].y && sources::listRowY(inserted->rows[row],clock+std::chrono::milliseconds(250))<inserted->rows[row].y,"Older rows do not move during insertion");
			}
			auto rapidView=inserted->view;const auto rapidClock=clock+std::chrono::milliseconds(150);
			rapidView.items.insert(rapidView.items.begin(),{rapidFixtures.process(events::makeSimulation(events::EventType::Cheer,9000+count,now)).event,"rapid","BITS","100 Bits"});rapidView.items.resize(count);
			const auto rapidFull=sources::prepareEventListFrame(engine,fullSettings,labels,rapidView,inserted,rapidClock);
			require(rapidFull->exiting.size()<=std::size_t(count),"Rapid updates accumulate unbounded exit textures");
			if(count>1)require(rapidFull->rows[1].fromY==sources::listRowY(inserted->rows[0],rapidClock),"Rapid insertion jumps retained row");
			for(const auto &exit:rapidFull->exiting)if(exit.item.event==inserted->exiting[0].item.event)require(exit.exitStarted==clock,"Rapid insertion restarted exit fade");
			fullSettings.animate=false;const auto noAnimation=sources::prepareEventListFrame(engine,fullSettings,labels,previewView,{},clock,true);
			require(noAnimation->exiting.empty() && noAnimation->rows[0].entered==visual::MotionClock::time_point{},"Disabled animation retains exit or entrance");
		}
		checkRightDetails(engine,settings,demoView,{},clock);
		for(int duration:{1000,3000,6000,12000}) {
			auto timing=settings;timing.durationMs=duration;
			const auto fullView=sources::eventListView(core.sharedSnapshot(),timing,labels,{events::EventOrigin::Preview,demo},true);
			const auto timed=sources::prepareEventListFrame(engine,timing,labels,fullView,{},clock,true);
			const auto at=[&](double fraction){return clock+std::chrono::duration_cast<visual::MotionClock::duration>(std::chrono::duration<double,std::milli>(duration*fraction));};
			require(timed->exiting.size()==1 && timed->rows[0].scrollStarted==at(1.0/6),"Duration does not scale name scrolling delay");
			const auto &moving=timed->rows[1];
			require(std::abs(sources::listRowY(moving,at(1.0/24),duration)-(moving.fromY+moving.y)/2)<.001f && sources::listRowY(moving,at(1.0/12),duration)==moving.y,"Row shift ignores selected duration");
			auto nextView=timed->view;nextView.items.insert(nextView.items.begin(),{rapidFixtures.process(events::makeSimulation(events::EventType::Cheer,10000+duration,now)).event,"timed rapid","BITS","200 Bits"});nextView.items.resize(timing.rows);
			const auto resumed=sources::prepareEventListFrame(engine,timing,labels,nextView,timed,at(1.0/24));
			require(std::abs(resumed->rows[2].fromY-sources::listRowY(moving,at(1.0/24),duration))<.001f,"Selected duration jumps during rapid insertion");
			for(const auto mode:{sources::ListEntrance::Appearance,sources::ListEntrance::Left,sources::ListEntrance::Right,sources::ListEntrance::Top}) {
				const auto baseline=sources::sampleListEntry(750,mode),scaled=sources::sampleListEntry(duration/8.0,mode,600,64,duration);
				require(std::abs(baseline.text-scaled.text)<.0001f && std::abs(baseline.glow-scaled.glow)<.0001f && std::abs(baseline.particles-scaled.particles)<.0001f && std::abs(baseline.sweep-scaled.sweep)<.0001f && std::abs(baseline.offsetX-scaled.offsetX)<.0001f && std::abs(baseline.offsetY-scaled.offsetY)<.0001f,"Duration fails to scale the full choreography");
				require(!sources::sampleListEntry(duration-1,mode,600,64,duration).finished && sources::sampleListEntry(duration,mode,600,64,duration).finished,"Duration does not settle at selected endpoint");
			}
			const auto preference=sources::prepareEventListFrame(engine,timing,labels,demoView,demoFrame,clock+std::chrono::seconds(1));
			if(duration!=6000)require(preference->rows[0].entered==visual::MotionClock::time_point{} && preference->exiting.empty(),"Duration preference replays existing entries");
		}
		for(int duration:{999,12001}) {
			auto invalid=settings;invalid.durationMs=duration;bool rejected=false;
			try{sources::prepareEventListFrame(engine,invalid,labels,demoView,{},clock);}catch(const std::invalid_argument &){rejected=true;}
			require(rejected,"Invalid duration accepted");
		}
		auto customLabelSettings=settings;customLabelSettings.labelFamily="Consolas";
		const auto customLabels=sources::prepareEventListFrame(engine,customLabelSettings,labels,demoView,demoFrame,clock);
		for(std::size_t i=0;i<demoFrame->rows.size();++i) {
			require(customLabels->rows[i].base!=demoFrame->rows[i].base && customLabels->rows[i].name==demoFrame->rows[i].name,"Label font ignored or changed names");
			require(customLabels->rows[i].entered==visual::MotionClock::time_point{},"Label preference replayed event");
		}
		auto bothFontSettings=customLabelSettings;bothFontSettings.family="Segoe Print";
		const auto bothFonts=sources::prepareEventListFrame(engine,bothFontSettings,labels,demoView,customLabels,clock);
		require(bothFonts->rows[0].base==customLabels->rows[0].base && bothFonts->rows[0].name!=customLabels->rows[0].name,"Name font changed labels");
		const auto restoredFonts=sources::prepareEventListFrame(engine,settings,labels,demoView,bothFonts,clock);
		require(restoredFonts->rows[0].base->image==demoFrame->rows[0].base->image && restoredFonts->rows[0].name->image==demoFrame->rows[0].name->image,"Style inheritance failed to restore original list pixels");
		visual::AssetManager assets(QFileInfo(QString::fromUtf8(OTO_PRESET_FILE)).dir().filePath("assets"));
		for(const auto *presetId:{"neon-dj","cyberpunk","retro-crt","space","horror","love","graffiti","urban-city","futuristic-hologram"}) {
		auto neonSettings=settings;neonSettings.presetId=presetId;neonSettings.style=*registry.resolve(presetId).eventListStyle;neonSettings.family=neonSettings.style.family;
		const auto artwork=assets.load(neonSettings.style.backgroundImage);require(artwork->image.width()==1024 && artwork->image.pixelColor(0,0).alpha()==0,"Decorated artwork size/transparency wrong");
		if(neonSettings.presetId=="retro-crt")for(int gap:{211,402,590,777})
			require(artwork->image.pixelColor(artwork->image.width()/2,artwork->image.height()*gap/1000).alpha()<=8,"Retro CRT atlas contains a backdrop between cards");
		if(neonSettings.presetId=="space")for(int gap:{224,413,604,793})
			require(artwork->image.pixelColor(artwork->image.width()/2,artwork->image.height()*gap/1000).alpha()<=8,"Space atlas contains a backdrop between cards");
		if(neonSettings.presetId=="horror")for(int gap:{218,408,601,783})
			require(artwork->image.pixelColor(artwork->image.width()/2,artwork->image.height()*gap/1000).alpha()<=8,"Horror atlas contains a backdrop between cards");
		if(neonSettings.presetId=="love")for(int gap:{213,394,590,776})
			require(artwork->image.pixelColor(artwork->image.width()/2,artwork->image.height()*gap/1000).alpha()<=8,"Love atlas contains a backdrop between cards");
		if(neonSettings.presetId=="graffiti")for(int gap:{220,434,634,830})
			require(artwork->image.pixelColor(artwork->image.width()/2,artwork->image.height()*gap/1000).alpha()<=8,"Graffiti atlas contains a backdrop between cards");
		if(neonSettings.presetId=="urban-city")for(int gap:{205,391,586,776})
			require(artwork->image.pixelColor(artwork->image.width()/2,artwork->image.height()*gap/1000).alpha()<=8,"Urban atlas contains a backdrop between cards");
		if(neonSettings.presetId=="futuristic-hologram")for(int gap:{225,410,596,778})
			require(artwork->image.pixelColor(artwork->image.width()/2,artwork->image.height()*gap/1000).alpha()<=8,"Hologram atlas contains a backdrop between cards");
		if(neonSettings.presetId=="love" || neonSettings.presetId=="graffiti" || neonSettings.presetId=="urban-city" || neonSettings.presetId=="futuristic-hologram")for(const auto &crop:neonSettings.style.rowCrops) {
			// The right-aligned amount must stay on dark glass, inside the endcap.
			const int right=artwork->image.width()*(crop.x()+crop.width()*953/1000)/1000;
			for(int y:{450,500,550}) {
				const auto background=artwork->image.pixelColor(right,artwork->image.height()*(crop.y()+crop.height()*y/1000)/1000);
				require(background.alpha()>200 && qGray(background.rgb())<70,"Quantities cross artwork or leave the decorated card");
			}
		}
		const auto neon=sources::prepareEventListFrame(engine,neonSettings,labels,demoView,{},clock,true,artwork);
		const auto atlasKey=artwork->image.cacheKey();
		for(int opacity:{0,50}) {
			auto fadedSettings=neonSettings;fadedSettings.backgroundOpacity=opacity;
			const auto faded=sources::prepareEventListFrame(engine,fadedSettings,labels,demoView,neon,clock,false,artwork);
			require(faded->rows[0].entered==visual::MotionClock::time_point{} && faded->rows[0].name==neon->rows[0].name,"Opacity replayed events or rebuilt names");
			QPoint point;bool found=false;
			for(int x=neon->rows[0].nameX+20;x<neon->rows[0].nameX+neon->rows[0].nameWidth-20;++x) {
				const auto p=neon->rows[0].base->image.pixelColor(x,32);
				if(p.alpha()>200 && std::max({p.red(),p.green(),p.blue()})<90){point={x,32};found=true;break;}
			}
			require(found,"No dark interior for opacity verification");
			const int before=neon->rows[0].base->image.pixelColor(point).alpha(),after=faded->rows[0].base->image.pixelColor(point).alpha();
			require(std::abs(after-before*opacity/100)<=1,"Dark interior opacity incorrect");
			for(int y=0;y<5;++y)for(int x=0;x<neonSettings.width;++x)require(faded->rows[0].base->image.pixel(x,y)==neon->rows[0].base->image.pixel(x,y),"Opacity changed frame perimeter");
			for(int x=0;x<60;++x)for(int y=0;y<neonSettings.rowHeight;++y)require(faded->rows[0].base->image.pixel(x,y)==neon->rows[0].base->image.pixel(x,y),"Opacity faded icons");
			const auto prepared=engine.stats().renders;require(sources::prepareEventListFrame(engine,fadedSettings,labels,demoView,faded,clock,false,artwork)==faded && engine.stats().renders==prepared,"Opacity adds continuous preparation");
		}
		require(artwork->image.cacheKey()==atlasKey,"Opacity mutated shared artwork");
		auto emptyArtworkView=demoView;emptyArtworkView.items.clear();
		const auto waiting=sources::prepareEventListFrame(engine,neonSettings,labels,emptyArtworkView,{},clock);
		auto firstArtworkView=demoView;firstArtworkView.items.resize(1);
		const auto firstArtwork=sources::prepareEventListFrame(engine,neonSettings,labels,firstArtworkView,waiting,clock,false,artwork);
		require(firstArtwork->rows[0].entered==clock,"Loading artwork for the first live event suppressed entrance");
		require(neon->background==artwork && neon->rows[0].accent!=neon->rows[1].accent && neon->rows[2].accent!=neon->rows[3].accent,"Neon artwork retention/event palette wrong");
		const auto warmed=engine.stats().renders;
		require(sources::prepareEventListFrame(engine,neonSettings,labels,demoView,neon,clock,false,artwork)==neon && engine.stats().renders==warmed,"Neon idle rebuilt frames");
		const auto switched=sources::prepareEventListFrame(engine,neonSettings,labels,demoView,demoFrame,clock,false,artwork);require(switched->rows[0].entered==visual::MotionClock::time_point{},"Style change replayed entries");
		for(int width:{420,600,1200})for(int height:{48,64,96}){auto bounds=neonSettings;bounds.width=width;bounds.rowHeight=height;const auto frame=sources::prepareEventListFrame(engine,bounds,labels,demoView,{},clock,false,artwork);require(frame->rows[0].nameWidth>0 && frame->rows[0].base->image.size()==QSize(width,height),"Decorated layout dimensions invalid");checkRightDetails(engine,bounds,demoView,artwork,clock);}
		assets.clear();require(neon->background==artwork && !neon->rows[0].base->image.isNull(),"Eviction invalidated active list artwork");
		}
		require(demoFrame->rows[0].entered==clock && demoFrame->rows[1].entered==visual::MotionClock::time_point{},"Demo replayed old rows");
		const auto longView=sources::eventListView(core.sharedSnapshot(),settings,labels,{events::EventOrigin::Preview,longDemo});
		const auto longFrame=sources::prepareEventListFrame(engine,settings,labels,longView,{},clock);
		require(longFrame->hasScrolling && longFrame->rows[0].scrollDistance>0 && longFrame->rows[0].name->image.width()<=2048 && longView.items[0].name.contains(QString::fromUtf8("東京")),"Unicode/strip cap/ping-pong failed");
		require(demoFrame->rows[0].base->image.pixelColor(0,0).alpha()==0 && demoFrame->rows[0].base->image.pixelColor(300,32).alpha()>200,"Backdrop present or card missing");
		core.releaseSourcePreview(demo);require(core.sharedSnapshot()->channels.count({events::EventOrigin::Preview,longDemo})==1,"Ending one demo removed another");core.releaseSourcePreview(longDemo);
		storage->fail=true;const auto beforeFailure=core.sharedSnapshot();bool failed=false;try{core.processPendingEvents();}catch(...){failed=true;}
		require(failed && core.sharedSnapshot()==beforeFailure && core.pendingEventCount()==1,"Persistence failure partially published history");storage->fail=false;core.processPendingEvents();
		core.stop();core.start();require(core.sharedSnapshot()->channels.at(channel)->activity.empty() && storage->book==state::dailyBook(core.sharedSnapshot()),"History replayed/persisted or daily totals lost after restart");
		const auto initial=sources::sampleListEntry(0),textStage=sources::sampleListEntry(750),end=sources::sampleListEntry(6000);
		require(initial.card==0 && initial.text==0 && textStage.card==1 && textStage.text>0 && textStage.text<1 && end.finished && end.glow==0 && end.text==1,"Four-stage timeline wrong");
		for(const auto mode:{sources::ListEntrance::Appearance,sources::ListEntrance::Left,sources::ListEntrance::Right,sources::ListEntrance::Top}) {
			const auto start=sources::sampleListEntry(0,mode,1200,96),half=sources::sampleListEntry(250,mode,1200,96),arrived=sources::sampleListEntry(500,mode,1200,96),done=sources::sampleListEntry(6000,mode,1200,96);
			require(start.card==0 && start.text==0 && arrived.offsetX==0 && arrived.offsetY==0 && arrived.card==1 && done.finished && done.glow==0 && done.offsetX==0 && done.offsetY==0,"Directional entrance does not settle exactly");
			if(mode==sources::ListEntrance::Left)require(start.offsetX<=-1200 && half.offsetX<0 && half.offsetX>start.offsetX && start.offsetY==0,"Left entry direction or dimensions wrong");
			if(mode==sources::ListEntrance::Right)require(start.offsetX>=1200 && half.offsetX>0 && half.offsetX<start.offsetX && start.offsetY==0,"Right entry direction or dimensions wrong");
			if(mode==sources::ListEntrance::Top)require(start.offsetY<=-108 && half.offsetY<0 && half.offsetY>start.offsetY && start.offsetX==0,"Top entry direction or dimensions wrong");
			auto choice=demoFrame->settings;choice.entrance=mode;
			const auto selected=sources::prepareEventListFrame(engine,choice,labels,demoView,demoFrame,clock+std::chrono::seconds(2));
			if(mode!=sources::ListEntrance::Appearance)require(selected!=demoFrame && selected->rows[0].entered==visual::MotionClock::time_point{},"Changing entry side replayed old events");
			choice.animate=false;const auto disabled=sources::prepareEventListFrame(engine,choice,labels,demoView,{},clock,true);
			require(disabled->rows[0].entered==visual::MotionClock::time_point{},"Disabled entry animation still runs");
		}
		visual::Scene pathScene{QSize(32,32),{}};QPainterPath p;p.addEllipse(4,4,24,24);pathScene.nodes.push_back(visual::PathNode{p,Qt::white,Qt::transparent,2});const auto path=engine.prepare(pathScene);require(engine.prepare(pathScene)==path,"Vector paths not cached");std::get<visual::PathNode>(pathScene.nodes[0]).width=3;require(engine.prepare(pathScene)!=path,"Vector width missing in cache key");std::get<visual::PathNode>(pathScene.nodes[0]).width=100;failed=false;try{engine.prepare(pathScene);}catch(...){failed=true;}require(failed,"Invalid vector path accepted");
		std::cout<<"PASS: bounded chronological history, batch repetitions/dedup, immutable sharing, quiet frame reuse, filters, finite entrance/row shift/rapid updates, preview isolation/pending preservation, Unicode ping-pong, transparent cards, rollback/restart, cached bounded vector icons\n";return 0;
	}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
}
