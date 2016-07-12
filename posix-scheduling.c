#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>

#include "config.h"
#include "error.h"
#include "scheduling.h"

static const int defaultPriority = Realtime_Priority;
static const int boostPriority = Realtime_Priority + 1;

static pthread_t from = NULL;
static pthread_t to = NULL;

void registerFrom(pthread_t thread) {
  if (from) error(fromAlreadySet);
  if (thread == to) error(fromAndToTheSame);
  from = thread;
  debug(4, "'from' is %p\n", from);
}

void registerTo(pthread_t thread) {
  if (to) error(toAlreadySet);
  if (thread == from) error(fromAndToTheSame);
  to = thread;
  debug(4, "'to' is %p\n", to);
}

bool isFrom(pthread_t thread) { return thread == from; }
pthread_t getFrom() { return from; }

bool isTo(pthread_t thread) { return thread == to; }
pthread_t getTo() { return to; }

char const * getName(pthread_t thread) {
  if (isTo(thread)) return "to";
  if (isFrom(thread)) return "from";
  return "unknown";
}

char const * selfName() { return getName(pthread_self()); }

static int getPriority(pthread_t thread) {
  int unused;
  struct sched_param param;
  if (pthread_getschedparam(thread, &unused, &param))
    error(priorityGetFailed);
  return param.sched_priority;
}

static void setPriority(pthread_t thread, int priority) {
  debug(3, "'%s' setting priority %d for '%s'\n", selfName(), priority, getName(thread));
  if (getPriority(thread) == priority) error(priorityAlreadySet);
  if (pthread_setschedprio(thread, priority)) error(prioritySetFailed); // yields if priority > current thread's
}

void setBoostPriority(pthread_t thread) {
  if (thread == pthread_self()) error(yieldToSelf);
  if (boosted(thread)) error(alreadyBoosted);
  debug(3, "'%s' setting boost priority (%d) for '%s'\n", selfName(), boostPriority, getName(thread));
  setPriority(thread, boostPriority); // effectively yields if nothing prevents pre-emption
}

void setRegularPriority(pthread_t thread) {
  if (deboosted(thread)) error(alreadyDeboosted);
  setPriority(thread, defaultPriority); // never yields
  if (!deboosted(thread)) error(deboostError);
}

bool boosted(pthread_t thread) {
  return getPriority(thread) == boostPriority;
}

bool deboosted(pthread_t thread) {
  return getPriority(thread) == defaultPriority;
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
  printf("setting realtime parameters policy: %d, priority: %d\n", Scheduling_Policy, defaultPriority);
  struct sched_param param = {.sched_priority = defaultPriority};
  if (pthread_setschedparam(pthread_self(), Scheduling_Policy, &param))
    error(prioritySetFailed);
  printRRSchedulingInfo();
#endif
}