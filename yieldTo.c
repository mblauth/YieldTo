#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>

#include "yieldTo.h"
#include "barrier.h"
#include "config.h"
#include "error.h"
#include "log.h"
#include "scheduling.h"
#include "threadmanagement.h"
#include "statehandling.h"
#include "histogram.h"

static volatile bool toFinished = false;
static volatile bool started = false;

static void setupResources();
static void checkedYieldTo(yieldState*,yieldState*);
static void step();
static void *toLogic(void*);
static void *backgroundLogic(void*);
static void startup();
static void shutdown();
static void runLoop(yieldState*);
static void checkYield(yieldState*);

int main(int argc, char *argv[]) {
  startup();
  while (!toFinished) {
    checkedYieldTo(fromState, toState);
    runLoop(fromState);
  }
  shutdown();
}

bool toIsFinished() {
  return toFinished;
}

static void *toLogic(void *  __attribute__((unused)) ignored) {
  setToId();
  waitAtBarrier();
  sched_yield();
  marker();
  for (int i = 0; i < Yield_Count; i++) {
    runLoop(toState);
    checkedYieldTo(toState, fromState);
  }
  if (Yield_Count == 0) {
    runLoop(toState);
  }
  toFinished = true;
  printHistogram();
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
  status("sleeping for a second");
  sleep(1);
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
  fromState->incomingYield = noYield;
  toState->incomingYield = noYield;
}

static void checkYield(yieldState * state) {
  if (state == toState && toFinished) return;
  if (state->incomingYield == explicitYield)
    log(state->logEvents.incomingYield);
  else if (state->incomingYield == forcedYield)
    log(state->logEvents.preemption);
  state->incomingYield = noYield;
}

static void checkedYieldTo(yieldState * fromState, yieldState * toState) {
  toState->incomingYield = explicitYield;
  fromState->yieldTo();
  if (toState->incomingYield == explicitYield && !toFinished) fail(yieldToFail, fromState->thread);
  toState->incomingYield = noYield;
}

static void * backgroundLogic(void *__attribute__((unused)) ignored) {
  for (int i = 0; i < Yield_Count && !toFinished; i++)
    for (unsigned long k = 0; k < Loops_Between_Yields && !toFinished; k++) {
      if (toState->incomingYield == explicitYield) fail(yieldToFail, backgroundThread);
      if (fromState->incomingYield == explicitYield) fail(yieldBackFail, backgroundThread);
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
