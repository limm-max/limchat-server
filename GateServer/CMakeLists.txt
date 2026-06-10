cmake_minimum_required(VERSION 3.10)
project(GateServer
        VERSION 0.1.0
        DESCRIPTION "Gate Server of a chat program"
        LANGUAGES CXX)

#设置编程语言标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


# # ----------- 头文件搜索路径 -----------
# # PROJECT_SOURCE_DIR：CMake内置变量，项目根目录的绝对路径
# # 自定义库头文件
# include_directories(
#     ${PROJECT_SOURCE_DIR}/include

# )


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

add_executable(GateServer           
    GateServer.cc
    src/CServer.cc
    src/HttpConnection.cc
    src/LogicSystem.cc
    generated/verify.pb.cc 
    generated/verify.grpc.pb.cc
    src/ConfigMgr.cc
)

target_include_directories(GateServer PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/generated)

target_link_libraries(GateServer PRIVATE
    nlohmann_json::nlohmann_json     
    Boost::headers         
    Threads::Threads
    lim_base
    gRPC::grpc++ 
    protobuf::libprotobuf
    hiredis)

add_subdirectory(base)  