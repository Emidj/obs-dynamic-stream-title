// SPDX-License-Identifier: GPL-2.0-or-later
// Phase 13: same immutable models and draw routines as the six OBS sources.
// Synthetic, private fixtures. This deliberately allows Alert+SO overlap as an
// upper bound; production serializes them through PresentationQueue.
#include "adapters/supporter-draw.hpp"
#include "adapters/event-list-draw.hpp"
#include "adapters/alert-draw.hpp"
#include "adapters/shoutout-draw.hpp"
#include "adapters/hype-draw.hpp"
#include "adapters/chat-draw.hpp"
#include "core/plugin-core.hpp"
#include "visual/asset-manager.hpp"
#include "chat-fixtures.hpp"
#include <QApplication>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QFile>
#include <QDir>
#include <QBuffer>
#include <obs.h>
#include <windows.h>
#include <psapi.h>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <thread>
using namespace oto;
namespace {
using Clock=std::chrono::steady_clock;
double ms(Clock::duration d){return std::chrono::duration<double,std::milli>(d).count();}
void require(bool ok,const char *why){if(!ok)throw std::runtime_error(why);}
struct Config:IConfigStore{ConfigLoadResult load()override{return {ConfigStatus::Loaded,{false}};}bool save(Settings)override{return true;}};
struct Log:ILogger{void write(LogLevel,std::string_view)const noexcept override{}};
QJsonObject distribution(std::vector<double> v){if(v.empty())return {};std::sort(v.begin(),v.end());double total=0;for(auto x:v)total+=x;return {{"count",int(v.size())},{"mean_ms",total/v.size()},{"p50_ms",v[v.size()/2]},{"p95_ms",v[std::min(v.size()-1,size_t(v.size()*.95))]},{"p99_ms",v[std::min(v.size()-1,size_t(v.size()*.99))]},{"max_ms",v.back()}};}
QJsonObject memory(){PROCESS_MEMORY_COUNTERS_EX p{};p.cb=sizeof(p);require(GetProcessMemoryInfo(GetCurrentProcess(),reinterpret_cast<PROCESS_MEMORY_COUNTERS *>(&p),sizeof(p)),"Memory sample");return {{"working_mib",double(p.WorkingSetSize)/1048576},{"private_mib",double(p.PrivateUsage)/1048576}};}
double cpuSeconds(){FILETIME created,exited,kernel,user;require(GetProcessTimes(GetCurrentProcess(),&created,&exited,&kernel,&user),"CPU sample");ULARGE_INTEGER k{},u{};k.LowPart=kernel.dwLowDateTime;k.HighPart=kernel.dwHighDateTime;u.LowPart=user.dwLowDateTime;u.HighPart=user.dwHighDateTime;return double(k.QuadPart+u.QuadPart)/1e7;}
struct Workload {
 PluginCore core{std::make_unique<Config>(),std::make_unique<Log>()};
 std::shared_ptr<visual::VisualEngine> engine=std::make_shared<visual::VisualEngine>();
 chat::EmoteCache media{engine};visual::AssetManager assets{QFileInfo(OTO_PRESET_FILE).dir().filePath("assets")};
 visual::ImagePtr portrait;state::ChannelKey listKey{events::EventOrigin::Preview,""},chatKey=listKey;
 sources::BarFramePtr bar;sources::ListFramePtr list;sources::AlertFramePtr alert;sources::ShoutoutFramePtr so;sources::HypeFramePtr hype;sources::ChatFramePtr chat;
 sources::ChatSettings cs;sources::ListSettings ls;sources::ListLabels ll{"Anonymous","Bits","Subs","Viewers","Sub x1","Months"};
 visual::ImagePtr chatArt,listArt;visual::MotionClock::time_point start=Clock::time_point(std::chrono::seconds(100));uint64_t sequence=0,received=0;
 Workload(){core.start();media.setCatalogActive(false);require(media.ingest("https://static-cdn.jtvnw.net/emoticons/v2/fixture/animated/dark/2.0",animatedChatFixture()),"Animated fixture");QImage p(256,256,QImage::Format_RGBA8888_Premultiplied);p.fill(QColor(65,160,230));portrait=std::make_shared<const visual::ImageData>(visual::ImageData{p});}
 visual::ImagePtr image(const QString &ref){return ref.isEmpty()?visual::ImagePtr{}:assets.load(ref);}
 visual::ImagePtr region(const QString &ref,QRect crop){return ref.isEmpty()?visual::ImagePtr{}:assets.loadRegion(ref,crop);}
 void message(int emotes,Clock::time_point now){events::ChatEvent m{{"author"+std::to_string(sequence%32),"Streamer_with_a_long_name_"+std::to_string(sequence%32)},"!so @fixture ",false,"#FF44BB"};m.fragments.push_back({m.message,{},false});for(int i=0;i<emotes;++i){m.message+="Dance ";m.fragments.push_back({"Dance","fixture",true});m.fragments.push_back({" ",{},false});}require(bool(core.processEvent({"chat-"+std::to_string(sequence++),chatKey.channelId,std::chrono::system_clock::now(),chatKey.origin,m})),"Chat admission");received+=core.processPendingEventsFor(chatKey.origin,chatKey.channelId).processed;chat=sources::prepareChatFrame(*engine,cs,sources::chatView(core.sharedSnapshot(),chatKey,cs),chat,now,false,&media,chatArt);}
 void build(const presets::Definition &d,bool maximum){
  bar.reset();list.reset();alert.reset();so.reset();hype.reset();chat.reset();core.stop();core.start();engine->clear();assets.clear();
  const auto listId=core.createSourcePreview(true,true);listKey.channelId=listId;
  sources::BarSettings bs;bs.style=d.style;bs.presetId=d.id;bs.motion=visual::Motion::StyleChange;bs.durationMs=2500;if(maximum)bs.width=1920;
  sources::BarLabels bl{{"LAST FOLLOW","LAST SUB","LAST CHEER","LAST GIFT","DAILY BITS","DAILY SUBS"},"None","Anonymous"};
  bar=sources::prepareSupporterFrame(*engine,bs,bl,{"Long_streamer_name_123456","TechnoAlex","Pepito · 500","Carlos · 10","4,850","27"},{},start,region(d.style.backgroundImage,d.style.backgroundCrop),region(d.style.iconImage,d.style.iconCrop));
  ls={};ls.presetId=d.id;ls.style=*d.eventListStyle;ls.family=ls.style.family;if(maximum)ls.width=1200;
  listArt=region(ls.style.backgroundImage,ls.style.backgroundCrop);list=sources::prepareEventListFrame(*engine,ls,ll,sources::eventListView(core.sharedSnapshot(),ls,ll,listKey),{},start,false,listArt);
  const auto a=core.createAlertPreview(events::EventType::Cheer,false,true);const auto &ae=core.sharedSnapshot()->channels.at({events::EventOrigin::Preview,a})->activity.front();sources::AlertSettings as;as.presetId=d.id;
  alert=sources::prepareAlertFrame(*engine,as,*d.alertStyle,ae,image(d.alertStyle->atlas),portrait,start,image(d.alertStyle->iconAnimations[5]));
  const auto s=core.createShoutoutPreview(true,{"so","A_very_long_shoutout_streamer_name"},"fixture","Music",125000);const auto &se=core.sharedSnapshot()->channels.at({events::EventOrigin::Preview,s})->shoutouts.front();sources::ShoutoutSettings ss;ss.preset=d.id;
  so=sources::prepareShoutoutFrame(*engine,ss,*d.shoutoutStyle,se,portrait,start,image(d.shoutoutStyle->atlas));
  sources::HypeSettings hs;hs.presetId=d.id;hs.style=*d.hypeStyle;hs.accent=hs.style.accent;hs.numberFamily=hs.labelFamily=hs.style.family;if(maximum){hs.width=1920;hs.height=400;}
  sources::HypeView hv;hv.visible=true;hv.level=3;hv.progress=3900;hv.goal=5000;hv.ratio=.78;hv.remaining=84;hv.trainId="fixture";hv.contributors={{{"a","TechnoClub"},1500,3,500,1000,2},{{"b","MarinaDJ"},1200,1,1200,0,{}},{{"c","Long_contributor_12345678"},750,3,250,500,1}};
  hype=sources::prepareHypeFrame(*engine,hs,{},hv,{},start,true,region(hs.style.atlas,hs.style.atlasCrop),region(hs.style.thanksAtlas,hs.style.thanksCrop));
  chatKey.channelId=core.createChatPreview();cs={};cs.presetId=d.id;cs.style=*d.chatStyle;cs.nameFamily=cs.messageFamily=cs.style.family;cs.messages=12;if(maximum){cs.width=1920;cs.height=1080;}chatArt=region(cs.style.atlas,cs.style.crop);
  for(int i=0;i<12;++i)message(maximum?60:8,start);
 }
 void update(int frame,bool maximum){const auto now=start+std::chrono::milliseconds(frame*1000/60);message(maximum?60:8,now);if(frame%120==0){require(bool(core.processEvent({"follow-"+std::to_string(sequence++),listKey.channelId,std::chrono::system_clock::now(),listKey.origin,events::FollowEvent{{"f","New_follower_"+std::to_string(sequence)}}})),"Follow admission");received+=core.processPendingEventsFor(listKey.origin,listKey.channelId).processed;list=sources::prepareEventListFrame(*engine,ls,ll,sources::eventListView(core.sharedSnapshot(),ls,ll,listKey),list,now,false,listArt);} }
 void draw(ObsVisualRenderer &r,int n,bool dynamic=false){const auto t=start+std::chrono::milliseconds(n*1000/60%5900),live=dynamic?start+std::chrono::milliseconds(n*1000/60):t;const auto layer=[&](float x,float y,float scale,auto draw){gs_matrix_push();gs_matrix_translate3f(x,y,0);gs_matrix_scale3f(scale,scale,1);draw();gs_matrix_pop();};
  layer(0,980,.9f,[&]{drawSupporterFrame(r,*bar,live);});layer(0,0,.65f,[&]{drawEventListFrame(r,*list,live);});layer(490,0,.65f,[&]{drawAlertFrame(r,*alert,t);});layer(1050,20,.8f,[&]{drawShoutoutFrame(r,*so,t);});layer(490,680,.9f,[&]{drawHypeFrame(r,*hype,t);});layer(1300,350,.55f,[&]{drawChatFrame(r,*chat,start+std::chrono::milliseconds(n*1000/60));});
 }
};
struct Queries {gs_timer_t *timer=gs_timer_create();gs_timer_range_t *range=gs_timer_range_create();bool pending=false,measured=false;~Queries(){if(timer)gs_timer_destroy(timer);if(range)gs_timer_range_destroy(range);} };
QJsonObject run(Workload &w,ObsVisualRenderer &r,int frames,bool maximum,bool dynamic,bool paced,const QString &samples){
 std::ofstream csv(samples.toStdString());csv<<"frame,cpu_draw_ms,gpu_ms_latest_retired,update_ms,working_mib,private_mib,uploads,evictions\n";
 std::vector<double> cpu,gpu,update,waits;QJsonArray mem;const auto began=Clock::now();const double cpu0=cpuSeconds();
 obs_enter_graphics();auto *target=gs_texrender_create(GS_RGBA,GS_ZS_NONE);std::array<Queries,16> query;
 const auto collect=[&](Queries &q){if(!q.pending)return;bool disjoint=false;uint64_t frequency=0,ticks=0;const auto t=Clock::now();require(gs_timer_range_get_data(q.range,&disjoint,&frequency) && gs_timer_get_data(q.timer,&ticks),"GPU timestamp query");if(q.measured){require(!disjoint && frequency>0,"Disjoint GPU timestamp");gpu.push_back(double(ticks)*1000/frequency);waits.push_back(ms(Clock::now()-t));}q.pending=false;};
 obs_leave_graphics();
 for(int n=-120;n<frames;++n){double prep=0;if(dynamic && n>=0 && n%12==0){const auto u=Clock::now();w.update(n+120,maximum);prep=ms(Clock::now()-u);update.push_back(prep);}obs_enter_graphics();auto &q=query[size_t(n+120)%query.size()];collect(q);gs_texrender_reset(target);require(gs_texrender_begin(target,1920,1080),"Target begin");vec4 zero;vec4_zero(&zero);gs_clear(GS_CLEAR_COLOR,&zero,0,0);gs_ortho(0,1920,0,1080,-100,100);gs_timer_range_begin(q.range);gs_timer_begin(q.timer);const auto t=Clock::now();w.draw(r,n+120,dynamic);const auto duration=ms(Clock::now()-t);gs_timer_end(q.timer);gs_timer_range_end(q.range);gs_texrender_end(target);q.pending=true;q.measured=n>=0;gs_flush();obs_leave_graphics();
  if(n>=0){cpu.push_back(duration);if(n%60==0){auto m=memory();m["frame"]=n;m["elapsed_s"]=ms(Clock::now()-began)/1000;mem.append(m);const auto s=r.stats();csv<<n<<','<<duration<<','<<(gpu.empty()?0:gpu.back())<<','<<prep<<','<<m["working_mib"].toDouble()<<','<<m["private_mib"].toDouble()<<','<<s.uploads<<','<<s.evictions<<'\n';}}
  if(n>=0 && n%600==0)csv.flush();
  if(paced && n>=0)std::this_thread::sleep_until(began+std::chrono::microseconds((n+1)*1000000LL/60));
 }
 obs_enter_graphics();for(auto &q:query)collect(q);
 if(!dynamic){const auto uploads=r.stats().uploads,rasters=w.engine->stats().renders;for(int n=600;n<660;++n){gs_texrender_reset(target);require(gs_texrender_begin(target,1920,1080),"Cache guard target");gs_ortho(0,1920,0,1080,-100,100);w.draw(r,n);gs_texrender_end(target);}require(r.stats().uploads==uploads && w.engine->stats().renders==rasters,"Hot combined scene raster/upload churn");std::cout<<"PASS combined warm cache, no raster/upload per frame\n";}
 const auto hidden=r.stats();r.draw(w.bar->base,false);require(r.stats().draws==hidden.draws && r.stats().uploads==hidden.uploads,"Hidden source does work");require(r.stats().bytes<=ObsVisualRenderer::textureBudgetBytes,"GPU budget exceeded");gs_texrender_destroy(target);
 const auto s=r.stats();const auto v=w.engine->stats();const auto a=w.assets.stats();const auto wall=ms(Clock::now()-began)/1000;auto out=QJsonObject{{"wall_s",wall},{"cpu_seconds",cpuSeconds()-cpu0},{"cpu_draw",distribution(cpu)},{"gpu_draw",distribution(gpu)},{"update",distribution(update)},{"query_wait",distribution(waits)},{"memory",mem},{"gpu_cache_mib",double(s.bytes)/1048576},{"uploads",double(s.uploads)},{"evictions",double(s.evictions)},{"draws",double(s.draws)},{"rasters",double(v.renders)},{"visual_cache_mib",double(v.bytes)/1048576},{"asset_cache_mib",double(a.bytes)/1048576},{"received_updates",double(w.received)}};
 // Timer destructors need a context. Scope them explicitly via reset below.
 for(auto &q:query){gs_timer_destroy(q.timer);gs_timer_range_destroy(q.range);q.timer=nullptr;q.range=nullptr;}obs_leave_graphics();return out;
}
}
int main(int argc,char **argv){QApplication app(argc,argv);try{require(argc>=5,"Usage: performance-probe graphics-module obs-data output frames [soak|cycles]");const QString out=QString::fromUtf8(argv[3]);QDir().mkpath(out);const int frames=std::max(120,std::atoi(argv[4]));const bool soak=argc>5 && std::string(argv[5])=="soak",cycles=argc>5 && std::string(argv[5])=="cycles";require(obs_startup("en-US",nullptr,nullptr),"OBS startup");
#pragma warning(push)
#pragma warning(disable:4996)
 obs_add_data_path(argv[2]);
#pragma warning(pop)
 obs_video_info v{};v.graphics_module=argv[1];v.fps_num=60;v.fps_den=1;v.base_width=v.output_width=1920;v.base_height=v.output_height=1080;v.output_format=VIDEO_FORMAT_RGBA;v.colorspace=VIDEO_CS_DEFAULT;v.range=VIDEO_RANGE_FULL;v.scale_type=OBS_SCALE_DISABLE;require(obs_reset_video(&v)==OBS_VIDEO_SUCCESS,"D3D11 startup");
 QJsonArray results;{Workload w;ObsVisualRenderer r;const auto registry=presets::Registry::fromFile(OTO_PRESET_FILE);for(int round=0;round<(cycles?5:1);++round)for(const auto &d:registry.entries()){if(soak && d.id!="neon-dj")continue;for(bool maximum:{false,true}){if(soak && !maximum)continue;if(!cycles)r.clear();require(r.stats().bytes<=ObsVisualRenderer::textureBudgetBytes,"Cycle GPU budget exceeded");const auto t=Clock::now();w.build(d,maximum);const double buildMs=ms(Clock::now()-t);const auto result=run(w,r,frames,maximum,soak,soak,out+"/"+d.id+(maximum?"-maximum":"-normal")+QString("-%1.csv").arg(round));auto entry=result;entry["style"]=d.id;entry["maximum"]=maximum;entry["round"]=round;entry["build_ms"]=buildMs;results.append(entry);QFile file(out+"/results.json");require(file.open(QIODevice::WriteOnly),"Result file");file.write(QJsonDocument(results).toJson());std::cout<<"MEASURE "<<d.id.toStdString()<<" maximum="<<maximum<<" cpu_p95="<<result["cpu_draw"].toObject()["p95_ms"].toDouble()<<" gpu_p95="<<result["gpu_draw"].toObject()["p95_ms"].toDouble()<<" uploads="<<result["uploads"].toDouble()<<std::endl;}}
 r.clear();require(r.stats().bytes==0 && r.stats().entries==0,"Final GPU release failed");w.core.stop();}obs_shutdown();return 0;}catch(const std::exception &e){std::cerr<<"FAILED "<<e.what()<<std::endl;return 1;}}
