cmake_minimum_required(VERSION 3.1)
project(libuv_net_qt)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# 查找 Qt5
find_package(Qt5 COMPONENTS Core REQUIRED)

# 查找 fmt 库
find_package(fmt REQUIRED)

# 添加库
add_library(libuv_net_qt SHARED
    src/qt_client.cpp
)

# 设置包含目录
target_include_directories(libuv_net_qt
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/include
    ${Qt5Core_INCLUDE_DIRS}
)

# 链接库
target_link_libraries(libuv_net_qt
    PRIVATE
    Qt5::Core
    libuv_net
    fmt::fmt
)

# 安装
install(TARGETS libuv_net_qt
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
)

install(DIRECTORY include/
    DESTINATION include
) 