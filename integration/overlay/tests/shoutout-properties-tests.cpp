// SPDX-License-Identifier: GPL-2.0-or-later
#include "sources/shoutout-source.hpp"
#include <obs-module.h>
#include <QGuiApplication>
#include <QTemporaryDir>
#include <iostream>
#include <stdexcept>

// Test localization only; source registration and button callbacks are production code.
const char *obs_module_text(const char *key){return key;}
namespace {
void require(bool ok,const char *why){if(!ok)throw std::runtime_error(why);}
struct Config:oto::IConfigStore{
	oto::ConfigLoadResult load()override{return {oto::ConfigStatus::Loaded,{false}};}
	bool save(oto::Settings)override{return true;}
};
struct Log:oto::ILogger{void write(oto::LogLevel,std::string_view)const noexcept override{}};
struct Reloads{bool insideButton=false;int inlineCalls=0,asyncCalls=0;};
void propertiesChanged(void *data,calldata_t *){
	auto &calls=*static_cast<Reloads *>(data);
	if(calls.insideButton)++calls.inlineCalls;else ++calls.asyncCalls;
}
}
int main(int argc,char **argv){
	QGuiApplication app(argc,argv);
	try{
		QTemporaryDir dir;require(dir.isValid(),"Temporary directory unavailable");
		require(obs_startup("en-US",nullptr,nullptr),"OBS startup failed");
		auto core=std::make_shared<oto::PluginCore>(std::make_unique<Config>(),std::make_unique<Log>());core->start();
		auto service=std::make_shared<oto::twitch::Service>(core,dir.filePath("session.json"));
		auto registry=std::make_shared<oto::presets::Registry>(oto::presets::Registry::fromFile(OTO_PRESET_FILE));
		auto hub=std::make_shared<oto::sources::ShoutoutHub>(core,nullptr,nullptr,registry,nullptr,service);
		oto::sources::registerShoutoutSource(hub);
		auto *source=obs_source_create_private("oto_shoutout_card","properties-regression",nullptr);
		require(source,"Shoutout source creation failed");
		Reloads reloads;auto *sourceSignals=obs_source_get_signal_handler(source);
		signal_handler_connect(sourceSignals,"update_properties",propertiesChanged,&reloads);
		const auto click=[&](const char *key,const char *expected){
			auto *props=obs_source_properties(source);require(props,"Properties unavailable");
			reloads.insideButton=true;
			const bool refresh=obs_property_button_clicked(obs_properties_get(props,key),source);
			reloads.insideButton=false;
			require(reloads.inlineCalls==0,"Button rebuilt properties before returning (WidgetInfo use-after-free)");
			require(refresh,"Button didn't request OBS's deferred property refresh");
			const auto status=QString::fromUtf8(obs_property_description(obs_properties_get(props,"preview_status")));
			if(status!=expected)throw std::runtime_error(std::string(key)+": expected "+expected+", got "+status.toStdString());
			obs_properties_destroy(props);
		};
		// Keep graphics inactive: this fixture tests the OBS callback/lifetime contract.
		for(int n=0;n<100;++n){click("preview","Alert.PreviewActive");click("preview_long","Alert.PreviewActive");click("preview_end","Alert.PreviewStopped");}
		QCoreApplication::processEvents();
		signal_handler_disconnect(sourceSignals,"update_properties",propertiesChanged,&reloads);
		obs_source_release(source);QCoreApplication::processEvents();
		auto *settings=obs_data_create();obs_data_set_string(settings,"test_user","@unavailable");
		source=obs_source_create_private("oto_shoutout_card","lookup-regression",settings);obs_data_release(settings);
		require(source,"Lookup source creation failed");
		sourceSignals=obs_source_get_signal_handler(source);signal_handler_connect(sourceSignals,"update_properties",propertiesChanged,&reloads);
		click("preview","Alert.UserLoading");
		QCoreApplication::processEvents();
		require(reloads.asyncCalls==0,"Hidden source accepted a lookup callback");
		click("preview_long","Alert.UserLoading");click("preview_end","Alert.PreviewStopped");
		const int before=reloads.asyncCalls;QCoreApplication::processEvents();
		require(reloads.asyncCalls==before,"Stopped preview accepted a stale lookup callback");
		signal_handler_disconnect(sourceSignals,"update_properties",propertiesChanged,&reloads);
		obs_source_release(source);QCoreApplication::processEvents();
		hub->stop();hub.reset();service->stop();service.reset();core->stop();core.reset();obs_shutdown();
		std::cout<<"303 production button callbacks: no inline reload; hidden lookup and stale cancellation passed\n";return 0;
	}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
}
