#include <stdio.h>
#include <bits/pthreadtypes.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <stdbool.h>

#define POSIX_yield 0
#define Linux_yieldTo 1

#define Yield_Policy Linux_yieldTo
#define Scheduling_Policy SCHED_FIFO
#define Background_Thread_Number 20
#define Load_Factor 5


#ifndef __NR_sched_yieldTo
#define __NR_sched_yieldTo 323
#endif

#define UNUSED(expr) do { (void)(expr); } while (0)

static pthread_t tid[Background_Thread_Number];
static pthread_t to;
static pthread_barrier_t barrier;

static volatile bool yieldedTo = false;
static volatile long t2_id = 0;

static void createBackgroundThreads();

static void joinBackgroundThreads();

static void singleCoreOnly();

static void *toLogic(void *);

static void setPolicy();

#if Yield_Policy == POSIX_yield
static long yieldTo(long const ignored) { // not a yieldTo(), just here for testing
    UNUSED(ignored);
    sched_yield();
    return 0;
}
#elif Yield_Policy == Linux_yieldTo

static long yieldTo(long const id) {
    if (!id) {
        perror("could not find pid\n");
        exit(2);
    }
    return syscall(__NR_sched_yieldTo, id);
}

#else
  #error "Unknown Scheduling Policy"
#endif


void main(int argc, char *argv[]) {
    singleCoreOnly();
    setPolicy();
    pthread_barrier_init(&barrier, NULL, 2);
    pthread_create(&to, NULL, &toLogic, NULL);
    createBackgroundThreads();
    pthread_barrier_wait(&barrier);
    syscall(SYS_gettid); // just used as a marker for ftrace
    yieldedTo = true;
    long const result = yieldTo(t2_id);
    yieldedTo = false;
    syscall(SYS_gettid);
    pthread_join(to, NULL);
    pthread_barrier_destroy(&barrier);
    printf("yieldTo() returned %ld\n", result);
    joinBackgroundThreads();
}

static void *toLogic(void *ignored) {
    UNUSED(ignored);
    t2_id = syscall(SYS_gettid);
    pthread_barrier_wait(&barrier);
    sched_yield();
    syscall(SYS_gettid); // just used as a marker for ftrace
    bool worked = false;
    for (int i = 0; i < Load_Factor * 10; i++)
        for (unsigned long k = 0; k < 0xffffff; k++)
            if (yieldedTo && !worked) {
                worked = true;
                printf("yieldTo worked!\n");
            }
    if (!worked) printf("yieldTo did not work\n");
    return NULL;
}

static void *busy(void *ignored) {
    UNUSED(ignored);
    for (int i = 0; i < Load_Factor; i++)
        for (unsigned long k = 0; k < 0xffffff; k++)
            yieldedTo = false;
    return NULL;
}

static void singleCoreOnly() {
    cpu_set_t cpuSet;
    CPU_ZERO(&cpuSet);
    CPU_SET(0, &cpuSet);

    int s = pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuSet);
    if (s != 0) {
        perror("could not set affinity");
        exit(1);
    }
}

static void joinBackgroundThreads() {
    for (int i = 0; i < Background_Thread_Number; i++)
        pthread_join(tid[i], NULL);
}

static void createBackgroundThreads() {
    printf("creating %i background threads...", Background_Thread_Number);
    for (int i = 0; i < Background_Thread_Number; i++)
        pthread_create(&tid[i], NULL, &busy, NULL);
    printf("done\n");
}

static void setPolicy() {
#if Scheduling_Policy == SCHED_FIFO || Scheduling_Policy == SCHED_RR
    printf("setting realtime policy\n");
    struct sched_param param;
    param.__sched_priority = 75;
    if (sched_setscheduler(0, Scheduling_Policy, &param)) {
        printf("could not set realtime policy\n");
        exit(2);
    }
#endif
}