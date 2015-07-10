#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>


#ifdef __linux__

#include <sys/syscall.h>

#define HaveBarriers 1
#endif
#ifdef __PikeOS__
#include <sys/sched_hook.h>
#define HaveBarriers 0 // not so posix...
#endif

#define POSIX_yield 0
#define Linux_yieldTo 1
#define PikeOS_hook 2

#define Yield_Policy Linux_yieldTo
#define Scheduling_Policy SCHED_RR
#define Realtime_Priority 75
#define Background_Thread_Number 20
#define Load_Factor 5


#ifndef __NR_sched_yieldTo
#define __NR_sched_yieldTo 323
#endif

#define UNUSED(expr) do { (void)(expr); } while (0)

static pthread_t tid[Background_Thread_Number];
static pthread_t to;
#if HaveBarriers
static pthread_barrier_t barrier;
#else
static volatile bool toInitialized = false;
#endif

static volatile bool yieldedTo = false;
#ifdef __linux__
static volatile long toId = 0;
#else
static volatile pthread_t toId = 0;
#endif
static volatile bool toFinished = false;

static void createBackgroundThreads();
static void joinBackgroundThreads();
static void singleCoreOnly();
static void *toLogic(void *);
static void setRealtimeParameters(pthread_t thread);
static void marker();

#if Yield_Policy == POSIX_yield
static inline long yieldTo(long const ignored) { // not a yieldTo(), just here for testing
    UNUSED(ignored);
    sched_yield();
    return 0;
}
#elif Yield_Policy == Linux_yieldTo

static inline long yieldTo(long const id) {
    if (!id) {
        perror("could not find thread id\n");
        exit(2);
    }
    return syscall(__NR_sched_yieldTo, id);
}

#elif Yield_Policy == PikeOS_hook
// works via a preemption-notification, not a direct yieldTo
static volatile bool preempted = false;

static int preempt_hook(unsigned cpu, pthread_t t_old, pthread_t t_new) {
    UNUSED(cpu); UNUSED(t_old); UNUSED(t_new);
    if (yieldedTo) return false;
    preempted = true;
    return true;
}

static inline long yieldTo(pthread_t const to) {
    printf("boosting\n");
    struct sched_param param = { .sched_priority = Realtime_Priority + 1 };
    if (pthread_setschedparam(to, Scheduling_Policy, &param) != 0) {
        printf("boosting failed\n");
        exit(7);
    }
    if (preempted) {
        preempted = false;
        printf("re-allowing preemption\n");
        __revert_sched_boost(pthread_self());
    }
    return 0;
}

#else
  #error "Unknown Scheduling Policy"
#endif


void main(int argc, char *argv[]) {
    singleCoreOnly();
    setRealtimeParameters(pthread_self());
#if HaveBarriers
    pthread_barrier_init(&barrier, NULL, 2);
#endif
    pthread_create(&to, NULL, &toLogic, NULL);
    createBackgroundThreads();
#if Yield_Policy == PikeOS_hook
    if (__set_sched_hook(SCHED_PREEMPT_HOOK, preempt_hook) == (__sched_hook_t * ) - 1) {
        printf("failed to register preemption hook\n");
        exit(6);
    }
    printf("registered preemption hook\n");
    #endif
#if HaveBarriers
    pthread_barrier_wait(&barrier);
#else
    while(!toInitialized) sched_yield();
#endif
    marker();
    while (!toFinished) {
        for (unsigned long k = 0; k < 0xffffff; k++) yieldedTo = false;
        yieldedTo = true;
        long const result = yieldTo(toId);
        yieldedTo = false;
        printf("yieldTo() returned %ld\n", result);
    }
    pthread_join(to, NULL);
#if HaveBarriers
    pthread_barrier_destroy(&barrier);
#endif
    joinBackgroundThreads();
}

static inline void marker() {
#ifdef __linux__
    syscall(SYS_gettid); // just used as a marker for ftrace
#endif
}

static void *toLogic(void *ignored) {
    UNUSED(ignored);
#ifdef __linux__
    toId = syscall(SYS_gettid);
#else
    toId = pthread_self();
    #endif
#if HaveBarriers
    pthread_barrier_wait(&barrier);
#else
    toInitialized = true;
    #endif
    sched_yield();
    marker();
    for (int i = 0; i < Load_Factor * 10; i++)
        for (unsigned long k = 0; k < 0xffffff; k++) {
            if (yieldedTo) {
                printf("yieldTo worked!\n");
                yieldedTo = false;
                #ifdef __PikeOS__
                printf("deboosting\n");
                struct sched_param param = { .sched_priority = Realtime_Priority };
                if (pthread_setschedparam(to, Scheduling_Policy, &param) != 0) {
                    printf("deboosting failed\n");
                    exit(8);
                }
                #endif
            }
        }
    printf("yieldTo target finished execution\n");
    toFinished = true;
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
#ifdef __linux__
    cpu_set_t cpuSet;
    CPU_ZERO(&cpuSet);
    CPU_SET(0, &cpuSet);

    int s = pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuSet);
    if (s != 0) {
        perror("could not set affinity");
        exit(1);
    }
#endif
#ifdef __PikeOS__
    cpuset_t cpuSet;
    CPU_ZERO(&cpuSet);
    CPU_SET(0, &cpuSet);

    int s = pthread_setaffinity_np(pthread_self(), &cpuSet);
    if (s != 0) {
        perror("could not set affinity");
        exit(1);
    }
    #endif
}

static void joinBackgroundThreads() {
    for (int i = 0; i < Background_Thread_Number; i++)
        pthread_join(tid[i], NULL);
}

static void createBackgroundThreads() {
    printf("creating %i background threads...", Background_Thread_Number);
    pthread_attr_t attr;
    if (pthread_attr_init(&attr) != 0) {
        printf("thread attr init failure");
        exit(3);
    }
    if (pthread_attr_setschedpolicy(&attr, Scheduling_Policy)) {
        printf("could not set realtime policy\n");
        exit(4);
    }
    struct sched_param param = { .sched_priority = Realtime_Priority };
    if (pthread_attr_setschedparam(&attr, &param)) {
        printf("could not set realtime priority\n");
        exit(4);
    }
    for (int i = 0; i < Background_Thread_Number; i++)
        pthread_create(&tid[i], &attr, &busy, NULL);
    pthread_attr_destroy(&attr);
    printf("done\n");
}

static void setRealtimeParameters(pthread_t thread) {
#if Scheduling_Policy == SCHED_FIFO || Scheduling_Policy == SCHED_RR
    printf("setting realtime parameters\n");
    struct sched_param param = { .sched_priority = Realtime_Priority };
    if (pthread_setschedparam(thread, Scheduling_Policy, &param)) {
        printf("could not set realtime parameters\n");
        exit(5);
    }
#endif
}