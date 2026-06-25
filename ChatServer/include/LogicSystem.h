// LogicSystem.h
#pragma once
#include <map>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include "Singleton.h"
#include "LogicNode.h"

typedef std::function<void(std::shared_ptr<CSession>, short, const std::string&)> FunCallBack;

class LogicSystem : public Singleton<LogicSystem> {
    friend class Singleton<LogicSystem>;
public:
    ~LogicSystem();
    void PostMsgToQue(std::shared_ptr<LogicNode> msg);
private:
    LogicSystem();
    void DealMsg();              // 工作线程主循环
    void RegisterCallBacks();

    void HelloWordCallBack(std::shared_ptr<CSession>, short, const std::string&);  // echo 占位
    void LoginCallBack(std::shared_ptr<CSession>, short, const std::string&);
    void SearchUserCallBack(std::shared_ptr<CSession>, short, const std::string&);

    std::thread _worker_thread;
    std::queue<std::shared_ptr<LogicNode>> _msg_que;
    std::mutex _mutex;
    std::condition_variable _consume;   //工作线程需要的唤醒条件
    bool _b_stop;
    std::map<short, FunCallBack> _fun_callbacks;   // msg_id → handler
};