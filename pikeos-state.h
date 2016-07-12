#pragma once

#include <stdbool.h>
#include "config.h"

typedef volatile enum pikeos_state_t {
#if !Temporarily_Block_Preemption
  ps_preempted,
#endif
  ps_noPendingYield,
  ps_inExplicitYield,
  ps_inForcedYield,
#if Temporarily_Block_Preemption
  ps_pendingPreemption,
#endif
  ps_preemptedYield
} pikeos_state;

char const * currentState();

bool inExplicitYield();
bool inForcedYield();
#if Temporarily_Block_Preemption
bool isPreemptionPending();
#else
bool wasPreempted();
#endif
bool yieldWasPreempted();
bool notCurrentlyYielding();

void setForcedYield();
void setExplicitYield();
#if Temporarily_Block_Preemption
void setPendingPreemption();
#else
void setPreempted();
#endif
void setYieldFinished();
void setPreemptionRequestHandled();
void setPreemptedInYield();