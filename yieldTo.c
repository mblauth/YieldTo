#include <stdio.h>
#include <pthread.h>
#include <stdbool.h>

#include "yieldTo.h"
#include "barrier.h"
#include "config.h"
#include "error.h"
#include "posixhelpers.h"

struct inSync inSync = { false, false };

static volatile bool yieldedTo = false;
static volatile bool yieldedBack = false;
static volatile bool toFinished = false;
static volatile bool started = false;

static pthread_t tid[Background_Thread_Number];
static pthread_t to;

static void setupResources();
static void startThreads();
static void joinThreads();
static void checkedYieldTo();
static void checkYieldBack();
static void step();
static void checkYieldTo();
static void checkedYieldBack();

int main(int argc, char *argv[]) {
  status("launching yieldTo test");
  setupResources();
  startThreads();
  status("test setup finished");
  waitAtBarrier();
  registerPreemptionHook();
  marker();
  started = true;
  while (!toFinished) {
    checkedYieldTo();
    for (unsigned long k = 0; k < Loops_Between_Yields; k++) {
      step();
      syncPoint();
      checkYieldBack();
    }
    log(fromLoopFinishedEvent);
  }
  joinThreads();
  destroyBarrier();
  status("yieldTo test succeeded");
}

static void *toLogic(void *  __attribute__((unused)) ignored) {
  setToId();
  waitAtBarrier();
  sched_yield();
  marker();
  for (int i = 0; i < Yield_Count; i++) {
    for (unsigned long k = 0; k < Loops_Between_Yields; k++) {
      step();
      syncPoint();
      checkYieldTo();
    }
    log(toLoopFinishedEvent);

    checkedYieldBack();
  }
  toFinished = true;
  status("yieldTo target finished execution");
  return NULL;
}

inline static void step() {
  yieldedTo = false;
  yieldedBack = false;
}

static void checkYieldBack() {
  if (toFinished) return;
  else if (yieldedBack) {
    log(yieldBackEvent);
    yieldedBack = false;
  }
  else if (inSync.to) {
    log(toPreemptionEvent);
    inSync.to = false;
  }
}

static void checkYieldTo() {
  if (yieldedTo) {
    log(yieldToEvent);
    yieldedTo = false;
  }
  else if (inSync.from) {
    log(fromPreemptionEvent);
    inSync.from = false;
  }
}

static void checkedYieldTo() {
  if (inSync.from) error(inSyncpoint);
  yieldedTo = true;
  yieldTo();  // resets yieldedTo to false in to thread
  if (yieldedTo && !toFinished) fail(yieldToFail, fromThread);
  yieldedTo = false;
}

static void checkedYieldBack() {
  if (inSync.to) error(inSyncpoint);
  yieldedBack = true;
  yieldBack();
  if (yieldedBack) fail(yieldBackFail, toThread);
  yieldedBack = false;
}


static void *backgroundLogic(void *__attribute__((unused)) ignored) {
  for (int i = 0; i < Yield_Count && !toFinished; i++)
    for (unsigned long k = 0; k < Loops_Between_Yields && !toFinished; k++) {
      if (yieldedTo) fail(yieldToFail, backgroundThread);
      if (yieldedBack) fail(yieldBackFail, backgroundThread);
      if (Want_Starvation && started && !toFinished) fail(notStarved, backgroundThread);
      step();
    }
  return NULL;
}

static void startBackgroundThreads() {
  printf("creating %i background threads...", Background_Thread_Number);
#if Scheduling_Policy == SCHED_FIFO || Scheduling_Policy == SCHED_RR
  pthread_attr_t attr;
  if (pthread_attr_init(&attr)) error(threadAttributeError);
  if (pthread_attr_setschedpolicy(&attr, Scheduling_Policy)) error(policyError);
  struct sched_param param = {.sched_priority = Realtime_Priority};
  if (pthread_attr_setschedparam(&attr, &param)) error(prioritySetFailed);
  for (int i = 0; i < Background_Thread_Number; i++)
    pthread_create(&tid[i], &attr, &backgroundLogic, NULL);
  pthread_attr_destroy(&attr);
#else
  for (int i = 0; i < Background_Thread_Number; i++)
      pthread_create(&tid[i], NULL, &busy, NULL);
#endif
  printf("done\n");
}

static void startThreads() {
  pthread_create(&to, NULL, &toLogic, NULL);
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
  printf("setting realtime parameters policy: %d, priority: %d\n", Scheduling_Policy, Realtime_Priority);
  struct sched_param param = {.sched_priority = Realtime_Priority};
  if (pthread_setschedparam(thread, Scheduling_Policy, &param))
    error(prioritySetFailed);
  printRRSchedulingInfo();
#endif
}

static void setupResources() {
  singleCoreOnly();
  setRealtimeParameters(pthread_self());
  setFromId();
  initBarrier();
}
