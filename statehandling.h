#pragma once

#include <stdbool.h>
#include "error.h"

typedef volatile enum yieldType_t {
  noYield,       // no yield
  explicitYield, // explicit yieldTo
  forcedYield    // requested by kernel
} yieldType;

typedef struct yieldState_t {
  const directionalEvents logEvents;
  yieldType incomingYield;
} yieldState;

void createFromState();
void createToState();

yieldState * fromState;
yieldState * toState;