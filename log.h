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

enum logType {
  simple_ascii_log,
  benchmark_log
};

typedef struct directionalEvents_t {
  const enum logEvent finished;
  const enum logEvent incomingYield;
  const enum logEvent preemption;
} directionalEvents;

void log(enum logEvent);
