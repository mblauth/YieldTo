#include <stdio.h>
#include <stdbool.h>

#include "yieldTo.h"
#include "barrier.h"
#include "config.h"
#include "error.h"
#include "scheduling.h"
#include "threadmanagement.h"

struct inSync inSync = { false, false };

typedef struct activity_t {
  void (*syncpointCheck)();
  enum logEvent finishedEvent;
} activity;

static volatile bool yieldedTo = false;
static volatile bool yieldedBack = false;
static volatile bool toFinished = false;
static volatile bool started = false;

static void setupResources();
static void checkedYieldTo();
static void checkYieldBack();
static void step();
static void checkYieldTo();
static void checkedYieldBack();
static void *toLogic(void*);
static void *backgroundLogic(void*);

static void startup();
static void shutdown();
static void runLoop(activity loopActivity) ;

int main(int argc, char *argv[]) {
  startup();
  activity from = { .syncpointCheck=&checkYieldBack,
                    .finishedEvent=fromLoopFinishedEvent };
  while (!toFinished) {
    checkedYieldTo();
    runLoop(from);
  }
  shutdown();
}

static void *toLogic(void *  __attribute__((unused)) ignored) {
  setToId();
  waitAtBarrier();
  sched_yield();
  marker();
  activity to = { .syncpointCheck=&checkYieldTo,
                  .finishedEvent=toLoopFinishedEvent };
  for (int i = 0; i < Yield_Count; i++) {
    runLoop(to);
    checkedYieldBack();
  }
  toFinished = true;
  status("yieldTo target finished execution");
  return NULL;
}

static void runLoop(activity loopActivity) {
  for (unsigned long k = 0; k < Loops_Between_Yields; k++) {
    step();
    syncPoint();
    loopActivity.syncpointCheck();
  }
  log(loopActivity.finishedEvent);
}

static void shutdown() {
  joinThreads();
  destroyBarrier();
  status("yieldTo test succeeded");
}

static void startup() {
  status("launching yieldTo test");
  setupResources();
  startThreads(&toLogic, &backgroundLogic);
  status("test setup finished");
  waitAtBarrier();
  registerPreemptionHook();
  marker();
  started = true;
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
  } else if (inSync.to) {
    log(toPreemptionEvent);
    inSync.to = false;
  }
}

static void checkYieldTo() {
  if (yieldedTo) {
    log(yieldToEvent);
    yieldedTo = false;
  } else if (inSync.from) {
    log(fromPreemptionEvent);
    inSync.from = false;
  }
}

static void checkedYieldTo() {
  if (inSync.from) error(inSyncpoint);
  yieldedTo = true;
  yieldTo();
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


static void setupResources() {
  singleCoreOnly();
  setDefaultRealtimeParameters();
  setFromId();
  initBarrier();
}
