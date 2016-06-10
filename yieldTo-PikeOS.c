#ifndef __PikeOS__
  #error not on PikeOS
#endif

#include <sys/sched_hook.h>
#include <stdbool.h>
#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>

#include "yieldTo.h"

#define Scheduling_Policy SCHED_RR
#define Realtime_Priority 75
#define Background_Thread_Number 20
#define Load_Factor 5

// works via a preemption-notification, not a direct yieldTo
static volatile bool preempted = false;

static volatile pthread_t to;
static volatile pthread_t back;

void marker(){} // not implemented

void setFromId() {
  back = pthread_self();
}

void setToId() {
  to = pthread_self();
}

// Todo: use this
void deboost() {
  printf("deboosting\n");
  struct sched_param param = { .sched_priority = Realtime_Priority };
  if (pthread_setschedparam(to, Scheduling_Policy, &param) != 0) {
    printf("deboosting failed\n");
    exit(8);
  }
}

static int preempt_hook(unsigned cpu, pthread_t t_old, pthread_t t_new) {
  UNUSED(cpu); UNUSED(t_old); UNUSED(t_new);
  if (yieldedTo) return false;
  preempted = true;
  return true;
}

void registerPreemptionHook() {
  if (__set_sched_hook(SCHED_PREEMPT_HOOK, preempt_hook) == (__sched_hook_t *) - 1) {
    printf("failed to register preemption hook\n");
    exit(6);
  }
  printf("registered preemption hook\n");
}

static inline void boost(pthread_t id) {
  printf("boosting\n");
  struct sched_param param = { .sched_priority = Realtime_Priority + 1 };
  if (pthread_setschedparam(id, Scheduling_Policy, &param) != 0) {
    printf("boosting failed\n");
    exit(7);
  }
}

static inline void yield(pthread_t id) {
  boost(id);
  if (preempted) {
    preempted = false;
    printf("re-allowing preemption\n");
    __revert_sched_boost(pthread_self());
  }
}

inline void yieldTo() {
  yield(to);
}

inline void yieldBack() {
  yield(back);
}