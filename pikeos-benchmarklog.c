#include <limits.h>
#include <sys/timespec.h>
#include <time.h>
#include <stdio.h>
#include <ddapi/ddapi.h>

#include "log.h"

static volatile long minTimeMus = LONG_MAX;
static volatile long maxTimeMus = 0;

static volatile long startTimeMus;

static void handlePreemptRequest() {
  startTimeMus = (volatile long) (dd_get_time_raw() / 1000); // PikeOS-specific
}

void handlePreemptionEvent() {
  long endTimeMus = (volatile long) (dd_get_time_raw() / 1000);
  long musDiff = endTimeMus - startTimeMus;
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
      printf("Max: %ld, Min: %ld\n", maxTimeMus, minTimeMus);
      maxTimeMus = 0;
      minTimeMus = LONG_MAX;
      return;
    default: ; // ignore other events
  }
}