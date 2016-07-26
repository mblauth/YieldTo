#include <limits.h>
#include <sys/timespec.h>
#include <stdio.h>
#include <ddapi/ddapi.h>

#include "histogram.h"
#include "log.h"

static volatile uint64_t startTimeMus;
static volatile int calibrationRuns = 10;

static uint64_t getTimeInMus() { return (dd_get_time_raw() / 1000); }

static void handlePreemptRequest() {
  startTimeMus = getTimeInMus(); // PikeOS-specific
}

static void handlePreemptionEvent() {
  uint64_t endTimeMus = getTimeInMus();
  uint64_t musDiff = endTimeMus - startTimeMus;

  if (calibrationRuns-- > 0) calibrate(musDiff);
  else logValue(musDiff);
}

void log(enum logEvent event) {
  switch (event) {
    case preemptRequest:
      return handlePreemptRequest();
    case fromPreemptionEvent:
    case toPreemptionEvent:
      return handlePreemptionEvent();
    default: ; // ignore other events
  }
}