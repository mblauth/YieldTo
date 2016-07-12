#ifndef __PikeOS__
  #error not on PikeOS
#endif

#include <sys/sched_hook.h>
#include <stdbool.h>
#include <pthread.h>
#include <stdio.h>

#include "yieldTo.h"
#include "error.h"
#include "log.h"
#include "scheduling.h"
#include "config.h"
#include "statehandling.h"
#include "pikeos-state.h"


/*
 * Intentional Yielding: T1(noYield, explicitYield) -y-> T2(explicitYield, noYield)
 * Preemption: T1 -> K(noYield, pendingPreemption) -> T1(pendingPreemption, explicitYield) -y-> T2(explicitYield, noYield)
 */

void marker(){} // not implemented
void setFromId() { registerFrom(pthread_self()); }
void setToId() { registerTo(pthread_self()); }

static void deboost() {
  pthread_t self = pthread_self();
  debug(1, "deboosting '%s'...", selfName());
  setRegularPriority(self); // should never yield according to POSIX
  debug(1, "done\n");
  if (!deboosted(self)) error(deboostError);
}

static void printState(char const *message, pthread_t thread) {
  debug(2, "'%s' %s state boosted=%d, %s\n",
        getName(thread), message, boosted(thread), currentState());
}

static bool isFromOrTo(pthread_t thread) { return (isFrom(thread) || isTo(thread)); }

static int preempt_hook(unsigned __attribute__((unused)) cpu,
                        pthread_t currentThread,
                        pthread_t nextThread) {
  if (isFromOrTo(currentThread)) {
    if (inExplicitYield() || inForcedYield()) {
      debug(2, "kernel saw intentional yield to '%s'\n", getName(nextThread));
    } else if(Temporarily_Block_Preemption && (inExplicitYield() || notCurrentlyYielding())) {
      debug(1, "kernel wants to pre-empt '%s'\n", getName(currentThread));
      log(preemptRequest);
      if (inExplicitYield()) setPreemptedInYield();
      else if (notCurrentlyYielding()) setPendingPreemption();
      else error(unexpectedState);
      printState("kernel pre-emption with", currentThread);
      return false; // block pre-emption
    }
  } else debug(2, "kernel switches from '%s' to '%s'\n", getName(currentThread), getName(nextThread));
  return true; // allow pre-emption
}

void registerPreemptionHook() {
  if (__set_sched_hook(SCHED_PREEMPT_HOOK, preempt_hook) == (__sched_hook_t *) - 1)
    error(preemptionHookRegistrationError);
  debug(1, "registered pre-emption hook\n");
}

static pthread_t next() {
  pthread_t self = pthread_self();
  if (isTo(self)) return getFrom();
  if (isFrom(self)) return getTo();
  error(unknownThread);
  return self;
}

// These functions are only usable directly after a return
static bool otherThreadYieldedBackToUs() { return inExplicitYield() && boosted(pthread_self()); }
static bool otherThreadWasPreempted() {
  return !Temporarily_Block_Preemption && inForcedYield() && !boosted(pthread_self());
}
static bool otherThreadIsFinished() { return toIsFinished(); }

static volatile yieldType * incomingYieldFlagForSelf() ;

// This function always yields. It will yield with “yielding” set to true.
static void wrapYield(void (*yieldFunc)(pthread_t), pthread_t thread) {
  if (notCurrentlyYielding())
    setExplicitYield();
  else if (isPreemptionPending())
    setPreemptedInYield();

  printState("yield in", pthread_self());
  if (yieldWasPreempted()) {
    yieldFunc(thread);
    debug(1, "pre-emption while yielding, all good\n");
    setPreemptionRequestHandled();
    __revert_sched_boost(pthread_self());
  } else {
    yieldFunc(thread);
  }
  printState("returned in", pthread_self());

  if (inForcedYield()) {
    debug(1, "'%s' was forced to yield in syncpoint.\n", getName(thread));
  } else if (otherThreadWasPreempted()) {
    debug(1, "'%s' was preempted. yielding back to it to run into a syncpoint.\n", getName(thread));
    yieldFunc(thread);
    if(!incomingYieldFlagForSelf() == forcedYield) error(notInSyncpoint);
    debug(1, "'%s' returning (with '%s' in syncpoint)\n", selfName(), getName(thread));
  } else if (otherThreadYieldedBackToUs()) {
    debug(1, "'%s' returning\n", selfName());
  } else if (otherThreadIsFinished()) { // nothing to do
  } else error(unexpectedReturn);
  setYieldFinished();
  deboost();
}

static inline void yield(pthread_t thread) {
  pthread_t self = pthread_self();
  if (self == thread) error(yieldToSelf);
  debug(1, "'%s' wants to yield to '%s'\n", selfName(), getName(next()));
  if (!deboosted(pthread_self())) error(mustDeboostSelf);

  debug(1, "'%s' boosting '%s'\n", selfName(), getName(thread));
  wrapYield(&setBoostPriority, thread); // yields
}

inline void yieldTo() { syncPoint(); yield(getTo()); }
inline void yieldBack() { syncPoint(); yield(getFrom()); }

static volatile yieldType * incomingYieldFlagForSelf() {
  return (isTo(pthread_self()) ? &fromState->incomingYield : &toState->incomingYield);
}

static void preemptInSyncpoint() {
  debug(2, "'%s' handles pre-emption in syncpoint\n", selfName());
  volatile yieldType * incomingYield = incomingYieldFlagForSelf();
  if(*incomingYield == forcedYield) error(alreadyInSyncpoint);
  *incomingYield = forcedYield;
  debug(1, "forced yield in '%s'\n", selfName());
  setForcedYield();
  debug(2, "non-yielding boost\n");
  setBoostPriority(next());
  debug(1, "'%s' wants to re-allow pre-emption\n", selfName());
  wrapYield(&__revert_sched_boost, pthread_self());
  debug(1, "'%s' returning in syncpoint\n", selfName());
  *incomingYield = noYield;
}

static bool firstYieldTo=true;
inline void syncPoint() {
  if (inExplicitYield() && firstYieldTo) { // Todo: only really needed for first yieldTo, not in the greatest spot here
    firstYieldTo=false;
    debug(2, "handling first yieldTo\n");
    setYieldFinished();
    deboost();
  }
  if (isPreemptionPending()) preemptInSyncpoint();
}