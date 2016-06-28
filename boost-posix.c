#include <sys/types.h>
#include <stddef.h>
#include <pthread.h>
#include <stdio.h>

#include "config.h"
#include "error.h"
#include "yieldTo.h"
#include "scheduling.h"

static volatile pthread_t to = NULL;
static volatile pthread_t from = NULL;

void registerPreemptionHook(){}; // not implemented
void marker(){}; // not implemented

void yield(pthread_t thread);

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

static inline void boost(pthread_t thread) {
  printf("boosting '%s'\n", getName(thread));
  if (setPriority(thread, Realtime_Priority + 1)) error(boostError, "boost failed");
}

void yield(pthread_t thread) {
  if (pthread_self() == thread) error(yieldToSelfError, "Tried to yield to self.");
  boost(thread);
}

void yieldTo() {
  yield(to);
}

void yieldBack() {
  yield(from);
}