//logicsystem使用，记录每一个消息的来源（CSession）和对应消息体
#pragma once
#include <memory>

class CSession;     
class RecvNode;

class LogicNode{
public:
    LogicNode(std::shared_ptr<CSession> session, std::shared_ptr<RecvNode> recvnode)
        : _session(session), _recvnode(recvnode) {}
    std::shared_ptr<CSession> _session;     // 哪条连接发来的（回包要用）
    std::shared_ptr<RecvNode> _recvnode;    // 消息体（含 msg_id 和 data）
};