cmake_minimum_required(VERSION 3.28)
project(libdatachannel_test C CXX)

set(CMAKE_C_STANDARD 17)
set(CMAKE_CXX_STANDARD 17)

set(NO_WEBSOCKET ON)
set(NO_MEDIA ON)
set(NO_TESTS ON)
set(NO_EXAMPLES ON)
add_subdirectory(libdatachannel)
include_directories(libdatachannel/include)

add_executable(libdatachannel_test main.c)
target_link_libraries(libdatachannel_test PRIVATE datachannel-static)
