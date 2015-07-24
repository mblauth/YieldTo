#ifndef __linux
  #error not on Linux
#endif

#include <sys/syscall.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sched.h>
#include <pthread.h>

#include "yieldTo.h"

#ifndef __NR_sched_yieldTo
#define __NR_sched_yieldTo 323
#endif

inline void marker() {
  syscall(SYS_gettid);
}

void registerPreemptionHook() {} // not needed on Linux

inline void setToId() {
  toId = syscall(SYS_gettid);
}

void singleCoreOnly() {
  cpu_set_t cpuSet;
  CPU_ZERO(&cpuSet);
  CPU_SET(0, &cpuSet);

  int s = pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuSet);
  if (s != 0) {
    perror("could not set affinity");
    exit(1);
  }
}

inline long yieldTo(long const id) {
  if (!id) {
    perror("could not find thread\n");
    exit(2);
  }
  return syscall(__NR_sched_yieldTo, id);
}