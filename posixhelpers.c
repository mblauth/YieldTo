#include <pthread.h>

#include "config.h"
#include "error.h"
#include "posixhelpers.h"

enum errorcode setPriority(pthread_t thread, int priority) {
  struct sched_param param = { .sched_priority = priority};
  if (pthread_setschedparam(thread, Scheduling_Policy, &param) != 0)
    return prioritySetFailed;
  return noError;
}
