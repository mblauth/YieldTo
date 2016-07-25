#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdarg.h>
#include <limits.h>

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
    case threadAttributeError:
      return "failed to initialize thread attributes";
    case policyError:
      return "failed to set scheduling policy";
    case prioritySetFailed:
      return "failed to set realtime priority";
    case priorityGetFailed:
      return "failed to set realtime priority";
    case priorityAlreadySet:
      return "priority already set";
    case alreadyBoosted:
      return "thread is already boosted";
    case mustDeboostSelf:
      return "thread must deboost itself before boosting another thread";
    case unexpectedReturn:
      return "unexpected return to thread";
    case invalidBoostScenario:
      return "invalid boost scenario";
    case notInSyncpoint:
      return "not in syncpoint";
    case alreadyYielding:
      return "already in a yield";
    case alreadyDeboosted:
      return "already deboosted";
    case invalidLogType:
      return "should not be in a syncpoint";
    case unexpectedState:
      return "unexpected state";
    case stateUnchanged:
      return "state unchanged";
    case forcedExplicitTransition:
      return "direct transition from forced to explicit yield";
    case explicitForcedTransition:
      return "direct transition from explicit to forced yield";
    case yieldFailed:
      return "yield failed";
    case alreadyCalibrated:
      return "histogram already calibrated";
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

void status(char const * message) {
  printf("\n*** %s ***\n\n", message);
}
