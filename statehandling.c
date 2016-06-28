#include <stdlib.h>
#include <string.h>
#include "error.h"
#include "statehandling.h"

yieldState fromStateInternal = {
    .logEvents={
        .finished=fromLoopFinishedEvent,
        .incomingYield=yieldBackEvent,
        .preemption=toPreemptionEvent},
    .incomingYield=noYield
};

yieldState toStateInternal = {
    .logEvents={
        .finished=toLoopFinishedEvent,
        .incomingYield=yieldToEvent,
        .preemption=fromPreemptionEvent},
    .incomingYield=noYield
};

void createFromState() {
  fromState = malloc(sizeof(yieldState));
  memcpy(fromState, &fromStateInternal, sizeof(yieldState));
}

void createToState() {
  toState = malloc(sizeof(yieldState));
  memcpy(toState, &toStateInternal, sizeof(yieldState));
}