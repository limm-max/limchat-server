#include "Condition.h"

#include<errno.h>
#include<time.h>
#include <cstdint>  
namespace lim{

//裸露的pthread_cond_wait直接解锁，但是因为使用MutexLock封装了，所以需要额外操作
void Condition::wait(){
    pid_t preHolder=mutex_.holder_;
    mutex_.holder_=0;
    pthread_cond_wait(&cond_,mutex_.getPthreadMutex());
    mutex_.holder_=preHolder;   //排到队了，并拿到锁了
}

bool Condition::waitForSeconds(double seconds){
    struct timespec abstime;
    clock_gettime(CLOCK_REALTIME,&abstime);

    // 计算"几秒后"的绝对时间点
    const int64_t kNanoSecondsPerSecond = 1000000000;
    int64_t nanoseconds = static_cast<int64_t>(seconds * kNanoSecondsPerSecond);
    abstime.tv_sec += static_cast<time_t>((abstime.tv_nsec + nanoseconds) / kNanoSecondsPerSecond);
    abstime.tv_nsec = static_cast<long>((abstime.tv_nsec + nanoseconds) % kNanoSecondsPerSecond);

    pid_t prevHolder=mutex_.holder_;
    mutex_.holder_=0;
    int ret=pthread_cond_timedwait(&cond_,mutex_.getPthreadMutex(),&abstime);
    mutex_.holder_ = prevHolder;
    return ret==ETIMEDOUT;



}

}//namespace lim