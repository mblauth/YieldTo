#include <stdio.h>
#include "log.h"

void log(enum logEvent event) {
  switch (event) {
    case yieldToEvent:
      printf(">"); break;
    case yieldBackEvent:
      printf("<"); break;
    case toPreemptionEvent:
      printf("+"); break;
    case fromPreemptionEvent:
      printf("-"); break;
    case fromLoopFinishedEvent:
      printf("]"); break;
    case toLoopFinishedEvent:
      printf(")"); break;
    case preemptRequest:
      printf("~"); break;
  }
  fflush(stdout);
}