cmake_minimum_required(VERSION 3.10)
project(libuv_net VERSION 1.0.0)

# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 设置 vcpkg 工具链文件
if(DEFINED ENV{VCPKG_ROOT} AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake"
        CACHE STRING "")
endif()

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
find_package(fmt CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(Threads REQUIRED)
find_package(libuv CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(Protobuf CONFIG REQUIRED)

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

# 添加头文件
set(HEADERS
    include/libuv_net/client.hpp
    include/libuv_net/server.hpp
    include/libuv_net/session.hpp
    include/libuv_net/thread_pool.hpp
    include/libuv_net/message.hpp
    include/libuv_net/json_interceptor.hpp
    include/libuv_net/protobuf_interceptor.hpp
)

# 创建库
if(WIN32)
    add_library(libuv_net SHARED ${SOURCES} ${HEADERS})
    set_target_properties(libuv_net PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/Debug"
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/Debug"
        ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/Debug"
        PREFIX ""
        IMPORT_PREFIX ""
        WINDOWS_EXPORT_ALL_SYMBOLS ON
    )
else()
    add_library(libuv_net SHARED ${SOURCES} ${HEADERS})
endif()

# 链接库
target_link_libraries(libuv_net
    PRIVATE
    fmt::fmt
    spdlog::spdlog
    libuv::uv
    Threads::Threads
    nlohmann_json::nlohmann_json
    protobuf::libprotobuf
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
    nlohmann_json::nlohmann_json
    protobuf::libprotobuf
)

add_executable(server_test tests/server_test.cpp)
target_link_libraries(server_test
    PRIVATE
    libuv_net
    fmt::fmt
    spdlog::spdlog
    libuv::uv
    Threads::Threads
    nlohmann_json::nlohmann_json
    protobuf::libprotobuf
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
    FILES_MATCHING PATTERN "*.hpp"
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