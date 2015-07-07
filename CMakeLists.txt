cmake_minimum_required(VERSION 3.2)
project(YieldTo C)


set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c11")
set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
add_definitions(-D_GNU_SOURCE)

find_package(Threads REQUIRED)

set(SOURCE_FILES main.c)
add_executable(YieldTo ${SOURCE_FILES})
target_link_libraries(YieldTo ${CMAKE_THREAD_LIBS_INIT})