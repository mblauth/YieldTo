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
  directionalEvents events;
  volatile bool * yieldFlag;
  volatile bool * otherInSyncpoint;
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
static void runLoop(activity);
static void checkYield(activity);

int main(int argc, char *argv[]) {
  startup();
  activity from = { .direction=from_To_to,
                    .events=fromEvents,
                    .yieldFlag=&yieldedBack,
                    .otherInSyncpoint=&inSync.to };
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
                  .events=toEvents,
                  .yieldFlag=&yieldedTo,
                  .otherInSyncpoint=&inSync.from };
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
    checkYield(loopActivity);
  }
  log(loopActivity.events.finished);
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

static void logAndUnsetIfSet(volatile bool * flag, enum logEvent event) {
  if (*flag) {
    log(event);
    *flag=false;
  }
}

static void checkYield(activity yieldActivity) {
  if (yieldActivity.direction == To_to_from && toFinished) return;
  logAndUnsetIfSet(yieldActivity.yieldFlag, yieldActivity.events.incomingYield);
  logAndUnsetIfSet(yieldActivity.otherInSyncpoint, yieldActivity.events.preemption);
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
