#pragma once

// hardcoded errorcodes for systems without stdout
enum errorcode {
  threadAttributeError=3,
  policyError=4,
  preemptionHookRegistrationError=6,
  boostError=7,
  deboostError=8,
  fromAlreadySet=9,
  toAlreadySet=10,
  fromAndToTheSame=11,
  yieldToSelf=13,
  prioritySetFailed=14,
  unknownThread=15,
  alreadyInSyncpoint=16,
  priorityGetFailed=17,
  priorityAlreadySet=18,
  alreadyBoosted=19,
  mustDeboostSelf=20,
  unexpectedReturn=21,
  invalidBoostScenario=22,
  notInSyncpoint=23,
  alreadyYielding=24,
  alreadyDeboosted=25,
  invalidLogType=26,
  unexpectedState=27,
  stateUnchanged=28,
  forcedExplicitTransition=29,
  explicitForcedTransition=30,
  yieldFailed=31
};

enum failcode {
  yieldToFail=100,
  yieldBackFail=101,
  notStarved=102
};

enum threadType {
  backgroundThread,
  fromThread,
  toThread
};

void error(enum errorcode);
void fail(enum failcode, enum threadType);
void debug(int level, const char *, ...);
void status(char const *);
