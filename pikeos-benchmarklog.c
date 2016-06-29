#include <limits.h>
#include <sys/timespec.h>
#include <stdio.h>
#include <ddapi/ddapi.h>

#include "log.h"

static volatile uint64_t minTimeMus = LONG_MAX;
static volatile uint64_t maxTimeMus = 0;

static volatile uint64_t startTimeMus;

static uint64_t getTimeInMus() { return (dd_get_time_raw() / 1000); }

static void handlePreemptRequest() {
  startTimeMus = getTimeInMus(); // PikeOS-specific
}

static void handlePreemptionEvent() {
  uint64_t endTimeMus = getTimeInMus();
  uint64_t musDiff = endTimeMus - startTimeMus;
  if (minTimeMus > musDiff) minTimeMus = musDiff;
  if (maxTimeMus < musDiff) maxTimeMus = musDiff;
}

void log(enum logEvent event) {
  switch (event) {
    case preemptRequest:
      return handlePreemptRequest();
    case fromPreemptionEvent:
    case toPreemptionEvent:
      return handlePreemptionEvent();
    case toLoopFinishedEvent:
      printf("Max: %llu, Min: %llu\n", maxTimeMus, minTimeMus);
      maxTimeMus = 0;
      minTimeMus = UINT64_MAX;
      return;
    default: ; // ignore other events
  }
}