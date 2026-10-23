// SPDX-License-Identifier: GPL-2.0-or-later
#include "twitch/shared-provider.hpp"
#include "twitch-core-client.hpp"
#include <QCoreApplication>
#include <QTemporaryDir>
#include <iostream>
#include <thread>
#include <QUrlQuery>
#include "twitch-core-diagnostics.hpp"
using namespace oto;
struct Config:IConfigStore{ConfigLoadResult load()override{return {ConfigStatus::Loaded,{false}};}bool save(Settings)override{return true;}};
struct Log:ILogger{void write(LogLevel,std::string_view)const noexcept override{}};
#define CHECK(x) do{if(!(x)){std::cerr<<"FAIL "<<__LINE__<<"\n";return 1;}}while(false)
int main(int argc,char **argv){QCoreApplication app(argc,argv);QTemporaryDir dir;
 NexusTwitchCoreInfo info{};CHECK(!nexus_twitch_core_info(nullptr,sizeof(info)));CHECK(!nexus_twitch_core_info(&info,1));CHECK(nexus_twitch_core_info(&info,sizeof(info))&&nexus::compatibleCore(info));CHECK(QString::fromLatin1(info.version)=="1.1.0"&&nexus::corePath(info).endsWith("nexus-twitch-core.dll"));
 info.abi=7;CHECK(!nexus::compatibleCore(info));info.abi=6;std::strcpy(info.version,"2.0.0");CHECK(!nexus::compatibleCore(info));std::strcpy(info.version,"1.1.0");CHECK(nexus::compatibleCore(info));
 auto core=std::make_shared<PluginCore>(std::make_unique<Config>(),std::make_unique<Log>());core->start();
 auto service=std::make_shared<twitch::Service>(core,dir.filePath("session.bin"));
 int legacy=0;service->setChanged([&]{++legacy;});twitch::publishSharedService(service);
 const auto *api=nexus_twitch_core_get_api(NEXUS_TWITCH_ABI,sizeof(NexusTwitchApi));CHECK(api);
 CHECK(!nexus_twitch_core_get_api(99,sizeof(NexusTwitchApi))&&!nexus_twitch_core_get_api(NEXUS_TWITCH_ABI,0));
 dst::TwitchCoreClient first,second;int a=0,b=0;
 first.setChanged([&](uint32_t){++a;});second.setChanged([&](uint32_t){++b;});
 CHECK(first.attach(api)&&second.attach(api));NexusTwitchMessage x{},y{};
 CHECK(first.snapshot(x)&&second.snapshot(y)&&x.instance==y.instance&&!x.active);
 service->connectAccount("bad");CHECK(legacy==1&&a==1&&b==1);
 first.detach();service->connectAccount("bad");CHECK(legacy==2&&a==1&&b==2&&second.available());
 uint64_t rejected=99;std::thread foreign([&]{rejected=api->acquire(nullptr,nullptr);});foreign.join();CHECK(rejected==0);
 auto stale=api->acquire(nullptr,nullptr);CHECK(stale);
 api->release(stale);CHECK(!api->snapshot(stale,&x,sizeof(x)));
 twitch::unpublishSharedService();CHECK(!second.available()&&b==3&&!api->snapshot(stale,&x,sizeof(x)));
 CHECK(!api->acquire(nullptr,nullptr));second.detach();
 // The provider may be registered again; stale handles never address a new owner.
 twitch::publishSharedService(service);CHECK(first.attach(api)&&first.snapshot(y)&&y.instance!=x.instance);
 twitch::unpublishSharedService();service->stop();core->stop();
 // Independent runtime, two plugin owners: no Overlay DLL and no browser UI.
 const auto path=dir.filePath("standalone/session.bin").toUtf8();
 nexus_twitch_runtime_set_locale("fr-FR"); // No OBS instance is required by the shared runtime.
 CHECK(nexus_twitch_runtime_retain(path.constData()));
 CHECK(nexus_twitch_runtime_retain(path.constData()));
 CHECK(first.attach(api)&&second.attach(api));
 CHECK(first.snapshot(x)&&second.snapshot(y)&&x.instance==y.instance&&!x.active);
 nexus_twitch_runtime_release();CHECK(second.snapshot(y));
 CHECK(second.authorize());CHECK(second.snapshot(y)&&y.active&&!y.authorized);
 CHECK(QUrlQuery(QUrl(QString::fromUtf8(y.verification))).queryItemValue("scope",QUrl::FullyDecoded).split(' ').contains("channel:manage:broadcast"));
 CHECK(second.disconnect());CHECK(second.snapshot(y)&&!y.active&&!*y.verification);
 first.detach();nexus_twitch_runtime_release();CHECK(!second.available()&&!nexus_twitch_runtime_active());
 std::cout<<"PASS: one owner, two consumers, legacy notifications, release isolation, ABI/thread checks, shutdown invalidation, stale lease rejection\n";
}

