#ifndef __QNX__
  #error not on QNX
#endif

#define _QNX_SOURCE 1

#include <process.h>
#include <sys/neutrino.h>
#include <stdio.h>
#include <stdlib.h>
#include <sched.h>
#include <pthread.h>
#include <errno.h>

static volatile long toId;
static volatile long fromId;

void marker() {
}

// not needed on QNX
void deboost() {}
void registerPreemptionHook() {}

void setFromId() {
  fromId = gettid();
}

void setToId() {
  toId = gettid();
}

void singleCoreOnly() {
  int runmask = 0x1;
  ThreadCtl(_NTO_TCTL_RUNMASK, &runmask);
}

static inline long yield(long id) {
  if (!id) {
    perror("could not find thread\n");
    exit(2);
  }
  int result = SchedSet(0, id, SCHED_ADJTOHEAD, NULL);
  if (result) {
    printf("SchedSet returned ERRNO %d\n", errno);
    exit(9);
  }
  return result;
}

void yieldTo() {
  yield(toId);
}

void yieldBack() {
  yield(fromId);
}