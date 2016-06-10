#ifndef __PikeOS__
  #error not on PikeOS
#endif

#include <sys/sched_hook.h>
#include <stdbool.h>
#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>

#include "yieldTo.h"
#include "error.h"
#include "posixhelpers.h"

#define Scheduling_Policy SCHED_RR
#define Realtime_Priority 63


// works via a preemption-notification, not a direct to
static volatile bool kernelWantsPreemption = false;

static volatile pthread_t to = NULL;
static volatile pthread_t from = NULL;

void marker(){} // not implemented

void setFromId() {
  if (from) error(fromAlreadySet, "from is already set");
  if (to && to == from) error(fromAndToTheSame, "from and to are the same thread");
  from = pthread_self();
  printf("'from' is %p\n", from);
}

void setToId() {
  if (to) error(toAlreadySet, "to is already set");
  if (from && to == from) error(fromAndToTheSame, "from and to are the same thread");
  to = pthread_self();
  printf("'to' is %p\n", to);
}

static char* getName(pthread_t thread) {
  if (thread == to) return "to";
  if (thread == from) return "from";
  return "unknown";
}

void deboost() {
  printf("deboosting '%s'\n", getName(pthread_self()));
  if (setPriority(pthread_self(), Realtime_Priority)) error(deboostError, "deboost failed");
}

static int preempt_hook(unsigned __attribute__((unused)) cpu,
                        pthread_t t_old,
                        pthread_t __attribute__((unused)) t_new) {
  printf("kernel invoked pre-emption hook in %s\n", getName(pthread_self()));
  if (t_old == from || t_old == to) {
    kernelWantsPreemption = true;
    return false;
  }
  return true;
}

void registerPreemptionHook() {
  if (__set_sched_hook(SCHED_PREEMPT_HOOK, preempt_hook) == (__sched_hook_t *) - 1)
    error(preemptionHookRegistrationError, "failed to register pre-emption hook");
  printf("registered pre-emption hook\n");
}

static inline void boost(pthread_t thread) {
  printf("boosting '%s'\n", getName(thread));
  if (setPriority(thread, Realtime_Priority + 1)) error(boostError, "boost failed");
}

static inline void yield(pthread_t thread) {
  pthread_t self = pthread_self();
  if (self == thread) error(yieldToSelfError, "Tried to yield to self.");
  boost(thread);
  if (kernelWantsPreemption) {
    kernelWantsPreemption = false;
    printf("'%s' re-allows pre-emption\n", getName(self));
    __revert_sched_boost(self);
  }
}

inline void yieldTo() {
  yield(to);
}

inline void yieldBack() {
  yield(from);
}