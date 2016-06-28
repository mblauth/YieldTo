#include <stdio.h>
#include <stdbool.h>

#include "yieldTo.h"
#include "barrier.h"
#include "config.h"
#include "error.h"
#include "scheduling.h"
#include "threadmanagement.h"

struct inSync inSync = { false, false };

enum yieldDirection {
  from_To_to, To_to_from
};

typedef struct activity_t {
  enum yieldDirection direction;
  enum logEvent finishedEvent;
} activity;

static volatile bool yieldedTo = false;
static volatile bool yieldedBack = false;
static volatile bool toFinished = false;
static volatile bool started = false;

static void setupResources();
static void checkedYieldTo();
static void step();
static void checkedYieldBack();
static void *toLogic(void*);
static void *backgroundLogic(void*);

static void startup();
static void shutdown();
static void runLoop(activity loopActivity) ;

static void checkYield(enum yieldDirection direction) ;

int main(int argc, char *argv[]) {
  startup();
  activity from = { .direction=from_To_to,
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
  activity to = { .direction=To_to_from,
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
    checkYield(loopActivity.direction);
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

static void checkYield(enum yieldDirection direction) {
  if (direction == To_to_from && toFinished) return;

  enum logEvent yieldEvent;
  enum logEvent preemptionEvent;
  volatile bool *yieldFlag;
  volatile bool *syncFlag;

  if (direction == from_To_to) {
    yieldEvent = yieldBackEvent;
    preemptionEvent = toPreemptionEvent;
    yieldFlag = &yieldedBack;
    syncFlag = &inSync.to;
  } else {
    yieldEvent = yieldToEvent;
    preemptionEvent = fromPreemptionEvent;
    yieldFlag = &yieldedTo;
    syncFlag = &inSync.from;
  }

  if (*yieldFlag) {
    log(yieldEvent);
    *yieldFlag = false;
  } else if (*syncFlag) {
    log(preemptionEvent);
    *syncFlag = false;
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
