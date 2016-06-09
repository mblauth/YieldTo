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

static volatile long toId;
static volatile long fromId;

inline void marker() {
  syscall(SYS_gettid);
}

// not needed on Linux
void deboost() {}
void registerPreemptionHook() {}

inline void setFromId() {
  fromId = syscall(SYS_gettid);
}

inline void setToId() {
  toId = syscall(SYS_gettid);
}

static inline long yield(long id) {
  if (!id) {
    perror("could not find thread\n");
    exit(2);
  }
  return syscall(__NR_sched_yieldTo, id);
}

inline void yieldTo() {
  yield(toId);
}

inline void yieldBack() {
  yield(fromId);
}