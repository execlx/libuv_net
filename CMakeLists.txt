cmake_minimum_required(VERSION 3.15)
project(libuv_net VERSION 1.0.0 LANGUAGES CXX)

# 设置 C++14 标准
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 查找依赖包
find_package(Threads REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBUV REQUIRED libuv)
find_package(spdlog REQUIRED)

# 包含目录
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${LIBUV_INCLUDE_DIRS}
)

# 源文件
file(GLOB_RECURSE SOURCES 
    "src/*.cpp"
)

# 创建库
add_library(libuv_net SHARED ${SOURCES})

# 链接库
target_link_libraries(libuv_net
    PRIVATE
    ${LIBUV_LIBRARIES}
    Threads::Threads
    spdlog::spdlog
)

# 创建测试程序
add_executable(server_test tests/server_test.cpp)
target_link_libraries(server_test PRIVATE libuv_net)

add_executable(client_test tests/client_test.cpp)
target_link_libraries(client_test PRIVATE libuv_net)

# 安装规则
install(TARGETS libuv_net
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
)

install(DIRECTORY include/
    DESTINATION include
) 