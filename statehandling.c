#include <stdlib.h>
#include <string.h>

#include "error.h"
#include "log.h"
#include "statehandling.h"
#include "yieldTo.h"

yieldState fromStateInternal = {
    .logEvents={
        .finished=fromLoopFinishedEvent,
        .incomingYield=yieldBackEvent,
        .preemption=toPreemptionEvent},
    .incomingYield=noYield,
    .yieldTo=&yieldTo,
    .thread=fromThread
};

yieldState toStateInternal = {
    .logEvents={
        .finished=toLoopFinishedEvent,
        .incomingYield=yieldToEvent,
        .preemption=fromPreemptionEvent},
    .incomingYield=noYield,
    .yieldTo=&yieldBack,
    .thread=toThread
};

void createFromState() {
  fromState = malloc(sizeof(yieldState));
  memcpy(fromState, &fromStateInternal, sizeof(yieldState));
}

void createToState() {
  toState = malloc(sizeof(yieldState));
  memcpy(toState, &toStateInternal, sizeof(yieldState));
}