#ifndef __PikeOS__
  #error not on PikeOS
#endif

#include <sys/sched_hook.h>

#include "yieldTo.h"

#define Scheduling_Policy SCHED_RR
#define Realtime_Priority 75
#define Background_Thread_Number 20
#define Load_Factor 5

// works via a preemption-notification, not a direct yieldTo
static volatile bool preempted = false;
static volatile pthread_t toId;

void marker(){} // not implemented

void deboost() {
  printf("deboosting\n");
  struct sched_param param = { .sched_priority = Realtime_Priority };
  if (pthread_setschedparam(to, Scheduling_Policy, &param) != 0) {
    printf("deboosting failed\n");
    exit(8);
  }
}

void registerPreemptionHook() {
  if (__set_sched_hook(SCHED_PREEMPT_HOOK, preempt_hook) == (__sched_hook_t *) - 1) {
    printf("failed to register preemption hook\n");
    exit(6);
  }
  printf("registered preemption hook\n");
}

void singleCoreOnly() {
  cpuset_t cpuSet;
  CPU_ZERO(&cpuSet);
  CPU_SET(0, &cpuSet);

  int s = pthread_setaffinity_np(pthread_self(), &cpuSet);
  if (s != 0) {
    perror("could not set affinity");
    exit(1);
  }
}

static int preempt_hook(unsigned cpu, pthread_t t_old, pthread_t t_new) {
  UNUSED(cpu); UNUSED(t_old); UNUSED(t_new);
  if (yieldedTo) return false;
  preempted = true;
  return true;
}

inline long yieldTo() {
  printf("boosting\n");
  struct sched_param param = { .sched_priority = Realtime_Priority + 1 };
  if (pthread_setschedparam(toId, Scheduling_Policy, &param) != 0) {
    printf("boosting failed\n");
    exit(7);
  }
  if (preempted) {
    preempted = false;
    printf("re-allowing preemption\n");
    __revert_sched_boost(pthread_self());
  }
  return 0;
}