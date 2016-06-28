#pragma once

#include <stdbool.h>
#include "error.h"

typedef struct yieldState_t {
  const directionalEvents logEvents;
  volatile bool incomingYield;
  volatile bool otherInSyncpoint;
} yieldState;

void createFromState();
void createToState();

yieldState * fromState;
yieldState * toState;