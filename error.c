#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdarg.h>

#include "error.h"
#include "config.h"

static char* errorStringFor(enum errorcode code) {
  switch (code) {
    case boostError:
      return "boost failed";
    case deboostError:
      return "deboost failed";
    case unknownThread:
      return "running unknown thread";
    case preemptionHookRegistrationError:
      return "failed to register pre-emption hook";
    case yieldToSelf:
      return "Tried to yield to self";
    case alreadyInSyncpoint:
      return "we are already in a syncpoint";
    case fromAlreadySet:
      return "from is already set";
    case toAlreadySet:
      return "to is already set";
    case fromAndToTheSame:
      return "from and to are the same";
    case noError:
      return "no error";
    case threadAttributeError:
      return "failed to initialize thread attributes";
    case policyError:
      return "failed to set scheduling policy";
    case prioritySetFailed:
      return "failed to set realtime priority";
    case priorityGetFailed:
      return "failed to set realtime priority";
    case alreadyDeboosted:
      return "thread is already deboosted";
    case alreadyBoosted:
      return "thread is already boosted";
    case mustDeboostSelf:
      return "thread must deboost itself before boosting another thread";
    case yieldBeforeDeboost:
      return "tried to yield before deboosting self";
    case notDeboosted:
      return "we were not deboosted";
    case stillBoosted:
      return "other thread is still boosted";
    case alreadyYielding:
      return "already in a yield";
    case inSyncpoint:
      return "should not be in a syncpoint";
  }
  return "unknown error"; // should be unreachable
}

void error(enum errorcode code) {
  printf("Error %d: %s\n", code, errorStringFor(code));
  exit(code);
}

static char* failureStringFor(enum failcode code) {
  switch (code) {
    case notStarved:
      return "thread was not starved";
    case yieldToFail:
      return "yieldTo failed";
    case yieldBackFail:
      return "yieldBack failed";
  }
  return "unkown failure"; // should be unreachable
}

static char* failThreadToString(enum threadType thread) {
  switch (thread) {
    case backgroundThread:
      return "background thread";
    case fromThread:
      return "from";
    case toThread:
      return "to";
  }
  return "unkown thread"; // should be unreachable
}

void fail(enum failcode code, enum threadType thread) {
  printf("%s, currently in %s\n", failureStringFor(code), failThreadToString(thread));
  exit(code);
}

void debug(int level, const char * format, ...) {
  if (Debug_Output && level <= Debug_Level) {
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
  }
}

void status(const char * message) {
  printf("\n*** %s ***\n\n", message);
}

#if Log_Type == Simple_ascii_log
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
  }
  fflush(stdout);
}
#endif