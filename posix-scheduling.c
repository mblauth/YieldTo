#include <pthread.h>
#include <stdio.h>
#include <unistd.h>

#include "config.h"
#include "error.h"
#include "scheduling.h"

static enum errorcode setPriority(pthread_t thread, int priority) {
  if (getPriority(thread) == priority) error(priorityAlreadySet);
  if (pthread_setschedprio(thread, priority) != 0)
    return prioritySetFailed;
  return noError;
}

void setBoostPriority(pthread_t thread) {
  enum errorcode errorcode = setPriority(thread, Realtime_Priority + 1);
  if (errorcode != noError) error(errorcode);
}

void setRegularPriority(pthread_t thread) {
  enum errorcode errorcode = setPriority(thread, Realtime_Priority);
  if (errorcode != noError) error(errorcode);
}

int getPriority(pthread_t thread) {
  int unused;
  struct sched_param param;
  if (pthread_getschedparam(thread, &unused, &param))
    error(priorityGetFailed);
  return param.sched_priority;
}

static void printRRSchedulingInfo() {
#if defined(_POSIX_PRIORITY_SCHEDULING) && Scheduling_Policy == SCHED_RR
  struct timespec interval;
  sched_rr_get_interval(0, &interval);
  printf("RR interval: %ld ms %ld µs %ld ns\n",
         interval.tv_nsec / 1000000, interval.tv_nsec / 1000 % 1000, interval.tv_nsec % 1000);
#endif
}

void setDefaultRealtimeParameters() {
#if Scheduling_Policy == SCHED_FIFO || Scheduling_Policy == SCHED_RR
  printf("setting realtime parameters policy: %d, priority: %d\n", Scheduling_Policy, Realtime_Priority);
  struct sched_param param = {.sched_priority = Realtime_Priority};
  if (pthread_setschedparam(pthread_self(), Scheduling_Policy, &param))
    error(prioritySetFailed);
  printRRSchedulingInfo();
#endif
}