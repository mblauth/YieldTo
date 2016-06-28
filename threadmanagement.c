#include <stdio.h>
#include <sys/types.h>
#include <pthread.h>
#include "threadmanagement.h"
#include "config.h"
#include "error.h"
#include "scheduling.h"

static void startBackgroundThreads(void* (*backgroundLogic)(void*)) {
  printf("creating %i background threads...", Background_Thread_Number);
#if Scheduling_Policy == SCHED_FIFO || Scheduling_Policy == SCHED_RR
  pthread_attr_t attr;
  if (pthread_attr_init(&attr)) error(threadAttributeError);
  if (pthread_attr_setschedpolicy(&attr, Scheduling_Policy)) error(policyError);
  struct sched_param param = {.sched_priority = Realtime_Priority};
  if (pthread_attr_setschedparam(&attr, &param)) error(prioritySetFailed);
  for (int i = 0; i < Background_Thread_Number; i++)
    pthread_create(&tid[i], &attr, backgroundLogic, NULL);
  pthread_attr_destroy(&attr);
#else
  for (int i = 0; i < Background_Thread_Number; i++)
      pthread_create(&tid[i], NULL, &busy, NULL);
#endif
  printf("done\n");
}

void startThreads(void* (*toLogic)(void*), void* (*backgroundLogic)(void*)) {
  pthread_create(&to, NULL, toLogic, NULL);
  startBackgroundThreads(backgroundLogic);
}

static void joinBackgroundThreads() {
  for (int i = 0; i < Background_Thread_Number; i++)
    pthread_join(tid[i], NULL);
}

void joinThreads() {
  pthread_join(to, NULL);
  joinBackgroundThreads();
}