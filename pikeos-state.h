#pragma once

#include <stdbool.h>
#include "config.h"

typedef volatile enum pikeos_state_t {
  ps_noPendingYield,
  ps_inExplicitYield,
  ps_inForcedYield,
  ps_pendingPreemption,
  ps_preemptedYield
} pikeos_state;

char const * currentState();

bool inExplicitYield();
bool inForcedYield();
bool isPreemptionPending();
bool yieldWasPreempted();
bool notCurrentlyYielding();

void setForcedYield();
void setExplicitYield();
void setPendingPreemption();
void setYieldFinished();
void setPreemptionRequestHandled();
void setPreemptedInYield();