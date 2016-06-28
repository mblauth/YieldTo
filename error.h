#pragma once

enum errorcode {
  noError=0,
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
  alreadyDeboosted=18,
  alreadyBoosted=19,
  mustDeboostSelf=20,
  yieldBeforeDeboost=21,
  notDeboosted=22,
  stillBoosted=23,
  alreadyYielding=24,
  inSyncpoint=25
};

enum failcode {
  yieldToFail=100,
  yieldBackFail=101,
  notStarved=102
};

enum failThread {
  backgroundThread,
  fromThread,
  toThread
};

enum logEvent {
  yieldToEvent,
  yieldBackEvent,
  toPreemptionEvent,
  fromPreemptionEvent,
  toLoopFinishedEvent,
  fromLoopFinishedEvent
};

typedef struct directionalEvents_t {
  enum logEvent finished;
  enum logEvent incomingYield;
  enum logEvent preemption;
} directionalEvents;

directionalEvents toEvents;
directionalEvents fromEvents;

void error(enum errorcode);
void fail(enum failcode, enum failThread);
void debug(int level, const char *, ...);
void status(const char *);
void log(enum logEvent);
