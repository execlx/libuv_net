cmake_minimum_required(VERSION 3.1)
project(libuv_net_example)

# 设置编译器
if(MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /EHsc")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MDd")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MD")
endif()

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# 设置 Qt 路径
set(CMAKE_PREFIX_PATH "D:/Qt/Qt5.12.12/5.12.12/msvc2015_64")

# 设置 libuv_net 路径
set(libuv_net_DIR "${CMAKE_SOURCE_DIR}/../build/lib")

# 查找 Qt5
find_package(Qt5 COMPONENTS Core Widgets REQUIRED)

# 查找 libuv_net 和 Qt 包装器
find_package(libuv_net REQUIRED)
find_package(libuv_net_qt REQUIRED)

# 添加可执行文件
add_executable(libuv_net_example
    main.cpp
    mainwindow.cpp
    mainwindow.h
    mainwindow.ui
)

# 设置包含目录
target_include_directories(libuv_net_example
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${Qt5Widgets_INCLUDE_DIRS}
    ${CMAKE_SOURCE_DIR}/../build/include
    ${CMAKE_SOURCE_DIR}/../qt_wrapper/include
)

# 链接库
target_link_libraries(libuv_net_example
    PRIVATE
    Qt5::Core
    Qt5::Widgets
    libuv_net
    libuv_net_qt
)

# 设置输出目录
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib) 