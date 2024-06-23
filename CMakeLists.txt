cmake_minimum_required(VERSION 3.28)
project(libdatachannel_test C)

set(CMAKE_C_STANDARD 17)

add_executable(libdatachannel_test main.c)
target_link_libraries(libdatachannel_test datachannel)
