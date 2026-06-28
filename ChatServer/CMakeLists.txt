cmake_minimum_required(VERSION 3.10)
project(ChatServer1
        VERSION 0.1.0
        DESCRIPTION "Chat Server of a chat program"
        LANGUAGES CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
#设置编程语言标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ----------- 输出路径 -----------
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)   #可执行文件
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib)   #动态库
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib)   #静态库
set(CMAKE_FIND_USE_SYSTEM_ENVIRONMENT_PATH OFF)

find_package(nlohmann_json REQUIRED) 
find_package(Boost REQUIRED)                                
find_package(Threads REQUIRED)             
find_package(Protobuf REQUIRED)
find_package(gRPC REQUIRED)

add_executable(ChatServer
    src/CServer.cc
    src/CSession.cc
    src/LogicSystem.cc
    src/ConfigMgr.cc
    src/IOServicePool.cpp
    src/MysqlDao.cc
    src/MysqlMgr.cc
    src/MysqlPool.cc
    src/RedisConPool.cc
    src/RedisMgr.cc
    src/UserMgr.cc

    ChatServer.cc
)

target_include_directories(ChatServer PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/generated)

target_link_libraries(ChatServer PRIVATE
    nlohmann_json::nlohmann_json     
    Boost::headers         
    Threads::Threads
    lim_base
    gRPC::grpc++ 
    protobuf::libprotobuf
    hiredis
    mysqlcppconn)

add_subdirectory(base)  