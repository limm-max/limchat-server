# ========================================
# base 模块：基础设施
# ========================================

set(BASE_SOURCES
    src/Timestamp.cc
    src/Thread.cc
    src/CurrentThread.cc
    src/Condition.cc            
    src/CountDownLatch.cc       
    src/LogStream.cc
    src/FileUtil.cc
    src/LogFile.cc
    src/Logger.cc
    src/AsyncLogging.cc
    )

add_library(lim_base STATIC ${BASE_SOURCES})

target_include_directories(lim_base PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Thread 用到了 pthread,必须链接
target_link_libraries(lim_base PUBLIC pthread)