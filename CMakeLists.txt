cmake_minimum_required(VERSION 3.10)
project(libuv_net)

# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 查找必要的包
find_package(fmt REQUIRED)
find_package(spdlog REQUIRED)
find_package(Threads REQUIRED)

# 查找 libuv
find_library(LIBUV_LIBRARY uv)
if(NOT LIBUV_LIBRARY)
    message(FATAL_ERROR "libuv library not found")
endif()

# 添加头文件目录
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${fmt_INCLUDE_DIRS}
    ${spdlog_INCLUDE_DIRS}
)

# 添加源文件
set(SOURCES
    src/client.cpp
    src/server.cpp
    src/session.cpp
    src/thread_pool.cpp
)

# 创建库
add_library(libuv_net SHARED ${SOURCES})

# 链接库
target_link_libraries(libuv_net
    fmt::fmt
    spdlog::spdlog
    ${LIBUV_LIBRARY}
    Threads::Threads
)

# 添加测试程序
add_executable(client_test tests/client_test.cpp)
target_link_libraries(client_test
    libuv_net
    fmt::fmt
    spdlog::spdlog
    ${LIBUV_LIBRARY}
    Threads::Threads
)

add_executable(server_test tests/server_test.cpp)
target_link_libraries(server_test
    libuv_net
    fmt::fmt
    spdlog::spdlog
    ${LIBUV_LIBRARY}
    Threads::Threads
)

# 安装
install(TARGETS libuv_net
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
)

install(DIRECTORY include/
    DESTINATION include
) 