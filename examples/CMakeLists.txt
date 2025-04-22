add_executable(echo_server echo_server.cpp)
target_link_libraries(echo_server PRIVATE libuv_net)

add_executable(echo_client echo_client.cpp)
target_link_libraries(echo_client PRIVATE libuv_net) 