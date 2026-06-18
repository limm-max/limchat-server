#pragma once

#include "Noncopyable.h"
#include "FixedBuffer.h"

#include <string>

namespace lim{
// LogStream:类似 std::cout 的流式接口
// 但底层写到 FixedBuffer,不是终端

class LogStream:Noncopyable{
public:
    typedef detail::FixedBuffer<detail::kSmallBuffer> Buffer;

    // 一系列 operator<< 重载,支持各种类型流式输出
    LogStream& operator<<(bool v);

    LogStream& operator<<(short);
    LogStream& operator<<(unsigned short);
    LogStream& operator<<(int);
    LogStream& operator<<(unsigned int);
    LogStream& operator<<(long);
    LogStream& operator<<(unsigned long);
    LogStream& operator<<(long long);
    LogStream& operator<<(unsigned long long);

    LogStream& operator<<(const void*);   // 指针(打印为十六进制地址)
    LogStream& operator<<(float);
    LogStream& operator<<(double);

    LogStream& operator<<(char v) {
        buffer_.append(&v, 1);
        return *this;
    }

    LogStream& operator<<(const char* str) {
        if (str) {
            buffer_.append(str, strlen(str));
        } else {
            buffer_.append("(null)", 6);
        }
        return *this;
    }

    LogStream& operator<<(const std::string& v) {
        buffer_.append(v.c_str(), v.size());
        return *this;
    }

    // 直接拿 Buffer 内部数据(给后端用)
    void append(const char* data, int len) { buffer_.append(data, len); }
    const Buffer& buffer() const { return buffer_; }
    void resetBuffer() { buffer_.reset(); }


private:
    template<typename T>
    void formatInteger(T);

    Buffer buffer_;

    // 64 位整数最长 20 位,加正负号、终止符等保险给 32
    static const int kMaxNumericSize = 32;
};















}//namespace lim