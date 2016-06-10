#include <pthread.h>

#include "config.h"
#include "error.h"
#include "posixhelpers.h"

enum errorcode setPriority(pthread_t thread, int priority) {
  if (pthread_setschedprio(thread, priority) != 0)
    return prioritySetFailed;
  return noError;
}
