#include <sys/cpuset.h>
#include <stdarg.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

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