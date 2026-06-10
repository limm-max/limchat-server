#include "LogStream.h"

#include<algorithm>
#include<stdio.h>
#include<string.h>
#include <cstdint>
namespace lim{
namespace detail{

// muduo 的高效整数转字符串
// 核心:查表法,避免 snprintf 的格式化开销
// 比 snprintf("%d", x) 快 5-10 倍

//使用查表法，将整数每一位变成字符
const char digits[]="9876543210123456789";
const char* zero=digits+9;

//buf输出型参数
template<typename T>
size_t convert(char buf[],T value){
    T i=value;
    char* p=buf;

    do{
        int lsd=static_cast<int>(i%10);
        i/=10;
        *p++=zero[lsd];
    }while(i!=0);

    if(value<0){
        *p++='-';

    }
    *p='\0';

    std::reverse(buf,p);// 因为是从低位往高位写的,要反转
    return p-buf;


}

//指针转十六进制字符串,uintptr_t,它的长度被保证与当前系统的指针长度完全一致
size_t convertHex(char buf[],uintptr_t  value){
    static const char digitsHex[] = "0123456789ABCDEF";
    uintptr_t i = value;
    char* p = buf;
    do {
        int lsd = static_cast<int>(i % 16);
        i /= 16;
        *p++ = digitsHex[lsd];
    } while (i != 0);

    *p = '\0';
    std::reverse(buf, p);
    return p - buf;
}



}//namespace details

// 通用整数格式化:先在临时缓冲格式化,再拷贝到 buffer_
template <typename T>
void LogStream::formatInteger(T v) {
    if (buffer_.avail() >= kMaxNumericSize) {
        size_t len = detail::convert(buffer_.current(), v);
        buffer_.add(len);
    }
}

// 实例化各种整数类型的 operator
LogStream& LogStream::operator<<(bool v) {
    buffer_.append(v ? "1" : "0", 1);
    return *this;
}

//这俩是将其转为int再格式化
LogStream& LogStream::operator<<(short v)              { return *this << static_cast<int>(v); }
LogStream& LogStream::operator<<(unsigned short v)     { return *this << static_cast<unsigned int>(v); }

LogStream& LogStream::operator<<(int v)                { formatInteger(v); return *this; }
LogStream& LogStream::operator<<(unsigned int v)       { formatInteger(v); return *this; }
LogStream& LogStream::operator<<(long v)               { formatInteger(v); return *this; }
LogStream& LogStream::operator<<(unsigned long v)      { formatInteger(v); return *this; }
LogStream& LogStream::operator<<(long long v)          { formatInteger(v); return *this; }
LogStream& LogStream::operator<<(unsigned long long v) { formatInteger(v); return *this; }

// 指针:打印为十六进制
LogStream& LogStream::operator<<(const void* p) {
    uintptr_t v = reinterpret_cast<uintptr_t>(p);
    if (buffer_.avail() >= kMaxNumericSize) {
        char* buf = buffer_.current();
        buf[0] = '0';
        buf[1] = 'x';
        size_t len = detail::convertHex(buf + 2, v);
        buffer_.add(len + 2);
    }
    return *this;
}

// 浮点数:暂用 snprintf(够用)
LogStream& LogStream::operator<<(float v) {
    *this << static_cast<double>(v);
    return *this;
}

LogStream& LogStream::operator<<(double v) {
    if (buffer_.avail() >= kMaxNumericSize) {
        int len = snprintf(buffer_.current(), kMaxNumericSize, "%.12g", v);
        buffer_.add(len);
    }
    return *this;
}

} // namespace lim