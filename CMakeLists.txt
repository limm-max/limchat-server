cmake_minimum_required(VERSION 3.10)
project(lim-chat
        VERSION 0.1.0
        DESCRIPTION "A chat app program"
        LANGUAGES CXX)

#设置编程语言标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


# ----------- 头文件搜索路径 -----------
# PROJECT_SOURCE_DIR：CMake内置变量，项目根目录的绝对路径
# 自定义库头文件
include_directories(
    ${PROJECT_SOURCE_DIR}/mysqlpool/include
    ${PROJECT_SOURCE_DIR}/redispool/include
)


# ----------- 输出路径 -----------
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)   #可执行文件
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib)   #动态库
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib)   #静态库




add_subdirectory(GateServer)