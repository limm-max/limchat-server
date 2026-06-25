#include"const.h"
#include"CServer.h"
#include"IOServicePool.h"
#include"ConfigMgr.h"
#include "Logger.h"
#include "AsyncLogging.h"


using namespace lim;
static AsyncLogging* g_asyncLog = nullptr;
static void asyncOutput(const char* msg, int len) { g_asyncLog->append(msg, len); }


int main(){

    AsyncLogging asyncLog("log/ChatServer", 500 * 1000 * 1000);   // basename, rollSize=500MB
    g_asyncLog = &asyncLog;
    Logger::setOutput(asyncOutput);          //  Logger 出口接到异步后端
    Logger::setLogLevel(Logger::DEBUG);      // 运行期级别
    asyncLog.start();
    LOG_INFO<<"日志系统启动完毕。";

    try{
        auto cfg=ConfigMgr::GetInstance();
        unsigned short port=static_cast<unsigned short>(std::stoi((*cfg)["ChatServer"]["port"]));

        net::io_context ioc;
        net::signal_set signals(ioc,SIGINT,SIGTERM);
        signals.async_wait([&ioc](auto, auto) {
            LOG_INFO << "Shutting down ChatServer..." ;
            IOServicePool::GetInstance()->stop();
            ioc.stop();
        });

        CServer acceptor(ioc,port);
        LOG_INFO<<"ChatServer runs by listening on "<<port;
        ioc.run();
    }catch(const std::exception& e){
        LOG_FATAL<<"ChatServer error:"<<e.what();
    }
    return 0;
}
