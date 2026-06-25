// ============================================================
// Timestamp.h
//一个 int64_t 包起来的时间点，提供 now()、toString()、addTime()、<、==
// 时间戳类，表示从 Unix Epoch (1970-01-01) 至今的微秒数。
// 用 int64_t 存储，可使用约 29 万年。
//
// 使用示例：
//     Timestamp now = Timestamp::now();
//     std::cout << now.toString() << std::endl;
//
// 设计灵感：muduo::Timestamp
// ============================================================

#pragma once

#include <cstdint>      // int64_t
#include <string>

namespace lim {

class Timestamp {
public:
    // ----- 构造函数 -----

    // 默认构造：值为 0（无效时间戳）
    Timestamp() : microSecondsSinceEpoch_(0) {}

    // 显式构造：从一个 int64_t 微秒数构造
    explicit Timestamp(int64_t microSecondsSinceEpoch)
        : microSecondsSinceEpoch_(microSecondsSinceEpoch) {}

    // ----- 静态工厂函数 -----

    // 获取当前时间戳
    static Timestamp now();

    // 返回一个无效时间戳（值为 0）
    static Timestamp invalid() {
        return Timestamp();
    }

    // ----- 获取数据 -----

    // 返回原始微秒数
    int64_t microSecondsSinceEpoch() const {
        return microSecondsSinceEpoch_;
    }

    // 返回秒数（去掉小数部分）
    time_t secondsSinceEpoch() const {
        return static_cast<time_t>(
            microSecondsSinceEpoch_ / kMicroSecondsPerSecond);
    }

    // ----- 转字符串（用于日志） -----

    // 简单格式："1778155200.123456"（秒.微秒）
    std::string toString() const;

    // 易读格式："2026-05-07 12:00:00.123456"
    std::string toFormattedString(bool showMicroseconds = true) const;

    // ----- 判断 -----

    // 是否是有效时间戳
    bool valid() const {
        return microSecondsSinceEpoch_ > 0;
    }

    // ----- 静态常量 -----

    static const int kMicroSecondsPerSecond = 1000 * 1000;

private:
    int64_t microSecondsSinceEpoch_;  // 从 epoch 开始的微秒数
};

// ----- 运算符重载（用于比较两个 Timestamp） -----

inline bool operator<(Timestamp lhs, Timestamp rhs) {
    return lhs.microSecondsSinceEpoch() < rhs.microSecondsSinceEpoch();
}

inline bool operator==(Timestamp lhs, Timestamp rhs) {
    return lhs.microSecondsSinceEpoch() == rhs.microSecondsSinceEpoch();
}

// ----- 时间戳加上一定秒数（用于定时器） -----

inline Timestamp addTime(Timestamp timestamp, double seconds) {
    int64_t delta = static_cast<int64_t>(
        seconds * Timestamp::kMicroSecondsPerSecond);
    return Timestamp(timestamp.microSecondsSinceEpoch() + delta);
}

} // namespace lim