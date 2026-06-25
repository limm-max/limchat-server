// CSession.cpp
#include "CSession.h"
#include "CServer.h"
#include "const.h"
#include"Logger.h"
#include"LogicSystem.h"
#include"UserMgr.h"
#include"ConfigMgr.h"
#include"RedisMgr.h"
#include <iostream>
#include <cstring>

std::atomic<int> CSession::_session_id_gen{1};

CSession::CSession(boost::asio::io_context& io_context, CServer* server)
    : _socket(io_context), _server(server),_b_close(false),_uid(0)
    ,_session_id(_session_id_gen++)
    {  _recv_head_node=std::make_shared<MsgNode>(HEAD_TOTAL_LEN);}

//先读头
void CSession::Start() {
    //先读4字节头
    _recv_head_node->_cur_len=0;
    memset(_recv_head_node->_data, 0, HEAD_TOTAL_LEN);

    // shared_from_this() 关键：把自己塞进回调，保证异步期间 session 不被析构
    //使用async_read api指定读的字节数
    net::async_read(_socket,net::buffer(_recv_head_node->_data, HEAD_TOTAL_LEN),
        [self = shared_from_this()](const boost::system::error_code& error, size_t bytes) {
            self->HandleReadHead(error);
        });
}


//处理读到的头字节，然后读body
void CSession::HandleReadHead(const boost::system::error_code& error) {
    if(error){
        LOG_INFO<<"read head error:"<<error.message(); 
        Close();
        return;}
    short msg_id=0, msg_len=0;
    memcpy(&msg_id,_recv_head_node->_data,HEAD_ID_LEN);
    memcpy(&msg_len,_recv_head_node->_data+HEAD_ID_LEN,HEAD_DATA_LEN);
    msg_id=ntohs(msg_id);
    msg_len=ntohs(msg_len);
    if(msg_len>MAX_LENGTH){LOG_ERROR<<"receive illegal message len:"<<msg_len;}

    //读出整个body长度msg_len
    _recv_msg_node=std::make_shared<RecvNode>(msg_len,msg_id);
    net::async_read(_socket,net::buffer(_recv_msg_node->_data,msg_len),
                    [self=shared_from_this()](const boost::system::error_code& ec, size_t ){

                        self->HandleReadBody(ec);
                    });

}

//处理body
void CSession::HandleReadBody(const boost::system::error_code& error){
    if(error){
        LOG_INFO<<"read body error:"<<error.message(); 
        Close();
        return;}

    auto node=std::make_shared<LogicNode>(shared_from_this(),_recv_msg_node);
    LogicSystem::GetInstance()->PostMsgToQue(node);
    
    Start();    //继续下一条信息的读取

}


//发送消息入队。如果前序无等待消息则直接发送，如果有则返回（handlewrite会一直处理队列）
void CSession::Send(const char* msg, short max_len, short msg_id){
    std::lock_guard<std::mutex> lock(_send_mutex);
    bool pending=!_send_que.empty();
    _send_que.push(std::make_shared<SendNode>(msg,max_len,msg_id));
    if(pending) return;
    auto& node=_send_que.front();
    // _send_que.pop();     真正send出去之后才从队列中pop出去。
    net::async_write(_socket,net::buffer(node->_data,node->_total_len),
                     [self=shared_from_this()](const boost::system::error_code& ec, size_t) {
                        self->HandleWrite(ec);
                     });

}
 
void CSession::HandleWrite(const boost::system::error_code& ec){
    if(ec){ LOG_ERROR<<"write error:"<<ec.message(); return;}
    std::lock_guard<std::mutex> lock(_send_mutex);
    _send_que.pop();
    if(!_send_que.empty()){
        auto& node=_send_que.front();
        net::async_write(_socket,net::buffer(node->_data,node->_total_len),
                     [self=shared_from_this()](const boost::system::error_code& ec, size_t) {
                        self->HandleWrite(ec);
                     });
    }
}

void CSession::SetUid(int uid){
    _uid=uid;
}

int CSession::GetUid(){
    return _uid;
}

int CSession::GetSessionid(){
    return _session_id;
}

void CSession::Close(){
    if(_b_close)    return;
    _b_close=true;

    int uid=GetUid();
    
    //登记过的uid才需要清理
    if(uid!=0){
        UserMgr::GetInstance()->RemoveUserSession(uid,shared_from_this());

        auto name=(*ConfigMgr::GetInstance())["SelfServer"]["name"];
        RedisMgr::GetInstance()->Decr(LOGINCOUNTPREFIX+name);
        RedisMgr::GetInstance()->Del(USERSERVERPREFIX+std::to_string(uid));
        LOG_INFO << "user offline, uid=" << uid;
    }

    boost::system::error_code ec;
    _socket.shutdown(tcp::socket::shutdown_both, ec);
    _socket.close(ec);

}