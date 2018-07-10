cmake_minimum_required(VERSION 3.10)
project(server C)

set(CMAKE_C_STANDARD 99)
set(MAIN ex05/server_main.c)
set(MYLIB my_lib)
set(
        SOURCE_FILES
        ${MAIN}
        ${MYLIB}/my_net.h
        ${MYLIB}/tcp_client.c
        ${MYLIB}/tcp_server.c
        ${MYLIB}/common.c
        ${MYLIB}/udp_server.c
        ${MYLIB}/udp_client.c
        ex05/client_util.h
        ex05/client_util.c
        ex05/server_util.c
        ex05/server_util.h
        ex05/setting.h)

include_directories(my_lib)

add_library(my_library my_lib/tcp_client.c my_lib/tcp_server.c my_lib/common.c
        my_lib/udp_client.c my_lib/udp_server.c ex05/client_util.c ex05/server_util.c
        )

add_executable(server ${SOURCE_FILES})