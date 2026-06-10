#include "HttpConnection.h"
#include "LogicSystem.h"
#include<iostream>
#include "Logger.h"
#include "AsyncLogging.h"
HttpConnection::HttpConnection(net::io_context& ioc) : _socket(ioc) {}
tcp::socket& HttpConnection::GetSocket() { return _socket; }

void HttpConnection::Start() {
    auto self = shared_from_this();        // mortal！靠 self 续命到回调跑完
    http::async_read(_socket, _buffer, _request,
        [self](beast::error_code ec,std::size_t bytes){
            try{
                if(ec){
                    LOG_ERROR << "[GateServer]http req err:" << ec.message();
                    return;
                }

                boost::ignore_unused(bytes);
                self->HandleReq();
                self->CheckDeadline();
            }catch(std::exception& e){
                LOG_ERROR << "[GateServer]exception: " << e.what();
            }
        });
}


void HttpConnection::HandleReq() {
    _response.version(_request.version());
    _response.keep_alive(false); 

    if (_request.method() == http::verb::post) {
        // target() 是 path（如 "/get_varifycode"），转 std::string 当 map 的 key
        bool ok = LogicSystem::GetInstance()->HandlePost(
                      std::string(_request.target()), shared_from_this()); 
        _response.result(ok ? http::status::ok : http::status::not_found);
        if (!ok) beast::ostream(_response.body()) << "url not found\r\n";
        _response.set(http::field::server, "GateServer");
        WriteResponse();
        return;
    }
    else if (_request.method() == http::verb::get) {
        bool ok = LogicSystem::GetInstance()->HandleGet(
                      std::string(_request.target()), shared_from_this());
        _response.result(ok ? http::status::ok : http::status::not_found);
        if (!ok) beast::ostream(_response.body()) << "url not found\r\n";
        _response.set(http::field::server, "GateServer");   //设置响应头，http::field::是响应字段头名称枚举
        WriteResponse();
        return;
    }
}

void HttpConnection::WriteResponse() {
    auto self = shared_from_this();
    _response.content_length(_response.body().size());   // 设 Content-Length（让客户端知道 body 多长）
    http::async_write(_socket, _response,
        [self](beast::error_code ec, std::size_t) {
            LOG_DEBUG << "[GateServer]回复已发送，正在关闭连接";
            self->_socket.shutdown(tcp::socket::shutdown_send, ec);  // 关写端
            self->deadline_.cancel();                                // 撤销超时
        });
}

void HttpConnection::CheckDeadline() {
    auto self = shared_from_this();
    deadline_.async_wait([self](beast::error_code ec) {
        if (!ec) {
            LOG_DEBUG << "[GateServer]请求超时，关闭连接";
            self->_socket.close(ec);
        }
    });
}