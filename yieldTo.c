#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdbool.h>

#include "yieldTo.h"
#include "barrier.h"

#define Scheduling_Policy SCHED_RR
#define Realtime_Priority 63 // 63 is the maximum value on QNX
#define Background_Thread_Number 20
#define Load_Factor 10
#define YieldBack true

volatile bool yieldedTo = false;

static pthread_t tid[Background_Thread_Number];
static pthread_t to;

static volatile bool yieldedBack = false;
static volatile bool toFinished = false;

static void setupResources();
static void startThreads();
static void joinThreads();

int main(int argc, char *argv[]) {
  printf("launching yieldTo\n");
  setupResources();
  startThreads();
  registerPreemptionHook();
  waitAtBarrier();
  marker();
  while (!toFinished) {
    for (unsigned long k = 0; k < 0xffffff && !toFinished; k++)
      yieldedTo = false;
    yieldedTo = true;
    yieldTo();
    if (yieldedTo) printf("yieldTo failed\n");
    yieldedTo = false;

#if YieldBack
    if (yieldedBack) {
      printf("yieldBack worked\n");
      yieldedBack = false;
    } else printf("yieldBack failed\n");
#endif
  }
  joinThreads();
  destroyBarrier();
}

static void *toLogic(void *ignored) {
  UNUSED(ignored);
  setToId();
  waitAtBarrier();
  sched_yield();
  marker();
  for (int i = 0; i < Load_Factor; i++)
    for (unsigned long k = 0; k < 0xffffff; k++) {
      if (yieldedTo) {
        printf("yieldTo worked!\n");
        yieldedTo = false;

#if YieldBack
        yieldedBack = true;
        yieldBack();
        if (yieldedBack) printf("yieldBack failed!\n");
        yieldedBack = false;
#endif
      }
    }
  printf("yieldTo target finished execution\n");
  toFinished = true;
  return NULL;
}

static void startToThread() {
  pthread_create(&to, NULL, &toLogic, NULL);
}

static void *busy(void *ignored) {
  UNUSED(ignored);
  for (int i = 0; i < Load_Factor; i++)
    for (unsigned long k = 0; k < 0xffffff; k++) {
      if (yieldedTo) printf("yieldTo failed\n");
      yieldedTo = false;
#if YieldBack
      if (yieldedBack) printf("yieldBack failed\n");
      yieldedBack = false;
#endif
    }
  return NULL;
}

static void startBackgroundThreads() {
  printf("creating %i background threads...", Background_Thread_Number);
#if Scheduling_Policy == SCHED_FIFO || Scheduling_Policy == SCHED_RR
  pthread_attr_t attr;
  if (pthread_attr_init(&attr) != 0) {
    printf("thread attr init failure");
    exit(3);
  }
  if (pthread_attr_setschedpolicy(&attr, Scheduling_Policy)) {
    printf("could not set realtime policy\n");
    exit(4);
  }
  struct sched_param param = {.sched_priority = Realtime_Priority};
  if (pthread_attr_setschedparam(&attr, &param)) {
    printf("could not set realtime priority\n");
    exit(4);
  }
  for (int i = 0; i < Background_Thread_Number; i++)
    pthread_create(&tid[i], &attr, &busy, NULL);
  pthread_attr_destroy(&attr);
#else
  for (int i = 0; i < Background_Thread_Number; i++)
      pthread_create(&tid[i], NULL, &busy, NULL);
#endif
  printf("done\n");
}

static void startThreads() {
  startToThread();
  startBackgroundThreads();
}

static void joinBackgroundThreads() {
  for (int i = 0; i < Background_Thread_Number; i++)
    pthread_join(tid[i], NULL);
}

static void joinThreads() {
  pthread_join(to, NULL);
  joinBackgroundThreads();
}

static void setRealtimeParameters(pthread_t thread) {
#if Scheduling_Policy == SCHED_FIFO || Scheduling_Policy == SCHED_RR
  printf("setting realtime parameters\n");
  struct sched_param param = {.sched_priority = Realtime_Priority};
  if (pthread_setschedparam(thread, Scheduling_Policy, &param)) {
    printf("could not set realtime parameters\n");
    exit(5);
  }
#if defined(_POSIX_PRIORITY_SCHEDULING) && Scheduling_Policy == SCHED_RR
  struct timespec interval;
  sched_rr_get_interval(0, &interval);
  printf("RR interval: %ld s %ld ns\n", interval.tv_sec, interval.tv_nsec);
#endif
#endif
}

static void setupResources() {
  singleCoreOnly();
  setRealtimeParameters(pthread_self());
  setFromId();
  initBarrier();
}
