#include "FileUtil.h"

#include <assert.h>
#include <stdio.h>

namespace lim {
namespace FileUtil {

AppendFile::AppendFile(const std::string& filename)
    : fp_(::fopen(filename.c_str(), "ae")),  // 'a'=追加, 'e'=O_CLOEXEC
      writtenBytes_(0) {
    assert(fp_ != nullptr);

    // 自定义 fp_ 的用户态缓冲区为我们的 buffer_(64KB)
    // 默认 stdio 缓冲只有 4KB,放大后减少 write 系统调用次数
    ::setbuffer(fp_, buffer_, sizeof buffer_);
}

AppendFile::~AppendFile() {
    ::fclose(fp_);   // fclose 会自动 fflush
}

void AppendFile::append(const char* logline, size_t len) {
    size_t written = 0;
    while (written != len) {
        size_t remain = len - written;
        size_t n = write(logline + written, remain);    //外部缓冲区写入内部缓冲区？
        if (n != remain) {
            int err = ferror(fp_);
            if (err) {
                fprintf(stderr, "AppendFile::append() failed errno=%d\n", err);
                break;
            }
        }
        written += n;
    }
    writtenBytes_ += written;
}

void AppendFile::flush() {
    ::fflush(fp_);
}

size_t AppendFile::write(const char* logline, size_t len) {
    // fwrite_unlocked 是 fwrite 的非线程安全版本(不加 FILE 内部锁)
    // 我们外部有 LogFile 的锁,不需要 stdio 自己的锁
    // 性能比 fwrite 高 30%+
    return ::fwrite_unlocked(logline, 1, len, fp_);
}

} // namespace FileUtil
} // namespace lim