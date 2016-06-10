#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdbool.h>

#include "yieldTo.h"
#include "barrier.h"
#include "config.h"
#include "error.h"
#include "posixhelpers.h"

volatile bool yieldedTo = false;
static volatile bool yieldedBack = false;

static pthread_t tid[Background_Thread_Number];
static pthread_t to;

static volatile bool toFinished = false;
static volatile bool started = false;

static void setupResources();
static void startThreads();
static void joinThreads();
static void checkedYieldTo();

int main(int argc, char *argv[]) {
  printf("launching yieldTo\n");
  setupResources();
  startThreads();
  registerPreemptionHook();
  waitAtBarrier();
  marker();
  started = true;
  while (!toFinished) {
    checkedYieldTo();

    if (yieldedBack) {
      printf("yieldBack worked\n");
      yieldedBack = false;
      deboost();
    } else fail(yieldBackFail, "yieldBack failed, status flag unchanged.");

    for (unsigned long k = 0; k < Loops_Between_Yields && !toFinished; k++)
      yieldedTo = false;
  }
  joinThreads();
  destroyBarrier();
  printf("yieldTo test succeeded\n");
}

static void checkedYieldTo() {
  yieldedTo = true;
  yieldTo();  // resets yieldedTo to false
  if (yieldedTo) fail(yieldToFail, "yieldTo failed, still in from.");
  yieldedTo = false;
}

static void *toLogic(void *ignored) {
  UNUSED(ignored);
  setToId();
  waitAtBarrier();
  sched_yield();
  marker();
  for (int i = 0; i < Yield_Count; i++)
    for (unsigned long k = 0; k < Loops_Between_Yields; k++) {
      if (yieldedTo) {
        printf("yieldTo worked!\n");
        yieldedTo = false;
        deboost();

        yieldedBack = true;
        yieldBack();
        if (yieldedBack) fail(yieldBackFail, "yieldBack failed, still in to.");
        yieldedBack = false;
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
  for (int i = 0; i < Yield_Count && !toFinished; i++)
    for (unsigned long k = 0; k < Loops_Between_Yields && !toFinished; k++) {
      if (yieldedTo) fail(yieldToFail, "yieldTo failed, in background thread.");
      yieldedTo = false;
      if (yieldedBack) fail(yieldBackFail, "yieldBack failed, in background thread.");
      yieldedBack = false;
      if (Want_Starvation && started && !toFinished) fail(notStarved, "background thread was not starved.");
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
  printRRSchedulingInfo();
#endif
}

static void setupResources() {
  singleCoreOnly();
  setRealtimeParameters(pthread_self());
  setFromId();
  initBarrier();
}
