#include "CServer.h"
#include "CSession.h"
#include "IOServicePool.h"
#include"Logger.h"

CServer::CServer(net::io_context& ioc,unsigned short port)
    :_ioc(ioc)
    ,_acceptor(ioc,tcp::endpoint(tcp::v4(),port))   //构造+bind+listen
{
    StartAccept();

}

void CServer::StartAccept(){
    //让新建的socket挂到ioservicepool池中的io_context上，而不占CServer的io_context
    auto& ioc=IOServicePool::GetInstance()->GetIOService();
    auto new_session=std::make_shared<CSession>(ioc,this);

    _acceptor.async_accept(new_session->GetSocket(),
        [this, new_session](const boost::system::error_code& error) {
            HandleAccept(new_session,error);

        });
}

void CServer::HandleAccept(std::shared_ptr<CSession> session,const boost::system::error_code& error){
    if (!error) {
        LOG_INFO<<"accept new session";
        session->Start();
    } else {
        LOG_ERROR << "accept error: " << error.message();
    }
    StartAccept();   // 继续接下一个
}