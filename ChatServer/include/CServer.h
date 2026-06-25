#pragma once
#include "const.h"
#include<memory>

class CSession;
class CServer{
public:
    CServer(net::io_context& ioc,unsigned short port);

private:
    //持续accept连接
    void StartAccept();
    void HandleAccept(std::shared_ptr<CSession> session,const boost::system::error_code& error);

    net::io_context& _ioc;
    tcp::acceptor _acceptor;
};