#pragma once
#include "const.h"
#include "HttpConnection.h"

class CServer{
public:
    CServer(net::io_context& ioc,unsigned short port);

private:
    //持续accept连接
    void StartAccept();

    net::io_context& _ioc;
    tcp::acceptor _acceptor;
};