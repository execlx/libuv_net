cmake_minimum_required(VERSION 3.10)
project(libuv_net VERSION 1.0.0)

# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Windows 特定设置
if(WIN32)
    # 添加 Windows 特定的定义
    add_definitions(-D_WIN32_WINNT=0x0601)
    add_definitions(-DWIN32_LEAN_AND_MEAN)
    add_definitions(-DNOMINMAX)
    
    # 设置 Windows SDK 版本
    set(CMAKE_SYSTEM_VERSION 10.0.22631.0)
endif()

# 设置编译选项
if(MSVC)
    add_compile_options(/W4)
    # 移除 /WX 选项，允许警告
    # add_compile_options(/WX)
else()
    add_compile_options(-Wall -Wextra -Wpedantic -Werror)
endif()

# 查找必要的包
find_package(fmt REQUIRED)
find_package(spdlog REQUIRED)
find_package(Threads REQUIRED)
find_package(libuv REQUIRED)

# 添加头文件目录
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
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
    PRIVATE
    fmt::fmt
    spdlog::spdlog
    libuv::uv
    Threads::Threads
)

# Windows 特定链接
if(WIN32)
    target_link_libraries(libuv_net PRIVATE ws2_32)
endif()

# 添加测试程序
add_executable(client_test tests/client_test.cpp)
target_link_libraries(client_test
    PRIVATE
    libuv_net
    fmt::fmt
    spdlog::spdlog
    libuv::uv
    Threads::Threads
)

add_executable(server_test tests/server_test.cpp)
target_link_libraries(server_test
    PRIVATE
    libuv_net
    fmt::fmt
    spdlog::spdlog
    libuv::uv
    Threads::Threads
)

# 安装
install(TARGETS libuv_net
    EXPORT libuv_netTargets
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
    INCLUDES DESTINATION include
)

install(DIRECTORY include/
    DESTINATION include
)

install(EXPORT libuv_netTargets
    FILE libuv_netTargets.cmake
    NAMESPACE libuv_net::
    DESTINATION lib/cmake/libuv_net
)

# 生成和安装配置文件
include(CMakePackageConfigHelpers)
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/libuv_netConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/libuv_netConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/libuv_netConfig.cmake"
    INSTALL_DESTINATION lib/cmake/libuv_net
)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/libuv_netConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/libuv_netConfigVersion.cmake"
    DESTINATION lib/cmake/libuv_net
) 