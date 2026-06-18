#pragma once

#include "Noncopyable.h"

#include <string.h>      // memcpy
#include <strings.h>     // bzero
#include <string>

//-------------------------------
//缓冲区模板类
//-------------------------------
namespace lim{
namespace detail{

// 两种缓冲区大小:
// - 小缓冲(4KB):前端单条日志暂存
// - 大缓冲(4MB):后端批量落盘暂存(Day 7 异步日志会用)
const int kSmallBuffer=4000;
const int kLargeBuffer=4000*1000;

// 模板类 FixedBuffer:固定大小的字节数组缓冲区
// 模板参数 SIZE 在编译期确定,所以 data_ 就是栈/对象内嵌数组
template<int SIZE>
class FixedBuffer:Noncopyable{
public:
    FixedBuffer():cur_(data_){
        setCookie(cookieStart);
    }

    ~FixedBuffer(){
        setCookie(cookieEnd);
    }

    //添加新数据到缓冲区里
    void append(const char* buf,size_t len){
        if(static_cast<size_t>(avail())>len){
            memcpy(cur_,buf,len);
            cur_+=len;
        }
    }

    const char* data() const {return data_;}

    int length() const{return static_cast<int>(cur_-data_);}

    char* current(){return cur_;}

    int avail() const{return static_cast<int>(end()-cur_);}

    // 手动推进写指针(snprintf 写完后用)
    void add(size_t len){cur_+=len;}

    //重置缓冲罐
    void reset(){cur_=data_;}

    // 把整个缓冲区清零(调试用).::从全局空间中找系统及bzero而非递归
    void bzero() { ::bzero(data_, sizeof data_); }

    // 把内容转成 std::string(主要给测试用)
    std::string toString() const { return std::string(data_, length()); }


private:
    //计算data_数组末尾位置指针
    const char* end() const{return data_+ sizeof data_ ;}

    // cookie 是用于崩溃时识别"这是个日志缓冲"的标识函数指针(muduo 的小技巧)
    //后续服务器崩溃的时候，可以通过这个标识找到对应对象
    // 不是核心功能,先简化处理
    void setCookie(void(*cookie)()){cookie_=cookie;}
    static void cookieStart() {}
    static void cookieEnd() {}

    char  data_[SIZE];          // 关键:对象内嵌的固定大小数组。这跟 char* data_ = new char[SIZE] 完全不同
    char* cur_;                 // 当前写指针
    void  (*cookie_)();         // 函数指针,先忽略
};
}//namespace detail
}//namespace lim