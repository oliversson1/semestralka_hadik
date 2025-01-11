cmake_minimum_required(VERSION 3.10)
project(semestralka_WSL C)

set(CMAKE_C_STANDARD 11)

add_executable(server server.c)

add_executable(client client.c)

add_custom_target(cleanup
        COMMAND rm -f server client
)
