#include "CServer.h"
#include<iostream>
CServer::CServer(net::io_context& ioc,unsigned short port)
    :_ioc(ioc)
    ,_acceptor(ioc,tcp::endpoint(tcp::v4(),port))   //构造+bind+listen
{
    StartAccept();

}

void CServer::StartAccept(){
    auto new_conn=std::make_shared<HttpConnection>(_ioc);

    _acceptor.async_accept(new_conn->GetSocket(),
        [this, new_conn](beast::error_code ec) {
            //判断是否accept成功
            if (ec) {
                StartAccept();
                return;
            }

            //accpet成功之后，让连接开启异步读,进入自己的读写生命周期
            new_conn->Start();

            //成功也要继续造另外一个空连接
            StartAccept();

        });
}