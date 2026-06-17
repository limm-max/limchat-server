#include "const.h"
#include "CServer.h"
#include "LogicSystem.h"
#include<iostream>
#include "Logger.h"
#include "AsyncLogging.h"
#include "ConfigMgr.h"
#include "RedisMgr.h"
using namespace lim;

static AsyncLogging* g_asyncLog = nullptr;
static void asyncOutput(const char* msg, int len) { g_asyncLog->append(msg, len); }


int main(){


    //装载日志系统
    AsyncLogging asyncLog("log/GateServer", 500 * 1000 * 1000);   // basename, rollSize=500MB
    g_asyncLog = &asyncLog;
    Logger::setOutput(asyncOutput);          //  Logger 出口接到异步后端
    Logger::setLogLevel(Logger::DEBUG);      // 运行期级别
    asyncLog.start();
    LOG_INFO<<"日志系统启动完毕。";

    //读取配置
    auto cfg=ConfigMgr::GetInstance();
    std::string port_str = (*cfg)["GateServer"]["port"]; 
    unsigned short port = static_cast<unsigned short>(std::stoi(port_str));
    //unsigned short port=static_cast<unsigned short>(8080);


    LogicSystem::GetInstance();
    net::io_context ioc{1};
    net::signal_set signals(ioc,SIGINT,SIGTERM);    //注册SIGINT（Ctrl+C 触发）和 SIGTERM（kill 命令触发）
    //优雅退出
    signals.async_wait([&ioc](const boost::system::error_code& error, int signal_number){
        if(error){
            return;
        }
        ioc.stop();
    });

    CServer server(ioc, port);           // 栈对象：裸 this 安全（生命周期包住 run()）
    LOG_INFO << "[GateServer]服务器启动 " ;
    ioc.run();
    LOG_INFO << "[GateServer]服务器关闭 " ;
    asyncLog.stop(); 
    return 0;

}