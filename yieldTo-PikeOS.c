#ifndef __PikeOS__
  #error not on PikeOS
#endif

#include <sys/sched_hook.h>
#include <stdbool.h>
#include <stdio.h>
#include <pthread.h>

#include "yieldTo.h"
#include "error.h"
#include "posixhelpers.h"

#define Scheduling_Policy SCHED_RR
#define Realtime_Priority 63


// works via a preemption-notification, not a direct to
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

static char* getName(pthread_t thread) {
  if (thread == to) return "to";
  if (thread == from) return "from";
  return "unknown";
}

bool boosted(pthread_t thread) {
  return getPriority(thread) == Realtime_Priority + 1;
}

bool deboosted(pthread_t thread) {
  return getPriority(thread) == Realtime_Priority;
}

void deboost() {
  pthread_t self = pthread_self();
  if (deboosted(self)) {
    debug(2, "handling last yieldBack\n");
    return;
  }
  printf("deboosting '%s'...", getName(self));
  if (setPriority(self, Realtime_Priority)) error(deboostError); // should never yield
  printf("done\n");
  if (!deboosted(self)) error(deboostError);
}

static int preempt_hook(unsigned __attribute__((unused)) cpu,
                        pthread_t t_old,
                        pthread_t __attribute__((unused)) t_new) {
  if (yielding) {
    debug(2, "kernel saw intentional yield to '%s'\n", getName(t_new));
    return true;
  }
  if (t_old == from || t_old == to) {
    printf("kernel invoked pre-emption hook in '%s'\n", getName(pthread_self()));
    kernelWantsPreemption = true;
    return false; // block pre-emption
  }
  return true; // allow pre-emption
}

void registerPreemptionHook() {
  if (__set_sched_hook(SCHED_PREEMPT_HOOK, preempt_hook) == (__sched_hook_t *) - 1)
    error(preemptionHookRegistrationError);
  printf("registered pre-emption hook\n");
}

static pthread_t next() {
  pthread_t self = pthread_self();
  if (self == to) return from;
  if (self == from) return to;
  error(unknownThread);
  return self;
}

void returningFromYield() {
  pthread_t self = pthread_self();
  printf("'%s' returning\n", getName(self));
  yielding = false;
  deboost();
}

static void boost(pthread_t thread) {
  pthread_t self = pthread_self();
  printf("'%s' boosting '%s'\n", getName(self), getName(thread));
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
  printf("'%s' wants to yield to '%s'\n", getName(self), getName(next()));
  if (yielding) error(alreadyYielding);
  if (!deboosted(self)) error(yieldBeforeDeboost);
  if (self == thread) error(yieldToSelf);
  boost(thread);
  if (kernelWantsPreemption) { // need to revert kernel boost in case it forced the pre-emption
    kernelWantsPreemption = false;
    printf("'%s' re-allows pre-emption\n", getName(self));
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

inline void syncPoint() {
  if (yielding) { // Todo: only really needed for first yieldTo, not in the greatest spot here
    debug(2, "handling first yieldTo\n");
    yielding = false;
    deboost();
  }
  if (inSync) error(alreadyInSyncpoint);
  inSync = true;
  if (kernelWantsPreemption)  {
    printf("forced yield in '%s'\n", getName(pthread_self()));
    yield(next());
  }
  inSync = false;
}