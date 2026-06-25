//A1简化版
// CSession.h
#pragma once
#include <boost/asio.hpp>
#include <memory>
#include<queue>
#include<mutex>
#include<atomic>
#include"MsgNode.h"

class CServer;


class CSession : public std::enable_shared_from_this<CSession> {
public:

    //CSession自编号自增
    static std::atomic<int> _session_id_gen;
    CSession(boost::asio::io_context& io_context, CServer* server);
    boost::asio::ip::tcp::socket& GetSocket() { return _socket; }
    void Start();
    void Send(const char* msg, short max_len, short msg_id);

    void SetUid(int uid);
    int  GetUid();

    int GetSessionid();

    //负责关闭socket，清楚usermgr中的登记和redismgr中的登记
    void Close();
private:

    boost::asio::ip::tcp::socket _socket;
    CServer* _server;
    int _uid;   //用户uid
    bool _b_close;
    int _session_id;

    //读节点
    void HandleReadHead(const boost::system::error_code& error);
    void HandleReadBody(const boost::system::error_code& error);

    std::shared_ptr<MsgNode> _recv_head_node;
    std::shared_ptr<RecvNode> _recv_msg_node;

    //发送队列+锁（使用发送队列处理异步发送）
    //加锁：因为send函数不一定由本线程执行：比如处理某个msg的handler跑在另一个session所在的线程中
    //或者跨服转发的grpc handler
    
    void HandleWrite(const boost::system::error_code& ec);
    std::queue<std::shared_ptr<SendNode>> _send_que;
    std::mutex _send_mutex;
};