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

#define Scheduling_Policy SCHED_RR
#define Realtime_Priority 63


// works via a preemption-notification, not a direct to
static volatile bool preempted = false;

static volatile pthread_t to = NULL;
static volatile pthread_t from = NULL;

void marker(){} // not implemented

void setFromId() {
  if (from) exit(9);
  if (to && to == from) exit(11);
  from = pthread_self();
  printf("'from' is %p\n", from);
}

void setToId() {
  if (to) exit (10);
  if (from && to == from) exit(12);
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
  struct sched_param param = { .sched_priority = Realtime_Priority };
  if (pthread_setschedparam(to, Scheduling_Policy, &param) != 0) {
    printf("deboosting failed\n");
    exit(8);
  }
}

static int preempt_hook(unsigned cpu, pthread_t t_old, pthread_t t_new) {
  UNUSED(cpu); UNUSED(t_old); UNUSED(t_new);
  printf("kernel invoked pre-emption hook\n");
  if (yieldedTo) return false;
  preempted = true;
  return true;
}

void registerPreemptionHook() {
  /*if (__set_sched_hook(SCHED_PREEMPT_HOOK, preempt_hook) == (__sched_hook_t *) - 1) {
    printf("failed to register pre-emption hook\n");
    exit(6);
  }
  printf("registered pre-emption hook\n");*/
}

static inline void boost(pthread_t thread) {
  printf("boosting '%s'\n", getName(thread));
  struct sched_param param = { .sched_priority = Realtime_Priority + 1 };
  if (pthread_setschedparam(thread, Scheduling_Policy, &param) != 0)
    error(boostError, "boosting failed");
}

static inline void yield(pthread_t id) {
  pthread_t self = pthread_self();
  if (self == id) error(yieldToSelfError, "Tried to yield to self.");
  boost(self); // we don't want to yield quite yet
  boost(id);
  if (preempted) {
    preempted = false;
    printf("'%s' re-allows pre-emption\n", getName(self));
    __revert_sched_boost(self);
    deboost(); // should imply yield
  }
}

inline void yieldTo() {
  yield(to);
}

inline void yieldBack() {
  yield(from);
}