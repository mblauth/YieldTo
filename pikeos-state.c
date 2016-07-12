#include <stdbool.h>

#include "pikeos-state.h"
#include "error.h"

static pikeos_state state = ps_noPendingYield;

static char const * stateName(pikeos_state state) {
  switch (state) {
    case ps_noPendingYield: return "no yield";
    case ps_inExplicitYield: return "explicit yield";
    case ps_inForcedYield: return "forced yield";
    case ps_pendingPreemption: return "pending preemption";
    case ps_preemptedYield: return "preempted yield";
  }
  return "unknown"; // should be unreachable
}

char const * currentState() {
  return stateName(state);
}

bool inExplicitYield() {
  return state == ps_inExplicitYield;
}

bool inForcedYield() {
  return state == ps_inForcedYield;
}

bool isPreemptionPending() {
  return state == ps_pendingPreemption;
}

bool yieldWasPreempted() {
  return state == ps_preemptedYield;
}

bool notCurrentlyYielding() {
  return state == ps_noPendingYield;
}

/* returns false when it was preempted */
static bool switchState(pikeos_state old, pikeos_state new) {
  if (old == new) error(stateUnchanged);
  debug(2, "switching state from '%s' to '%s', currently set to '%s'\n",
        stateName(old), stateName(new), currentState());
  bool result = __sync_bool_compare_and_swap(&state, old, new);
  if (!result) debug(2, "cas failed, apparently we were pre-empted\n");
  return result;
}

void setForcedYield() {
  if (state == ps_inExplicitYield) error(explicitForcedTransition);
  if (!switchState(ps_pendingPreemption, ps_inForcedYield)) error(unexpectedState);
}

void setExplicitYield() {
  if (state == ps_inForcedYield) error(forcedExplicitTransition);
  if (!switchState(ps_noPendingYield, ps_inExplicitYield)) switchState(ps_pendingPreemption, ps_preemptedYield);
}

void setPendingPreemption() {
  if(!switchState(ps_noPendingYield, ps_pendingPreemption)) error(unexpectedState);
}

void setPreemptedInYield() {
  if (state == ps_inExplicitYield) {
    if (!switchState(ps_inExplicitYield, ps_preemptedYield)) error(unexpectedState);
  } else if (state == ps_pendingPreemption) {
    if (!switchState(ps_pendingPreemption, ps_preemptedYield)) error(unexpectedState);
  } else error(unexpectedState);
}

void setYieldFinished() {
  if (inExplicitYield()) {
    if (!switchState(ps_inExplicitYield, ps_noPendingYield)) error(unexpectedState);
  } else if (inForcedYield()) {
    if (!switchState(ps_inForcedYield, ps_noPendingYield)) error(unexpectedState);
  } else error(unexpectedState);
}

void setPreemptionRequestHandled() {
  if (isPreemptionPending()) switchState(ps_pendingPreemption, ps_inExplicitYield);
  else if (yieldWasPreempted()) switchState(ps_preemptedYield, ps_inExplicitYield);
  else error(unexpectedState);
}