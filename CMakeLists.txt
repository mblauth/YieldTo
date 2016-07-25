cmake_minimum_required(VERSION 3.5)
project(YieldTo C)

include_directories(${CMAKE_CURRENT_BINARY_DIR})

set(BarrierImplementation pthread-barrier.c)
set(YieldToImplementation yield-posix.c)
set(SingleCoreOnlyImplementation singlecoreonly-posix.c)
set(LogImplementation simpleasciilog.c)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall")
if(PIKEOS)
  add_definitions(-DPIKEOS_POSIX_PSE52)
  add_definitions(-DPIKEOS_POSIX_SCHEDULER_HOOK=true) # Note that this requires a custom build of the PikeOS toolchain
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fno-builtin -fno-unit-at-a-time -nostdinc -fno-strict-aliasing -msoft-float -O3")
  set(CMAKE_FIND_ROOT_PATH "${PIKEOS_ROOT}")
  set(POSIXLIBS ${CMAKE_FIND_ROOT_PATH}/target/ppc/e500/bposix/lib/)
  set(CMAKE_C_COMPILER ${CMAKE_FIND_ROOT_PATH}/cdk/ppc/e500/bin/ppc_e500-gcc)
  set(CMAKE_LINKER ${CMAKE_FIND_ROOT_PATH}/cdk/ppc/e500/bin/ppc_e500-ld)
  set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
  set(CMAKE_FIND_ROOT_PATH_NIDE_LIBRARY ONLY)
  set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
  include_directories(SYSTEM ${CMAKE_FIND_ROOT_PATH}/target/ppc/e500/bposix/include)
  link_directories(SYSTEM ${CMAKE_FIND_ROOT_PATH}/target/ppc/e500/lib
                   SYSTEM ${CMAKE_FIND_ROOT_PATH}/target/ppc/e500/bposix/os/up)
  set(BarrierImplementation polling-barrier.c) # not so POSIX…
  set(YieldToImplementation yieldTo-PikeOS.c pikeos-state.c) # boost-posix.c is also supported
  set(LogImplementation pikeos-benchmarklog.c)
  add_definitions(-DHAVE_PREEMPTION_HOOK)
elseif(QNX)
  set(QNX_TARGET $ENV{QNX_TARGET})
  if(NOT EXISTS ${QNX_TARGET})
    MESSAGE(FATAL_ERROR "QNX_TARGET incorrect: ${QNX_TARGET}")
  endif()
  set(QNX_HOST $ENV{QNX_HOST})
  if(NOT EXISTS ${QNX_HOST})
    MESSAGE(FATAL_ERROR "QNX_HOST incorrect: ${QNX_HOST}")
  endif()

  set(CMAKE_FIND_ROOT_PATH ${QNX_TARGET}/usr/include)
  set(POSIXLIBS ${CMAKE_FIND_ROOT_PATH}/target/qnx6/x86/usr/lib/)
  set(CMAKE_C_COMPILER ${QNX_HOST}/usr/bin/i486-pc-nto-qnx6.6.0-gcc)
  set(CMAKE_LINKER ${QNX_HOST}/usr/bin/i486-pc-nto-qnx6.6.0-ld)

  set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
  set(CMAKE_FIND_ROOT_PATH_NIDE_LIBRARY ONLY)
  set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
  include_directories(SYSTEM ${CMAKE_FIND_ROOT_PATH})
  link_directories(SYSTEM ${POSIXLIBS})
  set(BarrierImplementation pthread-barrier.c)
  set(YieldToImplementation yieldTo-qnx.c)
  unset(SingleCoreOnlyImplementation)
elseif(${CMAKE_SYSTEM} MATCHES "Linux")
  set(BarrierImplementation pthread-barrier.c)
  set(YieldToImplementation yield-posix.c)
endif()

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c99") # PikeOS toolchain does not support c11
add_definitions(-D_GNU_SOURCE)

add_executable(YieldTo yieldTo.c error.c histogram.c statehandling.c threadmanagement.c posix-scheduling.c
               ${LogImplementation} ${BarrierImplementation} ${YieldToImplementation} ${SingleCoreOnlyImplementation})

if(PIKEOS)
  set (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -T${CMAKE_FIND_ROOT_PATH}/target/ppc/e500/scripts/app-ld-script \
    ${CMAKE_FIND_ROOT_PATH}/target/ppc/e500/bposix/os/up/crt0.o \
    ${CMAKE_FIND_ROOT_PATH}/target/ppc/e500/bposix/os/up/h_scheduler.o \
    ${POSIXLIBS}/pse52.o \
    ${POSIXLIBS}/pse52_ext2fs.o \
    ${POSIXLIBS}/pse52_provfs.o \
    ${POSIXLIBS}/pse52_ufs.o \
    ${POSIXLIBS}/rootfs.a")
  target_link_libraries(YieldTo c sbuf m vm p4 gcc stand)
  if(PIKEOS_TARGET)
    install(PROGRAMS ${CMAKE_BINARY_DIR}/YieldTo DESTINATION ${PIKEOS_TARGET} RENAME posix)
  endif()
elseif(QNX)
  target_link_libraries(YieldTo ${CMAKE_THREAD_LIBS_INIT})
else()
  target_link_libraries(YieldTo pthread ${CMAKE_THREAD_LIBS_INIT})
endif()



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