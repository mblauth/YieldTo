#include <stdio.h>
#include <pthread.h>
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
static void checkYieldBack();
static void step();

int main(int argc, char *argv[]) {
  printf("launching yieldTo test\n");
  setupResources();
  startThreads();
  registerPreemptionHook();
  waitAtBarrier();
  marker();
  started = true;
  while (!toFinished) {
    checkedYieldTo();
    checkYieldBack();
    for (unsigned long k = 0; k < Loops_Between_Yields; k++)
      step();
  }
  joinThreads();
  destroyBarrier();
  printf("yieldTo test succeeded\n");
}

inline static void step() {
  yieldedTo = false;
  yieldedBack = false;
}

static void checkYieldBack() {
  if (toFinished) return;
  if (yieldedBack) {
      printf("yieldBack worked\n");
      yieldedBack = false;
      deboost();
    } else fail(yieldBackFail, "yieldBack failed, status flag unchanged.");
}

static void checkedYieldTo() {
  yieldedTo = true;
  yieldTo();  // resets yieldedTo to false
  if (yieldedTo && !toFinished) fail(yieldToFail, "yieldTo failed, still in from.");
  yieldedTo = false;
}

static void *toLogic(void *  __attribute__((unused)) ignored) {
  setToId();
  waitAtBarrier();
  sched_yield();
  marker();
  for (int i = 0; i < Yield_Count; i++)
    if (yieldedTo) {
      printf("yieldTo worked #%d of %d\n", i+1, Yield_Count);
      yieldedTo = false;
      deboost();

      yieldedBack = true;
      yieldBack();
      if (yieldedBack) fail(yieldBackFail, "yieldBack failed, still in to.");
      yieldedBack = false;
    }
  printf("yieldTo target finished execution\n");
  toFinished = true;
  return NULL;
}

static void *busy(void * __attribute__((unused)) ignored) {
  for (int i = 0; i < Yield_Count && !toFinished; i++)
    for (unsigned long k = 0; k < Loops_Between_Yields && !toFinished; k++) {
      if (yieldedTo) fail(yieldToFail, "yieldTo failed, in background thread.");
      if (yieldedBack) fail(yieldBackFail, "yieldBack failed, in background thread.");
      if (Want_Starvation && started && !toFinished) fail(notStarved, "background thread was not starved.");
      step();
    }
  return NULL;
}

static void startBackgroundThreads() {
  printf("creating %i background threads...", Background_Thread_Number);
#if Scheduling_Policy == SCHED_FIFO || Scheduling_Policy == SCHED_RR
  pthread_attr_t attr;
  if (pthread_attr_init(&attr)) error(threadAttributeError, "failed to initialize thread attributes");
  if (pthread_attr_setschedpolicy(&attr, Scheduling_Policy)) error(policyError, "failed to set scheduling policy");
  struct sched_param param = {.sched_priority = Realtime_Priority};
  if (pthread_attr_setschedparam(&attr, &param)) error(prioritySetFailed, "failed to set realtime priority");
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
  printf("setting realtime parameters\n");
  struct sched_param param = {.sched_priority = Realtime_Priority};
  if (pthread_setschedparam(thread, Scheduling_Policy, &param))
    error(prioritySetFailed, "failed to set realtime parameters");
  printRRSchedulingInfo();
#endif
}

static void setupResources() {
  singleCoreOnly();
  setRealtimeParameters(pthread_self());
  setFromId();
  initBarrier();
}
