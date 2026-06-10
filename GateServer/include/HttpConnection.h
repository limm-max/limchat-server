#pragma once
#include "const.h"

class HttpConnection : public std::enable_shared_from_this<HttpConnection>
{
    friend class LogicSystem;          
public:
    HttpConnection(net::io_context& ioc);
    void Start();                      // 连接接受后：开始异步读请求
    tcp::socket& GetSocket();          // CServer 把 accept 落到这个 socket 上

private:
    void HandleReq();                  // 按 method 分流到 LogicSystem
    void WriteResponse();              // 回包 + 关连接（短连接）
    void CheckDeadline();              // 超时保护

    tcp::socket _socket;                                  
    beast::flat_buffer _buffer{ 8192 };
    http::request<http::dynamic_body>  _request;
    http::response<http::dynamic_body> _response;
    net::steady_timer deadline_{ _socket.get_executor(),   
                                 std::chrono::seconds(60) };
};