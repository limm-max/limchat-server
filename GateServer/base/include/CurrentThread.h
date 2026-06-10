/*
获取当前线程内核id，由于gettid系统调用切换内核态麻烦，使用这个来进行线程局部缓存
//一组用 __thread（TLS）缓存当前线程 tid 的函数：tid()、tidString()、isMainThread()
*/

#pragma once

namespace lim{
namespace CurrentThread{

//这两个变量是"线程局部存储",每个线程都有自己的副本,__thread关键字，用于定义线程局部变量，每个线程会拥有一份该变量的独立拷贝。
extern __thread int t_cachedTid;        // 当前线程的 tid 缓存
extern __thread char t_tidString[32];   // tid 的字符串形式(打日志用)
extern __thread int t_tidStringLength;  // tid 字符串长度
extern __thread const char* t_threadName; // 当前线程的名字

void cacheTid();

//如果没有缓存，那就进行系统调用，否则直接返回缓存
inline int tid(){
    // __builtin_expect 是 GCC 内置,告诉编译器"几乎肯定 t_cachedTid != 0"
    // 让编译器优化分支预测,这是性能敏感代码常见手法
    if(__builtin_expect(t_cachedTid==0,0)){
        cacheTid();
    }
    return t_cachedTid;
}

inline const char* tidString(){ return t_tidString;}
inline int tidStringLength() { return t_tidStringLength;}
inline const char* name() { return t_threadName;}
}// namespace CurrentThread
}// namespace lim