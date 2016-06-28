#ifndef __PikeOS__
  #error not on PikeOS
#endif

#include <sys/sched_hook.h>
#include <stdbool.h>
#include <stdio.h>
#include <pthread.h>

#include "yieldTo.h"
#include "error.h"
#include "scheduling.h"
#include "config.h"
#include "statehandling.h"

static volatile bool kernelWantsPreemption = false;
static volatile bool yielding = false;

static volatile pthread_t to = NULL;
static volatile pthread_t from = NULL;

void marker(){} // not implemented

void setFromId() {
  if (from) error(fromAlreadySet);
  if (to && to == from) error(fromAndToTheSame);
  from = pthread_self();
  debug(3, "'from' is %p\n", from);
}

void setToId() {
  if (to) error(toAlreadySet);
  if (from && to == from) error(fromAndToTheSame);
  to = pthread_self();
  debug(3, "'to' is %p\n", to);
}

static const char const * getName(pthread_t thread) {
  if (thread == to) return "to";
  if (thread == from) return "from";
  return "unknown";
}

static const char const * selfName() { return getName(pthread_self()); }

static bool boosted(pthread_t thread) {
  return getPriority(thread) == Realtime_Priority + 1;
}

static bool deboosted(pthread_t thread) {
  return getPriority(thread) == Realtime_Priority;
}

static void deboost() {
  pthread_t self = pthread_self();
  if (deboosted(self)) {
    debug(2, "handling last yieldBack\n");
    return;
  }
  debug(1, "deboosting '%s'...", selfName());
  if (setPriority(self, Realtime_Priority)) error(deboostError); // should never yield according to POSIX
  debug(1, "done\n");
  if (!deboosted(self)) error(deboostError);
}

static int preempt_hook(unsigned __attribute__((unused)) cpu,
                        pthread_t t_old,
                        pthread_t __attribute__((unused)) t_new) {
  if (yielding) {
    debug(2, "kernel saw intentional yield to '%s'\n", getName(t_new));
    return true; // allow pre-emption
  }
  if (t_old == from || t_old == to) {
    debug(1, "kernel invoked pre-emption hook in '%s'\n", selfName());
    kernelWantsPreemption = true;
    return false; // block pre-emption
  }
  return true; // allow pre-emption
}

void registerPreemptionHook() {
  if (__set_sched_hook(SCHED_PREEMPT_HOOK, preempt_hook) == (__sched_hook_t *) - 1)
    error(preemptionHookRegistrationError);
  debug(1, "registered pre-emption hook\n");
}

static pthread_t next() {
  pthread_t self = pthread_self();
  if (self == to) return from;
  if (self == from) return to;
  error(unknownThread);
  return self;
}

static void returningFromYield() {
  debug(1, "'%s' returning\n", selfName());
  yielding = false;
  deboost();
}

static void boost(pthread_t thread) {
  pthread_t self = pthread_self();
  debug(1, "'%s' boosting '%s'\n", selfName(), getName(thread));
  if (!deboosted(self)) error(mustDeboostSelf);
  if (boosted(thread)) error(alreadyBoosted);

  if (!kernelWantsPreemption) {
    yielding = true;
    if (setPriority(thread, Realtime_Priority + 1)) error(boostError);
    // by this point we should have went through one yieldTo/back-cycle and hence be boosted
    returningFromYield();
  } else {
    debug(2, "non-yielding boost\n");
    if (setPriority(thread, Realtime_Priority + 1)) error(boostError); // not yielding due to kernel boost
  }
}

static inline void yield(pthread_t thread) {
  pthread_t self = pthread_self();
  debug(1, "'%s' wants to yield to '%s'\n", selfName(), getName(next()));
  if (yielding) error(alreadyYielding);
  if (!deboosted(self)) error(yieldBeforeDeboost);
  if (self == thread) error(yieldToSelf);
  boost(thread);
  if (kernelWantsPreemption) { // need to revert kernel boost in case it forced the pre-emption
    kernelWantsPreemption = false;
    debug(1, "'%s' re-allows pre-emption\n", selfName());
    yielding = true;
    __revert_sched_boost(self);
    // by this point we should have went through one yieldTo/back-cycle and hence be boosted
    returningFromYield();
  }
}

inline void yieldTo() {
  yield(to);
}

inline void yieldBack() {
  yield(from);
}

static volatile yieldType * incomingYieldFlagForSelf() {
  return (pthread_self() == to ? &fromState->incomingYield : &toState->incomingYield);
}

static void preemptInSyncpoint() {
  debug(2, "handling pre-emption in syncpoint");
  volatile yieldType * incomingYieldFlag = incomingYieldFlagForSelf();
  if(*incomingYieldFlag == forcedYield) error(alreadyInSyncpoint);
  *incomingYieldFlag = forcedYield;
  debug(1, "forced yield in '%s'\n", selfName());
  yield(next());
  debug(1, "'%s' returning in syncpoint\n", selfName());
  *incomingYieldFlag = noYield;
}


inline void syncPoint() {
  if (yielding) { // Todo: only really needed for first yieldTo, not in the greatest spot here
    debug(2, "handling first yieldTo\n");
    yielding = false;
    deboost();
  }
  if (kernelWantsPreemption) preemptInSyncpoint();
}