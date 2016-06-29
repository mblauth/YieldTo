#pragma once

enum logEvent {
  yieldToEvent,
  yieldBackEvent,
  toPreemptionEvent,
  fromPreemptionEvent,
  toLoopFinishedEvent,
  fromLoopFinishedEvent,
  preemptRequest
};

typedef struct directionalEvents_t {
  const enum logEvent finished;
  const enum logEvent incomingYield;
  const enum logEvent preemption;
} directionalEvents;

void log(enum logEvent);
