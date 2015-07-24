#include <stdbool.h>
#include <sched.h>

#include "barrier.h"

static volatile bool barrierReached = false;
static volatile bool firstWaiting = false;

void initBarrier() {} // not needed
void destroyBarrier() {} // not needed

void waitAtBarrier() {
  if (firstWaiting) {
    barrierReached = true;
    return;
  } else {
    firstWaiting = true;
    while (!barrierReached) sched_yield();
  }
}