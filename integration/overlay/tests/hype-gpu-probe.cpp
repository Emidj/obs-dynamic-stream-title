// SPDX-License-Identifier: GPL-2.0-or-later
#include "adapters/hype-draw.hpp"
#include "visual/asset-manager.hpp"
#include <QApplication>
#include <QDir>
#include <QPainter>
#include <QFileInfo>
#include <obs.h>
#include <iostream>
using namespace oto;
namespace {
void require(bool ok,const char *why){if(!ok)throw std::runtime_error(why);}
int difference(const QImage &a,const QImage &b){int d=0;for(int y=0;y<a.height();++y)for(int x=0;x<a.width()*4;++x)d=(std::max)(d,std::abs(int(a.constScanLine(y)[x])-int(b.constScanLine(y)[x])));return d;}
}
int main(int argc,char **argv){QApplication app(argc,argv);try{
	require(argc==5 || argc==6,"Usage: hype-gpu-probe graphics-module obs-data registry output [contributors]");
	require(obs_startup("en-US",nullptr,nullptr),"OBS startup failed");
	// Standalone development executable needs the installed OBS base effects.
#pragma warning(push)
#pragma warning(disable:4996)
	obs_add_data_path(argv[2]);
#pragma warning(pop)
	obs_video_info video{};video.graphics_module=argv[1];video.fps_num=30;video.fps_den=1;video.base_width=video.output_width=900;video.base_height=video.output_height=180;video.output_format=VIDEO_FORMAT_RGBA;video.colorspace=VIDEO_CS_DEFAULT;video.range=VIDEO_RANGE_FULL;video.scale_type=OBS_SCALE_DISABLE;
	require(obs_reset_video(&video)==OBS_VIDEO_SUCCESS,"D3D video startup failed");
	const auto registry=presets::Registry::fromFile(QString::fromUtf8(argv[3]));const QString directory=QString::fromUtf8(argv[4]);require(QDir().mkpath(directory),"Output directory failed");
	{
		ObsVisualRenderer renderer;visual::VisualEngine engine;
		visual::AssetManager assets(QFileInfo(QString::fromUtf8(argv[3])).dir().filePath("assets"));
		const auto start=visual::MotionClock::time_point(std::chrono::seconds(100));
		sources::HypeView view;view.visible=true;view.level=3;view.progress=3900;view.goal=5000;view.total=8900;view.ratio=.78;view.remaining=84;view.trainId="gpu-fixture";view.contributors={{{"a","TechnoClub"},1500,2,0,1500,3},{{"b","MarinaDJ"},1200,1,1200,0,{}},{{"c","Alex_東京"},750,3,250,500,1}};
		QImage gallery(900,2200,QImage::Format_RGBA8888_Premultiplied);gallery.fill(QColor(12,13,22));QPainter painter(&gallery);painter.setPen(Qt::white);int row=0;
		for(const auto &preset:registry.entries()){
			require(bool(preset.hypeStyle),"Factory Hype metadata missing");sources::HypeSettings settings;settings.presetId=preset.id;settings.style=*preset.hypeStyle;settings.accent=settings.style.accent;settings.labels=settings.style.labels;settings.numbers=settings.style.numbers;settings.numberFamily=settings.labelFamily=settings.style.family;
			const auto artwork=assets.loadRegion(settings.style.atlas,settings.style.atlasCrop),thankArt=assets.loadRegion(settings.style.thanksAtlas,settings.style.thanksCrop);
			const auto frame=sources::prepareHypeFrame(engine,settings,{},view,{},start,true,artwork,thankArt);require(frame->authored,"Authored meter not loaded");
			const auto twin=sources::prepareHypeFrame(engine,settings,{},view,{},start,true,artwork,thankArt);
			require(frame->base==twin->base && frame->fill==twin->fill && frame->title==twin->title && frame->thanks==twin->thanks,"Identical meter instances duplicate graphics");
			auto endedView=view;endedView.ended=true;const auto end=sources::prepareHypeFrame(engine,settings,{},endedView,frame,start+std::chrono::milliseconds(4500));
			const auto render=[&](int ms,bool padded=false){return renderer.renderOffscreen(padded?QSize(1000,280):QSize(900,180),[&]{gs_matrix_push();if(padded)gs_matrix_translate3f(50,50,0);drawHypeFrame(renderer,ms<4500?*frame:*end,start+std::chrono::milliseconds(ms));gs_matrix_pop();});};
			if(argc==6){
				const auto settled=render(2800);require(settled.save(directory+"/"+preset.id+".png"),"Contributor poster failed");
				painter.drawText(15,row*220+24,preset.name);painter.drawImage(0,row*220+34,settled);++row;
				const auto guard=render(2800,true);for(int y=0;y<280;++y)for(int x=0;x<1000;++x)if(!QRect(50,50,900,180).contains(x,y))require(guard.constScanLine(y)[x*4+3]==0,"Contributor content exceeds source");
				auto without=view;without.contributors.clear();const auto clean=sources::prepareHypeFrame(engine,settings,{},without,{},start,true,artwork,thankArt);
				for(int y=0;y<180;++y)for(int x=0;x<900;++x)if(frame->numbers->image.pixel(x,y)!=clean->numbers->image.pixel(x,y))require(frame->leaderBox.contains(x,y),"Amounts leave contributor panel");
				const auto uploads=renderer.stats().uploads,rasters=engine.stats().renders;for(int ms:{2900,3000,3100,3200})render(ms);
				require(renderer.stats().uploads==uploads && engine.stats().renders==rasters,"Contributor updates rasterize per frame");
				std::cout<<"PASS contributor layout GPU "<<preset.id.toStdString()<<'\n';continue;
			}
			QImage blank(900,180,QImage::Format_RGBA8888_Premultiplied);blank.fill(Qt::transparent);
			require(difference(render(0),blank)==0 && difference(render(11500),blank)==0,"Exact start/end not transparent");
			const auto title=render(350),contour=render(1100),bar=render(1700),settled=render(2800),collected=render(5325),morph=render(5625),thanks=render(6000);
			require(difference(title,contour)>30 && difference(contour,bar)>30 && difference(bar,settled)>30,"GPU entrance stages are indistinguishable");
			require(difference(settled,collected)>30 && difference(collected,morph)>30 && difference(morph,thanks)>30,"GPU collect/morph missing");
			require(difference(render(3000),render(3300))>20,"No internal GPU movement");
			const auto titleProbe=[&](bool material,float progress){return renderer.renderOffscreen({900,180},[&]{DrawParams p;p.clip=QRectF(2,2,896,176);p.material=material?1.0f:0;p.materialVariant=6;p.dissolve=progress;p.time=5;renderer.draw(progress<1?frame->title:frame->thanks,true,p);});};
			require(difference(titleProbe(false,0),titleProbe(true,0))<=1 && difference(titleProbe(false,1),titleProbe(true,1))<=2,"Morph boundary jumps pixels");
			painter.drawText(15,row*220+24,preset.name);painter.drawImage(0,row*220+34,settled);++row;
			require(settled.save(directory+"/"+preset.id+".png"),"Style poster save failed");
			QDir().mkpath(directory+"/"+preset.id);
			// Warm every immutable layer, then prove the animation needs no new raster or upload.
			render(4500);
			const auto uploads=renderer.stats().uploads,rasters=engine.stats().renders;
			for(int n=0;n<=345;++n){const int ms=qRound(n*1000.0/30);const auto image=render(ms);
				require(image.save(directory+"/"+preset.id+QStringLiteral("/frame-%1.png").arg(n,3,10,QChar('0'))),"Frame save failed");
				if(n%15==0){const auto padded=render(ms,true);for(int y=0;y<280;++y)for(int x=0;x<1000;++x)if(!QRect(50,50,900,180).contains(x,y))require(padded.constScanLine(y)[x*4+3]==0,"GPU animation exceeds source bounds");}
			}
			require(renderer.stats().uploads==uploads && engine.stats().renders==rasters,"Per-frame rasterization or texture uploads");
			// All three selectable fills retain the authored frame; fixed/traffic stay stable.
			std::array<QImage,3> modes;
			for(int mode=0;mode<3;++mode){auto choice=settings;choice.fillMode=mode;const auto f=sources::prepareHypeFrame(engine,choice,{},view,{},start,false,artwork,thankArt);
				const auto drawFill=[&](int ms){return renderer.renderOffscreen({900,180},[&]{DrawParams p;p.clip=QRectF(f->bar.x(),f->bar.y(),f->bar.width()*.78,f->bar.height());if(mode==2){p.material=1;p.materialVariant=float(17+settings.style.pattern);p.time=ms/1000.f;p.dissolve=1;}renderer.draw(f->fill,true,p);});};
				modes[mode]=drawFill(3000);const auto moving=drawFill(3300);const auto delta=difference(modes[mode],moving);if(!(mode==2?delta>10:delta==0))std::cerr<<preset.id.toStdString()<<" fill mode "<<mode<<" pixel difference "<<delta<<'\n';require(mode==2?delta>10:delta==0,"Fill mode movement mismatch");
				if(preset.id=="neon-dj")modes[mode].save(directory+QStringLiteral("/fill-%1.png").arg(mode));
			}
			require(difference(modes[0],modes[1])>50 && difference(modes[0],modes[2])>50,"Fill modes indistinguishable");
			// Compare identical updated values at identical clocks: the extra pixels
			// must come from the level-up effect, not a changed number or ambient phase.
			auto raised=view;raised.level=4;raised.ratio=.02;raised.progress=100;
			for(int mode=0;mode<3;++mode){auto choice=settings;choice.fillMode=mode;
				const auto prior=sources::prepareHypeFrame(engine,choice,{},view,{},start,false,artwork,thankArt);
				const auto raisedFrame=sources::prepareHypeFrame(engine,choice,{},raised,prior,start+std::chrono::seconds(3));
				auto quiet=*raisedFrame;quiet.celebrated={};
				const auto levelRender=[&](int ms,bool active=true,bool padded=false){return renderer.renderOffscreen(padded?QSize(1000,280):QSize(900,180),[&]{gs_matrix_push();if(padded)gs_matrix_translate3f(50,50,0);drawHypeFrame(renderer,active?*raisedFrame:quiet,start+std::chrono::milliseconds(3000+ms));gs_matrix_pop();});};
				require(difference(levelRender(0),levelRender(0,false))==0 && difference(levelRender(2600),levelRender(2600,false))==0,"Level effect start/end jumps");
				const auto flash=levelRender(300),baseline=levelRender(300,false);int affected=0;
				for(int y=0;y<180;++y)for(int x=0;x<900;++x){const auto *a=flash.constScanLine(y)+x*4,*b=baseline.constScanLine(y)+x*4;bool changed=false;for(int c=0;c<4;++c)changed|=std::abs(int(a[c])-int(b[c]))>25;if(changed)++affected;}
				require(affected>600 && difference(flash,levelRender(1100))>30,"Level celebration lacks visible GPU movement");
				const auto levelUploads=renderer.stats().uploads,levelRasters=engine.stats().renders;
				if(mode==2)QDir().mkpath(directory+"/"+preset.id+"/level");
				for(int n=0;n<=90;++n){const int ms=qRound(n*1000.0/30);const auto image=levelRender(ms);
					if(mode==2)require(image.save(directory+"/"+preset.id+QStringLiteral("/level/frame-%1.png").arg(n,3,10,QChar('0'))),"Level preview save failed");
					if(n%15==0){const auto guard=levelRender(ms,true,true);require(guard.size()==QSize(1000,280),"Level guard canvas missing");for(int y=0;y<280;++y)for(int x=0;x<1000;++x)if(!QRect(50,50,900,180).contains(x,y) && guard.constScanLine(y)[x*4+3]!=0){guard.save(directory+"/level-guard-failure.png");std::cerr<<preset.id.toStdString()<<" mode "<<mode<<" age "<<ms<<" exterior "<<x<<","<<y<<'\n';require(false,"Level celebration exceeds bounds");}}
				}
				require(renderer.stats().uploads==levelUploads && engine.stats().renders==levelRasters,"Level celebration rerasterizes or uploads per frame");
				std::cout<<"PASS GPU level "<<preset.id.toStdString()<<" mode "<<mode<<": "<<affected<<" impact pixels, continuous boundaries, bounded/no uploads\n";
			}
			std::cout<<"PASS GPU "<<preset.id.toStdString()<<": staged entrance, live energy, collection/morph/THANK YOU, exterior guards, no per-frame raster/uploads\n";
		}
		// Maximum advertised resolution must also avoid upload churn.
		{
			const auto &preset=registry.entries().front();sources::HypeSettings big;
			big.width=1920;big.height=400;big.style=*preset.hypeStyle;big.accent=big.style.accent;
			const auto art=assets.loadRegion(big.style.atlas,big.style.atlasCrop),thankArt=assets.loadRegion(big.style.thanksAtlas,big.style.thanksCrop);
			const auto full=sources::prepareHypeFrame(engine,big,{},view,{},start,true,art,thankArt);
			auto last=view;last.ended=true;const auto end=sources::prepareHypeFrame(engine,big,{},last,full,start+std::chrono::milliseconds(4500));
			const auto render=[&](int ms){renderer.renderOffscreen({1920,400},[&]{drawHypeFrame(renderer,ms<4500?*full:*end,start+std::chrono::milliseconds(ms));});};
			render(3000);render(5600);const auto uploads=renderer.stats().uploads;
			for(int ms:{3100,3200,5400,5500,5600,5700,3100,3200})render(ms);
			require(renderer.stats().uploads==uploads,"Maximum-size meter repeatedly uploads immutable planes");
			require(renderer.stats().bytes<=ObsVisualRenderer::textureBudgetBytes,"GPU cache budget exceeded");
			std::cout<<"PASS GPU maximum size: stable uploads and bounded shared cache\n";
		}
		painter.end();require(gallery.save(directory+"/styles.png"),"Gallery save failed");
		renderer.clear();
	}
	obs_shutdown();return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';obs_shutdown();return 1;}}
