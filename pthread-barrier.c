#include <pthread.h>

#include "barrier.h"

static pthread_barrier_t barrier;

void initBarrier() {
  pthread_barrier_init(&barrier, NULL, 2);
}

void waitAtBarrier() {
  pthread_barrier_wait(&barrier);
}

void destroyBarrier() {
  pthread_barrier_destroy(&barrier);
}