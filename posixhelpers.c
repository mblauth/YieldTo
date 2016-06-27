#include <pthread.h>
#include <stdio.h>
#include <unistd.h>

#include "config.h"
#include "error.h"
#include "posixhelpers.h"

enum errorcode setPriority(pthread_t thread, int priority) {
  if (pthread_setschedprio(thread, priority) != 0)
    return prioritySetFailed;
  return noError;
}

int getPriority(pthread_t thread) {
  int unused;
  struct sched_param param;
  if (pthread_getschedparam(thread, &unused, &param))
    error(priorityGetFailed);
  return param.sched_priority;
}

void printRRSchedulingInfo() {
#if defined(_POSIX_PRIORITY_SCHEDULING) && Scheduling_Policy == SCHED_RR
  struct timespec interval;
  sched_rr_get_interval(0, &interval);
  printf("RR interval: %ld ms %ld µs %ld ns\n",
         interval.tv_nsec / 1000000, interval.tv_nsec / 1000 % 1000, interval.tv_nsec % 1000);
#endif
}
