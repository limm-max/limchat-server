//全局配置，命名空间别名、ErrorCodes与客户端对齐

#pragma once

#include<boost/beast.hpp>
#include<boost/beast/http.hpp>
#include<boost/asio.hpp>

#include <nlohmann/json.hpp>

namespace beast = boost::beast;   // 之后写 beast::flat_buffer，而非 boost::beast::flat_buffer
namespace http  = beast::http;    // http::request，而非 boost::beast::http::request
namespace net   = boost::asio;    // net::io_context，而非 boost::asio::io_context
using tcp = boost::asio::ip::tcp; // tcp::acceptor / tcp::socket / tcp::endpoint
using json=nlohmann::json;

enum ErrorCodes{
    Success=0,
    Error_Json=1001,
    RPCFailed=1003,
    RedisErr=1004,
};