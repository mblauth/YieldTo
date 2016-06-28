#pragma once

#include <stdbool.h>
#include "error.h"

enum yieldDirection {
  from_To_to, To_to_from
};

typedef struct yieldState_t {
  const enum yieldDirection direction;
  const directionalEvents logEvents;
  volatile bool incomingYield;
  volatile bool otherInSyncpoint;
} yieldState;

void createFromState();
void createToState();

yieldState * fromState;
yieldState * toState;