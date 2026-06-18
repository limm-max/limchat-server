#include <grpcpp/grpcpp.h>
#include <boost/asio.hpp>
#include <thread>
#include <iostream>
#include "StatusServiceImpl.h"
#include "ConfigMgr.h"
#include "Logger.h"
#include "AsyncLogging.h"
#include"const.h"

using namespace lim;
static AsyncLogging* g_asyncLog = nullptr;
static void asyncOutput(const char* msg, int len) { g_asyncLog->append(msg, len); }


int main(){

    AsyncLogging asyncLog("log/StatusServer", 500 * 1000 * 1000);   // basename, rollSize=500MB
    g_asyncLog = &asyncLog;
    Logger::setOutput(asyncOutput);          //  Logger 出口接到异步后端
    Logger::setLogLevel(Logger::DEBUG);      // 运行期级别
    asyncLog.start();
    LOG_INFO<<"日志系统启动完毕。";

    try{
        auto cfg=ConfigMgr::GetInstance();
        std::string server_address=(*cfg)["StatusServer"]["host"]+":"+(*cfg)["StatusServer"]["port"];

        StatusServiceImpl service;

        grpc::ServerBuilder builder;
        builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
        builder.RegisterService(&service);
        std::unique_ptr<grpc::Server> server(builder.BuildAndStart());
        LOG_INFO << "StatusServer listening on " << server_address;

        // server->Wait() 是阻塞的，丢到单独线程去阻塞
        std::thread grpc_thread([&server]() { server->Wait(); });


        net::io_context io_context;
        net::signal_set signals(io_context, SIGINT, SIGTERM);
        signals.async_wait([&server, &io_context](const boost::system::error_code& ec, int) {
            if (ec) return;
            LOG_INFO << "Shutting down StatusServer..." ;
            server->Shutdown();   // 让 grpc_thread 里的 Wait() 返回
            io_context.stop();
        });

        LOG_INFO<<"StatusServer run.";
        io_context.run();         // 阻塞等信号

        LOG_INFO<<"StatusServer stop.";
        grpc_thread.join();
    
    }catch(const std::exception& e){
        LOG_FATAL<<"StatusServer error:"<<e.what();
    }
}