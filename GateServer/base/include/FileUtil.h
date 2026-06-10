#pragma once

#include "Noncopyable.h"

#include <stdio.h>
#include <string>

namespace lim {
namespace FileUtil {

// 一个简单的"非线程安全"文件追加写入类
// 单个 LogFile 实例独占使用,不需要内部加锁

class AppendFile : Noncopyable {
public:
    explicit AppendFile(const std::string& filename);
    ~AppendFile();

    // 追加写入 logline[0..len) 到文件
    void append(const char* logline, size_t len);

    // 强制把缓冲区数据写到 OS(不一定写到磁盘)
    void flush();

    // 已写入的总字节数
    off_t writtenBytes() const { return writtenBytes_; }

private:
    // 不加锁的低层 write(性能更高)
    size_t write(const char* logline, size_t len);

    FILE*  fp_;                 // C 风格文件指针
    char   buffer_[64 * 1024];  // 64KB 用户态缓冲(setbuffer 用)
    off_t  writtenBytes_;       // 已写入字节数(用于 LogFile 判断滚动)
};



} // namespace FileUtil
} // namespace lim