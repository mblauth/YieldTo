#include <sched.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

void deboost(){}
void marker(){}
void registerPreemptionHook() {}
void setFromId(){}
void setToId(){}

#ifdef __linux
  void singleCoreOnly() {
    cpu_set_t cpuSet;
    CPU_ZERO(&cpuSet);
    CPU_SET(0, &cpuSet);

    int s = pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuSet);
    if (s != 0) {
      perror("could not set affinity");
      exit(1);
    }
  }
#else
  void singleCoreOnly() {
    printf("affinities not supported\n");
  }
#endif


void yieldTo() { // not a yieldTo(), just here for testing
  sched_yield();
}

void yieldBack() { // not a yieldTo(), just here for testing
  sched_yield();
}
