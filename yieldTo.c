#include <stdio.h>
#include <stdbool.h>

#include "yieldTo.h"
#include "barrier.h"
#include "config.h"
#include "error.h"
#include "scheduling.h"
#include "threadmanagement.h"
#include "statehandling.h"

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
static void runLoop(yieldState*);
static void checkYield(yieldState*);

int main(int argc, char *argv[]) {
  startup();
  while (!toFinished) {
    checkedYieldTo();
    runLoop(fromState);
  }
  shutdown();
}

static void *toLogic(void *  __attribute__((unused)) ignored) {
  setToId();
  waitAtBarrier();
  sched_yield();
  marker();
  for (int i = 0; i < Yield_Count; i++) {
    runLoop(toState);
    checkedYieldBack();
  }
  toFinished = true;
  status("yieldTo target finished execution");
  return NULL;
}

static void runLoop(yieldState * state) {
  for (unsigned long k = 0; k < Loops_Between_Yields; k++) {
    step();
    syncPoint();
    checkYield(state);
  }
  log(state->logEvents.finished);
}

static void shutdown() {
  joinThreads();
  destroyBarrier();
  status("yieldTo test succeeded");
}

static void startup() {
  status("launching yieldTo test");
  createFromState();
  createToState();
  setupResources();
  startThreads(&toLogic, &backgroundLogic);
  waitAtBarrier();
  registerPreemptionHook();
  marker();
  status("test setup finished");
  started = true;
}

inline static void step() {
  fromState->incomingYield = false;
  toState->incomingYield = false;
}

static void logAndUnsetIfSet(volatile bool * flag, enum logEvent event) {
  if (*flag) {
    log(event);
    *flag=false;
  }
}

static void checkYield(yieldState * state) {
  if (state->direction == To_to_from && toFinished) return;
  logAndUnsetIfSet(&state->incomingYield, state->logEvents.incomingYield);
  logAndUnsetIfSet(&state->otherInSyncpoint, state->logEvents.preemption);
}

static void checkedYieldTo() {
  if (fromState->otherInSyncpoint) error(inSyncpoint);
  toState->incomingYield = true;
  yieldTo();
  if (toState->incomingYield && !toFinished) fail(yieldToFail, fromThread);
  toState->incomingYield = false;
}

static void checkedYieldBack() {
  if (toState->otherInSyncpoint) error(inSyncpoint);
  fromState->incomingYield = true;
  yieldBack();
  if (fromState->incomingYield) fail(yieldBackFail, toThread);
}

static void * backgroundLogic(void *__attribute__((unused)) ignored) {
  for (int i = 0; i < Yield_Count && !toFinished; i++)
    for (unsigned long k = 0; k < Loops_Between_Yields && !toFinished; k++) {
      if (toState->incomingYield) fail(yieldToFail, backgroundThread);
      if (fromState->incomingYield) fail(yieldBackFail, backgroundThread);
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
