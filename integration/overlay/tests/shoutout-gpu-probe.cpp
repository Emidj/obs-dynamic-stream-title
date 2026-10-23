// SPDX-License-Identifier: GPL-2.0-or-later
#include "adapters/shoutout-draw.hpp"
#include "visual/asset-manager.hpp"
#include "core/plugin-core.hpp"
#include <QApplication>
#include <QDir>
#include <QFileInfo>
#include <QPainter>
#include <obs.h>
#include <iostream>
using namespace oto;
namespace {
void check(bool ok,const char *why){if(!ok)throw std::runtime_error(why);}
int delta(const QImage &a,const QImage &b){int d=0;for(int y=0;y<a.height();++y)for(int x=0;x<a.width()*4;++x)d=std::max(d,std::abs(int(a.constScanLine(y)[x])-int(b.constScanLine(y)[x])));return d;}
void guard(const QImage &image,QRect box){for(int y=0;y<800;++y)for(int x=0;x<800;++x)if(!box.contains(x,y))check(image.constScanLine(y)[x*4+3]==0,"Pixels exceed artwork bounds");}
struct Config:IConfigStore{ConfigLoadResult load()override{return {ConfigStatus::Loaded,{false}};}bool save(Settings)override{return true;}};
struct Log:ILogger{void write(LogLevel,std::string_view)const noexcept override{}};
}
int main(int argc,char **argv){QApplication app(argc,argv);try{
 check(argc==5,"usage: graphics-module obs-data registry output");check(obs_startup("en-US",nullptr,nullptr),"OBS startup");
#pragma warning(push)
#pragma warning(disable:4996)
 obs_add_data_path(argv[2]);
#pragma warning(pop)
 obs_video_info v{};v.graphics_module=argv[1];v.fps_num=60;v.fps_den=1;v.base_width=v.output_width=800;v.base_height=v.output_height=800;v.output_format=VIDEO_FORMAT_RGBA;v.colorspace=VIDEO_CS_DEFAULT;v.range=VIDEO_RANGE_FULL;v.scale_type=OBS_SCALE_DISABLE;check(obs_reset_video(&v)==OBS_VIDEO_SUCCESS,"D3D11 startup");
 {
 const auto registry=presets::Registry::fromFile(QString::fromUtf8(argv[3]));
 visual::AssetManager assets(QFileInfo(argv[3]).dir().filePath("assets"));visual::VisualEngine engine;ObsVisualRenderer renderer;
 PluginCore core(std::make_unique<Config>(),std::make_unique<Log>());core.start();
 const auto key=core.createShoutoutPreview(false,{"fixture","DJ_PEPITO"},"dj_pepito","Just Chatting",125000);
 const auto entry=core.sharedSnapshot()->channels.at({events::EventOrigin::Preview,key})->shoutouts.back();
 auto photo=std::make_shared<visual::ImageData>();photo->image=QImage(256,256,QImage::Format_RGBA8888_Premultiplied);
 photo->image.fill(QColor(35,55,90));{QPainter painter(&photo->image);painter.setRenderHint(QPainter::Antialiasing);painter.setPen(Qt::NoPen);painter.setBrush(QColor(130,155,195));painter.drawEllipse(85,44,86,86);painter.drawRoundedRect(47,140,162,105,60,60);}
 const auto origin=visual::MotionClock::now();const sources::AlertTimes times{2000,6000,700};
 QImage blank(800,800,QImage::Format_RGBA8888_Premultiplied);blank.fill(Qt::transparent);
 for(const auto &definition:registry.entries()){
  check(bool(definition.shoutoutStyle),"Missing Shoutout style");const auto style=*definition.shoutoutStyle;check(style.motionPattern>=0 && style.motionPattern<=9,"Native preset motion missing");
  const QString out=QString::fromUtf8(argv[4])+"/"+definition.id;check(QDir().mkpath(out+"/frames"),"Output directories");
  sources::ShoutoutSettings settings;settings.preset=definition.id;settings.times=times;
  const auto frame=sources::prepareShoutoutFrame(engine,settings,style,entry,photo,origin,assets.load(style.atlas));
  auto render=[&](double ms){return renderer.renderOffscreen({800,800},[&]{drawShoutoutFrame(renderer,*frame,origin+std::chrono::duration_cast<visual::MotionClock::duration>(std::chrono::duration<double,std::milli>(ms)));});};
  auto art=[&](double ms){return renderer.renderOffscreen({800,800},[&]{drawNativeShoutoutArtwork(renderer,frame->base,style,ms,times);});};
  check(delta(render(0),blank)==0 && delta(render(times.total()),blank)==0,"Transparent endpoints");
  auto a=art(300),b=art(900),c=art(1550),d=art(2100);check(delta(a,b)>40 && delta(b,c)>40,"Native stages missing");
  check(delta(d,renderer.renderOffscreen(frame->base))<=6,"Settled artwork altered");
  check(delta(art(2200),art(3100))>0,"Idle movement missing");
  for(double phase:{.15,.45,.78})check(delta(art(times.entry*phase),art(times.total()-times.exit*phase))<=1,"Exit shape differs from reverse entry");
  const auto settled=render(2100);const auto centre=style.portraitBox.center();const auto expected=frame->portrait->image.pixelColor(centre-style.portraitBox.topLeft());const auto actual=settled.pixelColor(centre);
  check(std::abs(expected.red()-actual.red())<=6 && std::abs(expected.green()-actual.green())<=6 && std::abs(expected.blue()-actual.blue())<=6,"Profile centre moved/covered");
  const auto uploads=renderer.stats().uploads,rasters=engine.stats().renders;
  int index=0;
  auto save=[&](double ms){const auto image=render(ms);guard(image,style.assetBox);check(image.save(out+QString("/frames/frame-%1.png").arg(index++,4,10,QChar('0'))),"Frame export");};
  // Preview sampling only; the source evaluates motion continuously at OBS FPS.
  for(int n=0;n<60;++n)save(n*2000.0/60);
  for(int n=0;n<90;++n)save(2000+n*6000.0/90);
  for(int n=0;n<21;++n)save(8000+n*700.0/21);
  save(8700);
  for(const auto timing:{sources::AlertTimes{100,0,100},sources::AlertTimes{10000,60000,10000}}){
   auto flexible=*frame;flexible.settings.times=timing;
   for(double p:{.15,.45,.78,1.0}){const auto image=renderer.renderOffscreen({800,800},[&]{drawShoutoutFrame(renderer,flexible,origin+std::chrono::milliseconds(int(timing.entry*p)));});guard(image,style.assetBox);}
  }
  check(renderer.stats().uploads==uploads && engine.stats().renders==rasters,"Per-frame upload/raster");
  check(a.save(out+"/stage-0300.png") && b.save(out+"/stage-0900.png") && c.save(out+"/stage-1550.png") && settled.save(out+"/settled.png"),"Stage export");
  std::cout<<"PASS GPU "<<definition.id.toStdString()<<": motion stages, original settled ink, centred profile, reverse exit, variable times, bounds, cache reuse\n";
  renderer.clear();
 }
 core.releaseSourcePreview(key);core.stop();
 }
 obs_shutdown();return 0;
 }catch(const std::exception &e){std::cerr<<e.what()<<'\n';obs_shutdown();return 1;}}
