// SPDX-License-Identifier: GPL-2.0-or-later
#include "sources/supporter-frame.hpp"
#include "sources/supporter-typography.hpp"
#include <QGuiApplication>
#include <QPainter>
#include <algorithm>
#include <iostream>
#include <stdexcept>
using namespace oto;
void require(bool value,const char *why) { if(!value) throw std::runtime_error(why); }
int main(int argc,char **argv)
{
	QGuiApplication app(argc,argv);
	try {
		using namespace visual; using namespace sources;
		const auto start=sampleMotion(Motion::SlideFade,0,250);
		const auto mid=sampleMotion(Motion::SlideFade,125,250);
		const auto end=sampleMotion(Motion::SlideFade,250,250);
		require(start.opacity==0 && start.offsetY==8 && !start.finished,"Bad entrance start");
		require(mid.opacity==0.5f && mid.offsetY==4 && !mid.finished,"Bad midpoint/easing");
		require(end.opacity==1 && end.offsetY==0 && end.finished,"Animation did not settle");
		require(sampleMotion(Motion::None,0,250).finished && sampleMotion(Motion::Fade,100,0).finished,"Disabled/zero duration");
		require(sampleMotion(Motion::Fade,-1,250).opacity==0 && sampleMotion(Motion::Fade,10000,250).opacity==1,"Timeline bounds");
		VisualEngine engine;
		BarSettings settings; BarLabels labels{{"Follow","Sub","Cheer","Gift","Bits","Subs"},"Empty","Anonymous"};
		std::array<QString,6> values{"One","Two","Three","Four","100","6"};
		const auto now=MotionClock::time_point(std::chrono::seconds(1));
		const auto initial=prepareSupporterFrame(engine,settings,labels,values,{},now);
		for(const auto mode:{Motion::None,Motion::Fade,Motion::SlideFade,Motion::StyleChange}){
			auto options=settings;options.motion=mode;
			BarContext simulated;simulated.generation=1;simulated.latestRevision={4,5,6,7};simulated.delivered={1,1,1,1};
			const auto loaded=prepareSupporterFrame(engine,options,labels,values,{},now,{},{},simulated);
			for(const auto &layer:loaded->layers)require(layer.started==MotionClock::time_point{} && !layer.previousValue,"Startup animated restored fields");
			auto real=simulated;real.channel={events::EventOrigin::Twitch,"123"};real.latestRevision={};real.delivered={};
			auto actual=values;actual[0]="ActualFollower";actual[1]=actual[2]=actual[3]="Empty";actual[4]=actual[5]="0";
			const auto connected=prepareSupporterFrame(engine,options,labels,actual,loaded,now,{},{},real);
			for(const auto &layer:connected->layers)require(layer.started==MotionClock::time_point{} && !layer.previousValue,"Channel switch animated the first three fields");
			actual[0]="FetchedFollower";
			const auto backfill=prepareSupporterFrame(engine,options,labels,actual,connected,now,{},{},real);
			for(const auto &layer:backfill->layers)require(layer.started==MotionClock::time_point{} && !layer.previousValue,"Historical follow animated");
			real.latestRevision[1]=12;real.delivered[1]=1;actual[1]="LiveSubscriber";actual[5]="1";
			const auto live=prepareSupporterFrame(engine,options,labels,actual,backfill,now,{},{},real);
			for(std::size_t i=0;i<6;++i)require((live->layers[i].started!=MotionClock::time_point{})==(mode!=Motion::None && (i==1 || i==5)),"Live subscription did not animate only Sub/Daily Subs");
			// Delivery with identical text updates the baseline without restarting/rasterizing.
			real.latestRevision[1]=13;real.delivered[1]=2;const auto statsBefore=engine.stats();
			const auto repeated=prepareSupporterFrame(engine,options,labels,actual,live,now+std::chrono::seconds(1),{},{},real);
			require(repeated->context==std::optional<BarContext>(real) && repeated->layers[1].started==live->layers[1].started && engine.stats().renders==statsBefore.renders,"Identical data restarted motion or failed to update delivery baseline");
			actual[4]="99";const auto noEvent=prepareSupporterFrame(engine,options,labels,actual,repeated,now,{},{},real);
			require(noEvent->layers[4].started==MotionClock::time_point{},"Counter text alone was treated as a live Cheer");
			real.delivered[2]=1;actual[4]="100";const auto lateCheer=prepareSupporterFrame(engine,options,labels,actual,noEvent,now,{},{},real);
			require((lateCheer->layers[4].started!=MotionClock::time_point{})==(mode!=Motion::None) && lateCheer->layers[2].started==MotionClock::time_point{},"Late Cheer failed selective daily animation");
			auto test=real;test.preview=true;test.channel={events::EventOrigin::Preview,"private-test"};auto demo=values;
			const auto preview=prepareSupporterFrame(engine,options,labels,demo,lateCheer,now,{},{},test);
			require((preview->layers[0].started!=MotionClock::time_point{})==(mode!=Motion::None),"TEST no longer demonstrates value changes");
			const auto ended=prepareSupporterFrame(engine,options,labels,actual,preview,now,{},{},real);
			for(const auto &layer:ended->layers)require(layer.started==MotionClock::time_point{} && !layer.previousValue,"Ending TEST replayed real fields");
			real.generation=2;const auto restarted=prepareSupporterFrame(engine,options,labels,values,ended,now,{},{},real);
			for(const auto &layer:restarted->layers)require(layer.started==MotionClock::time_point{} && !layer.previousValue,"New generation replayed restored history");
		}
		// Compare cropped layers with the legacy value coordinates on one transparent
		// canvas. Qt blends glyph edges differently onto a painted background; the
		// animated text must be independently compositable premultiplied RGBA.
		QImage combined=initial->base->image.copy();
		{ QPainter painter(&combined); for(const auto &layer:initial->layers) if(layer.frame) painter.drawImage(layer.x,layer.y,layer.frame->image); }
		auto referenceScene=supporterScene(settings,labels,values);
		referenceScene.nodes.erase(std::remove_if(referenceScene.nodes.begin(),referenceScene.nodes.end(),[](const auto &node) {
			const auto *text=std::get_if<TextNode>(&node);
			return !text || text->box.y()!=36;
		}),referenceScene.nodes.end());
		const auto referenceValues=engine.prepare(referenceScene);
		QImage reference=initial->base->image.copy();
		{ QPainter painter(&reference); painter.drawImage(0,0,referenceValues->image); }
		int pixelDelta=0;
		for(int y=0;y<combined.height();++y) for(int x=0;x<combined.width()*4;++x)
			pixelDelta=std::max(pixelDelta,std::abs(int(combined.constScanLine(y)[x])-int(reference.constScanLine(y)[x])));
		std::cout<<"Layer composition maximum pixel delta: "<<pixelDelta<<'\n';
		require(pixelDelta<=1,"Layered output changed static layout beyond 8-bit composition rounding");
		auto stats=engine.stats();
		require(prepareSupporterFrame(engine,settings,labels,values,initial,now+std::chrono::seconds(3))==initial,"Unchanged values restarted motion");
		require(engine.stats().renders==stats.renders && engine.stats().hits==stats.hits,"Unchanged values touched renderer");
		values[4]="200";
		const auto changed=prepareSupporterFrame(engine,settings,labels,values,initial,now+std::chrono::milliseconds(100));
		require(engine.stats().renders==stats.renders+1 && changed->base==initial->base,"One value rebuilt background or unrelated text");
		for(size_t i=0;i<6;++i) if(i!=4) require(changed->layers[i].frame==initial->layers[i].frame && changed->layers[i].started==initial->layers[i].started,"Unrelated layer restarted");
		require(changed->layers[4].started==now+std::chrono::milliseconds(100),"Changed layer did not restart");
		const auto second=prepareSupporterFrame(engine,settings,labels,values,{},now);
		require(second->base==changed->base && second->layers[4].frame==changed->layers[4].frame,"Identical instances did not share textures");
		settings.fields[0]=false;
		const auto hidden=prepareSupporterFrame(engine,settings,labels,values,changed,now);
		stats=engine.stats(); values[0]="Ignored hidden change";
		require(prepareSupporterFrame(engine,settings,labels,values,hidden,now)==hidden && engine.stats().renders==stats.renders,"Disabled field caused work");
		settings.motion=Motion::None;
		const auto disabled=prepareSupporterFrame(engine,settings,labels,values,hidden,now);
		require(engine.stats().renders==stats.renders,"Motion preference rasterized text");
		for(const auto &layer:disabled->layers) require(layer.started==MotionClock::time_point{},"Disabled animation retained timeline");
		settings.fields.fill(false);
		const auto empty=prepareSupporterFrame(engine,settings,labels,values,disabled,now);
		require(!empty->base,"All hidden fields retained a background");
		for(const auto &layer:empty->layers) require(!layer.frame,"All hidden fields retained values");
		settings.fields.fill(true);settings.motion=Motion::StyleChange;settings.durationMs=2000;settings.style.cards=true;
		const auto resting=prepareSupporterFrame(engine,settings,labels,values,{},now);
		for(const auto &layer:resting->layers)require(layer.started==MotionClock::time_point{} && !layer.previousValue,"Loading/restoring replayed an event");
		stats=engine.stats();values[0]="CarlosDJ";
		const auto eventTime=now+std::chrono::seconds(5);
		const auto event=prepareSupporterFrame(engine,settings,labels,values,resting,eventTime);
		require(event->base==resting->base && engine.stats().renders==stats.renders+1,"Style animation rebuilt unrelated layers");
		require(event->layers[0].previousValue==resting->layers[0].frame && event->layers[0].started==eventTime,"Outgoing value was not retained");
		for(size_t i=1;i<6;++i)require(event->layers[i].frame==resting->layers[i].frame && event->layers[i].started==MotionClock::time_point{},"Another card animated");
		const auto entrance=sampleChange(180,2000),main=sampleChange(600,2000),residual=sampleChange(1300,2000),fade=sampleChange(1850,2000),done=sampleChange(2000,2000);
		require(sampleChange(0,2000).oldOpacity==1 && sampleChange(0,2000).newOpacity==0 && sampleChange(0,2000).glow==0,"Change has no resting start");
		require(entrance.glow>0 && entrance.oldOpacity==1 && entrance.particles>0,"Entrance does not retain old text and alert");
		require(main.sweep>0 && main.oldOpacity==0 && main.newOpacity>0,"Main transition is missing or overlays two names");
		require(residual.newOpacity==1 && residual.oldOpacity==0 && residual.glow>0 && residual.sweep==0 && residual.particles==0,"Updated state has no residual glow");
		require(fade.glow<residual.glow && done.finished && done.glow==0 && done.newOpacity==1 && done.newOffsetY==0,"Fade did not settle exactly");
		stats=engine.stats();
		for(int ms=0;ms<5000;++ms) {
			const auto sample=sampleChange(ms,2000);
			require(sample.glow>=0 && sample.glow<=1 && sample.newOpacity>=0 && sample.newOpacity<=1 && sample.oldOpacity>=0 && sample.oldOpacity+sample.newOpacity<=1.00001f,"Unbounded timeline");
			require(prepareSupporterFrame(engine,settings,labels,values,event,eventTime+std::chrono::milliseconds(ms))==event,"Elapsed time rebuilt a frame or restarted an event");
		}
		require(engine.stats().renders==stats.renders && engine.stats().hits==stats.hits,"Animation sampling touched the CPU renderer");
		values[0]="Newest";
		const auto rapid=prepareSupporterFrame(engine,settings,labels,values,event,eventTime+std::chrono::milliseconds(50));
		require(rapid->layers[0].previousValue==event->layers[0].frame && rapid->layers[0].previousValue!=resting->layers[0].frame,"Rapid changes accumulated outgoing values");
		for(auto &value:values)value+="!";
		const auto simultaneous=prepareSupporterFrame(engine,settings,labels,values,rapid,eventTime+std::chrono::milliseconds(100));
		for(size_t i=0;i<6;++i)require(simultaneous->layers[i].previousValue==rapid->layers[i].frame,"Simultaneous changes not independent");
		settings.motion=Motion::None;stats=engine.stats();
		const auto stopped=prepareSupporterFrame(engine,settings,labels,values,simultaneous,eventTime);
		for(const auto &layer:stopped->layers)require(!layer.previousValue && layer.started==MotionClock::time_point{},"Disable retained an effect");
		require(engine.stats().renders==stats.renders,"Disable rebuilt text");
		require(sampleChange(-1,2000).progress==0 && sampleChange(0,0).finished && sampleChange(INFINITY,2000).finished,"Invalid/negative time unsafe");
		settings.motion=Motion::None;values={"DJ","Two","Three","Four","100","6"};
		const auto shortNames=prepareSupporterFrame(engine,settings,labels,values,{},now);
		require(!shortNames->hasScrolling && shortNames->layers[0].scrollDistance==0,"Fitting name started scrolling");
		values[0]=QString::fromUtf8(u8"José_日本語_Community_With_A_Very_Long_Name");
		const auto longName=prepareSupporterFrame(engine,settings,labels,values,shortNames,now);
		require(longName->hasScrolling && longName->layers[0].scrollDistance>0 && !longName->layers[0].frame->textMetrics.front().elided,"Long name still ellipsized or not scrolling");
		for(size_t i=1;i<6;++i)require(longName->layers[i].frame==shortNames->layers[i].frame && longName->layers[i].scrollDistance==0,"Long name affected another field");
		const double distance=longName->layers[0].scrollDistance,travel=std::max(800.0,distance/30.0*1000.0),cycle=2400+travel*2;
		require(samplePingPong(0,distance)==0 && samplePingPong(1100,distance)==0 && samplePingPong(1200+travel,distance)==distance,"Start/end pause missing");
		require(samplePingPong(2200+travel,distance)==distance && samplePingPong(cycle,distance)==0,"Return pause/cycle missing");
		require(std::abs(samplePingPong(1200+travel/2,distance)-distance/2)<.01 && std::abs(samplePingPong(2400+travel*1.5,distance)-distance/2)<.01,"Ping-pong is not symmetric");
		stats=engine.stats();for(int ms=0;ms<60000;ms+=7){const auto offset=samplePingPong(ms,distance);require(offset>=0 && offset<=distance,"Ping-pong escaped viewport");require(prepareSupporterFrame(engine,settings,labels,values,longName,now+std::chrono::milliseconds(ms))==longName,"Scrolling rebuilt a frame");}
		require(engine.stats().renders==stats.renders && engine.stats().hits==stats.hits,"Scrolling caused CPU work per frame");
		settings.valueFamily="Consolas";const auto custom=prepareSupporterFrame(engine,settings,labels,values,longName,now);
		require(custom->base==longName->base && custom->layers[0].frame!=longName->layers[0].frame && custom->layers[4].frame==longName->layers[4].frame,"Font override affected layout/counters or was ignored");
		for(const auto &layer:custom->layers)require(!layer.previousValue && layer.started==MotionClock::time_point{},"Font change replayed alert");
		settings.valueFamily.clear();const auto inherited=prepareSupporterFrame(engine,settings,labels,values,custom,now);
		require(inherited->layers[0].frame==longName->layers[0].frame,"Returning to preset font failed to reuse strip");
		settings.labelFamily="Consolas";
		stats=engine.stats();const auto customLabels=prepareSupporterFrame(engine,settings,labels,values,inherited,now);
		require(customLabels->base!=inherited->base && engine.stats().renders==stats.renders+1,"Label choice ignored or rebuilt more than base");
		for(size_t i=0;i<6;++i)require(customLabels->layers[i].frame==inherited->layers[i].frame && !customLabels->layers[i].previousValue && customLabels->layers[i].started==MotionClock::time_point{},"Label font changed name/counter or replayed event");
		settings.valueFamily="Segoe Print";const auto bothFonts=prepareSupporterFrame(engine,settings,labels,values,customLabels,now);
		require(bothFonts->base==customLabels->base && bothFonts->layers[0].frame!=customLabels->layers[0].frame && bothFonts->layers[4].frame==customLabels->layers[4].frame,"Name choice changed label or counter");
		settings.labelFamily.clear();settings.valueFamily.clear();const auto styleAgain=prepareSupporterFrame(engine,settings,labels,values,bothFonts,now);
		require(styleAgain->base==inherited->base && styleAgain->layers[0].frame==inherited->layers[0].frame,"Independent font inheritance failed to restore cached design");
		auto labelSizeSettings=settings;labelSizeSettings.height=160;
		const auto originalLabels=prepareSupporterFrame(engine,labelSizeSettings,labels,values,{},now);
		labelSizeSettings.labelPixelSize=8;
		const auto smallLabels=prepareSupporterFrame(engine,labelSizeSettings,labels,values,originalLabels,now);
		require(smallLabels->base->textMetrics.front().pixelSize==8 && smallLabels->layers[0].frame==originalLabels->layers[0].frame,"Small label size ignored or rebuilt unchanged value geometry");
		labelSizeSettings.labelPixelSize=24;
		const auto largeLabels=prepareSupporterFrame(engine,labelSizeSettings,labels,values,smallLabels,now);
		require(largeLabels->base->textMetrics.front().pixelSize>smallLabels->base->textMetrics.front().pixelSize && largeLabels->base->textMetrics.front().height>smallLabels->base->textMetrics.front().height,"Changing only label size did not grow the glyphs");
		require(largeLabels->layers[0].y>smallLabels->layers[0].y,"Larger labels did not reserve space above values");
		for(size_t i=0;i<6;++i){
			require(!largeLabels->base->textMetrics[i].overflow,"Custom label clipped vertically");
			require(largeLabels->layers[i].y+largeLabels->layers[i].frame->image.height()<=largeLabels->layers[i].card.bottom()+1,"Label size pushed values outside their cards");
			require(!largeLabels->layers[i].previousValue && largeLabels->layers[i].started==MotionClock::time_point{},"Label size replayed an event animation");
		}
		stats=engine.stats();
		require(prepareSupporterFrame(engine,labelSizeSettings,labels,values,largeLabels,now+std::chrono::seconds(3))==largeLabels && engine.stats().renders==stats.renders && engine.stats().hits==stats.hits,"Custom label size rebuilt at idle");
		labelSizeSettings.labelPixelSize=0;
		const auto restoredLabels=prepareSupporterFrame(engine,labelSizeSettings,labels,values,largeLabels,now);
		require(restoredLabels->base==originalLabels->base && restoredLabels->layers[0].frame==originalLabels->layers[0].frame,"Style inheritance did not restore original size and cached layout");
		// Read the same native OBS font objects as the source, including point sizes.
		auto *fontData=obs_data_create();fonts::defaults(fontData,"Arial");
		obs_data_set_bool(fontData,"use_style_font",false);obs_data_set_bool(fontData,"use_style_label_font",false);
		auto *fontChoice=obs_data_create();obs_data_set_string(fontChoice,"face","Arial");
		const auto registry=presets::Registry::fromFile(QString::fromUtf8(OTO_PRESET_FILE));
		const std::array<QString,6> shortValues{"DJ","DJ","DJ","DJ","100","6"};
		for(const auto &preset:registry.entries())for(const int height:{80,96,160,240}){
			BarSettings typography;typography.style=preset.style;typography.height=height;typography.motion=Motion::StyleChange;
			const auto original=prepareSupporterFrame(engine,typography,labels,shortValues,{},now);
			int lastSize=0;
			BarFramePtr previous=original;
			for(const int points:{6,12,18,24,36,96}){
				obs_data_set_int(fontChoice,"size",points);obs_data_set_obj(fontData,"name_font",fontChoice);
				// Keep label geometry identical while measuring changes to names alone.
				obs_data_set_bool(fontData,"use_style_label_font",true);readSupporterTypography(typography,fontData);
				const auto selected=prepareSupporterFrame(engine,typography,labels,shortValues,previous,now);
				const auto &metric=selected->layers[0].frame->textMetrics.front();
				require(metric.pixelSize>=lastSize && metric.pixelSize<=typography.valuePixelSize,"Name size ignored or not monotonic");
				require(metric.height<=selected->layers[0].frame->image.height(),"Custom name size clipped vertically");
				if(points==6)require(metric.pixelSize==8,"Small native font size ignored");
				if(height==240 && points==24)require(metric.pixelSize==32 && metric.pixelSize>lastSize,"Larger native font size ignored");
				for(size_t i=4;i<6;++i)require(selected->layers[i].frame==original->layers[i].frame,"Name size changed daily counters");
				for(const auto &layer:selected->layers)require(layer.started==MotionClock::time_point{} && !layer.previousValue,"Font size replayed an event");
				stats=engine.stats();require(prepareSupporterFrame(engine,typography,labels,shortValues,selected,now+std::chrono::seconds(2))==selected && engine.stats().renders==stats.renders,"Font size rebuilt at idle");
				lastSize=metric.pixelSize;previous=selected;
			}
			obs_data_set_bool(fontData,"use_style_font",true);obs_data_set_bool(fontData,"use_style_label_font",false);
			int lastLabelSize=0;
			for(const int points:{6,12,18,24,36,96}){
				obs_data_set_int(fontChoice,"size",points);obs_data_set_obj(fontData,"label_font",fontChoice);readSupporterTypography(typography,fontData);
				const auto selected=prepareSupporterFrame(engine,typography,labels,shortValues,previous,now);
				const auto layout=supporterLabelLayout(typography);
				const auto &metric=selected->base->textMetrics.front();
				require(metric.pixelSize>=lastLabelSize && metric.height<=layout.height-supporterLabelTopInset,"Label size clipped or stopped responding");
				if(height==240 && points==24)require(metric.pixelSize==32 && metric.pixelSize>lastLabelSize,"Large label size ignored");
				for(const auto &layer:selected->layers)require(layer.y+layer.frame->image.height()<=layer.card.bottom()+1 && !layer.previousValue && layer.started==MotionClock::time_point{},"Label size escaped card or replayed animation");
				lastLabelSize=metric.pixelSize;previous=selected;
			}
			obs_data_set_bool(fontData,"use_style_label_font",true);readSupporterTypography(typography,fontData);
			const auto restored=prepareSupporterFrame(engine,typography,labels,shortValues,previous,now);
			require(restored->base==original->base && restored->layers[0].frame==original->layers[0].frame,"Native style checkbox did not restore the original font sizes");
			obs_data_set_bool(fontData,"use_style_font",false);
		}
		obs_data_release(fontChoice);obs_data_release(fontData);
		const auto marginScene=supporterScene(settings,labels,shortValues);
		const auto &marginTitle=std::get<TextNode>(marginScene.nodes[1]);
		require(marginTitle.box.y()==14 && marginTitle.vertical==Vertical::Top,"Title top inset missing or vertical centering changed");
		std::cout<<"PASS: native point sizes, names/labels, ten styles/four heights/six sizes, quiet cache restoration and title top inset\n";
		for(const auto mode:{Motion::Fade,Motion::SlideFade,Motion::StyleChange}) {
			auto fontSettings=settings;fontSettings.motion=mode;
			const auto loaded=prepareSupporterFrame(engine,fontSettings,labels,values,{},now);
			fontSettings.valueFamily="Consolas";fontSettings.labelFamily="Segoe Print";
			const auto chosen=prepareSupporterFrame(engine,fontSettings,labels,values,loaded,now);
			for(const auto &layer:chosen->layers)require(!layer.previousValue && layer.started==MotionClock::time_point{},"Font preference replayed a fade or slide");
		}
		values[0]="DJ";const auto fitting=prepareSupporterFrame(engine,settings,labels,values,inherited,now);require(!fitting->hasScrolling,"Short replacement retained scrolling");
		require(samplePingPong(INFINITY,distance)==0 && samplePingPong(0,0)==0 && samplePingPong(-100,distance)==0,"Invalid ping-pong sample unsafe");
		std::cout<<"PASS: overflow-only ping-pong, endpoint pauses, full Unicode strips, cached/no CPU per frame, selective font override and cleanup\n";
		std::cout<<"PASS: five-stage change, one card only, initial restore quiet, rapid/simultaneous changes bounded, no CPU work per animation frame\n";
		std::cout<<"PASS: finite motion, disabled mode, layered pixel identity, selective one-value update, unchanged/hidden skip, shared frames, independent timelines and cleanup\n";
		return 0;
	} catch(const std::exception &error) { std::cerr<<error.what()<<'\n'; return 1; }
}
