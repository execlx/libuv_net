# 添加库
add_library(libuv_net SHARED
    client.cpp
    server.cpp
    session.cpp
    message.cpp
    thread_pool.cpp
)

# 设置包含目录
target_include_directories(libuv_net
    PUBLIC
    ${CMAKE_SOURCE_DIR}/include
    ${fmt_INCLUDE_DIRS}
)

# 链接库
target_link_libraries(libuv_net
    PRIVATE
    fmt::fmt
)

# 安装
install(TARGETS libuv_net
    EXPORT libuv_netTargets
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
    INCLUDES DESTINATION include
)

install(DIRECTORY ${CMAKE_SOURCE_DIR}/include/
    DESTINATION include
) 