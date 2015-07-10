cmake_minimum_required(VERSION 3.2)
project(YieldTo C)

if(PIKEOS)
set(CMAKE_FIND_ROOT_PATH /opt/pikeos-3.5)
set(CMAKE_C_COMPILER ${CMAKE_FIND_ROOT_PATH}/cdk/ppc/e500/bin/ppc_e500-gcc)
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_NIDE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
include_directories(SYSTEM ${CMAKE_FIND_ROOT_PATH}/target/ppc/e500/bposix/include )
endif()


set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c99") # PikeOS toolchain does not support c11
set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
add_definitions(-D_GNU_SOURCE)

find_package(Threads REQUIRED)

add_executable(YieldTo yieldTo.c)

target_link_libraries(YieldTo ${CMAKE_THREAD_LIBS_INIT})

find_program(SCP_EXECUTABLE scp)
if(WANT_SCP)
  if(SCP_EXECUTABLE)
    if(!SSH_PORT)
      set(SSH_PORT 22)
    endif()
    if(TEST_HOST AND IDENTITY_FILE)
      add_custom_command(TARGET YieldTo POST_BUILD
                         COMMAND ${SCP_EXECUTABLE} ARGS -i ${IDENTITY_FILE} -P ${SSH_PORT} YieldTo ${TEST_HOST}:./)
    else()
      message(SEND_ERROR "TEST_HOST or IDENTITY_FILE not set")
    endif()
  else()
    message(SEND_ERROR "scp not found")
  endif()
endif()