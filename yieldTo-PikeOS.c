#ifndef __PikeOS__
  #error not on PikeOS
#endif

#include <sys/sched_hook.h>
#include <stdbool.h>
#include <pthread.h>

#include "yieldTo.h"
#include "error.h"
#include "log.h"
#include "scheduling.h"
#include "config.h"
#include "statehandling.h"

static volatile yieldType state;
static volatile bool pendingPreemption;

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

char const * stateName(yieldType state) {
  switch (state) {
    case noYield: return "noYield";
    case explicitYield: return "explicitYield";
    case forcedYield: return "forcedYield";
  }
  return "unkown"; // should be unreachable
}

void switchState(yieldType old, yieldType new) {
  debug(3, "switching state from %s to %s\n", stateName(old), stateName(new));
  if (state != old) error(unexpectedState);
  if (old == new) error(stateUnchanged);
  if (old == forcedYield && new == explicitYield) error(forcedExplicitTransition);
  if (old == explicitYield && new == forcedYield) error(explicitForcedTransition);
  state = new;
}

static void printState(char const *message, pthread_t thread) {
  debug(2, "'%s' %s state boosted=%d, pendingPreemption=%d, %s\n",
        getName(thread), message, boosted(thread), pendingPreemption, stateName(state));
}

static bool isFromOrTo(pthread_t thread) { return (isFrom(thread) || isTo(thread)); }

static int preempt_hook(unsigned __attribute__((unused)) cpu,
                        pthread_t currentThread,
                        pthread_t nextThread) {
  if (isFromOrTo(currentThread)) {
    if (state == explicitYield) {
      debug(2, "kernel saw intentional yield to '%s'\n", getName(nextThread));
    } else if (!Temporarily_Block_Preemption && state == forcedYield) {
      debug(2, "kernel saw yield to '%s'\n", getName(nextThread));
    } else {
      if (Temporarily_Block_Preemption) debug(1, "kernel wants to pre-empt '%s'\n", getName(currentThread));
      else debug(1, "kernel pre-empts '%s'\n", getName(currentThread));
      log(preemptRequest);
      pendingPreemption = true;
      printState("pre-emption with", currentThread);
      return !Temporarily_Block_Preemption; // block pre-emption
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
static bool otherThreadYieldedBackToUs() { return state == explicitYield && boosted(pthread_self()); }
static bool otherThreadWasPreempted() {
  return !Temporarily_Block_Preemption && state == forcedYield && !boosted(pthread_self());
}
static bool otherThreadIsFinished() { return toIsFinished(); }
static bool preemptionWhileYielding() { return state == explicitYield && pendingPreemption; }

static volatile yieldType * incomingYieldFlagForSelf() ;

// This function always yields. It will yield with “yielding” set to true.
static void wrapYield(void (*yieldFunc)(pthread_t), pthread_t thread) {
  switchState(noYield, explicitYield);

  printState("yield in", pthread_self());
  yieldFunc(thread); // go through one yieldTo/yieldBack cycle. This should be the only return point!
  if (pendingPreemption) {
    debug(1, "pre-emption while yielding, all good\n");
    pendingPreemption = false;
    __revert_sched_boost(pthread_self()); // should not yield
  }
  printState("returned in", pthread_self());

  if (otherThreadWasPreempted()) {
    debug(1, "'%s' was preempted. yielding back to it to run into a syncpoint.\n", getName(thread));
    yieldFunc(thread);
    if(!incomingYieldFlagForSelf() == forcedYield) error(notInSyncpoint);
    debug(1, "'%s' returning (with '%s' in syncpoint)\n", selfName(), getName(thread));
  } else if (otherThreadYieldedBackToUs()) {
    debug(1, "'%s' returning\n", selfName());
  } else if (otherThreadIsFinished()) { // nothing to do
  } else error(unexpectedReturn);
  switchState(explicitYield, noYield);
  deboost();
}

static inline void yield(pthread_t thread) {
  pthread_t self = pthread_self();
  if (self == thread) error(yieldToSelf);
  debug(1, "'%s' wants to yield to '%s'\n", selfName(), getName(next()));
  if (!deboosted(pthread_self())) error(mustDeboostSelf);
  if (state != noYield) error(invalidBoostScenario);

  debug(1, "'%s' boosting '%s'\n", selfName(), getName(thread));
  wrapYield(&setBoostPriority, thread); // yields
}

inline void yieldTo() { yield(getTo()); }
inline void yieldBack() { yield(getFrom()); }

static volatile yieldType * incomingYieldFlagForSelf() {
  return (isTo(pthread_self()) ? &fromState->incomingYield : &toState->incomingYield);
}

static void preemptInSyncpoint() {
  debug(2, "'%s' handles pre-emption in syncpoint\n", selfName());
  volatile yieldType * incomingYield = incomingYieldFlagForSelf();
  if(*incomingYield == forcedYield) error(alreadyInSyncpoint);
  *incomingYield = forcedYield;
  debug(1, "forced yield in '%s'\n", selfName());
  debug(2, "non-yielding boost\n");
  setBoostPriority(next());
  debug(1, "'%s' re-allows pre-emption\n", selfName());
  pendingPreemption = false;
  wrapYield(&__revert_sched_boost, pthread_self());
  debug(1, "'%s' returning in syncpoint\n", selfName());
  *incomingYield = noYield;
}

static bool firstYieldTo=true;
inline void syncPoint() {
  if (state == explicitYield && firstYieldTo) { // Todo: only really needed for first yieldTo, not in the greatest spot here
    firstYieldTo=false;
    debug(2, "handling first yieldTo\n");
    switchState(explicitYield, noYield);
    deboost();
  }
  if (pendingPreemption) preemptInSyncpoint();
}