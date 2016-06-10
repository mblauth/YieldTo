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
  yieldToSelfError=13,
  prioritySetFailed=14
};

enum failcode {
  yieldToFail=100,
  yieldBackFail=101,
  notStarved=102
};

void error(enum errorcode code, const char * message);
void fail(enum failcode code, const char * message);
