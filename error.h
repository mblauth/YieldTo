#pragma once

enum errorcode {
  noError=0,
  boostError=7,
  deboostError=8,
  fromAlreadySet=9,
  toAlreadySet=10,
  fromAndToTheSame=11,
  yieldToSelfError=13,
  prioritySetFailed=14
};

enum failcode {
  yieldToFail=100
};

void error(enum errorcode code, const char * message);
void fail(enum failcode code);
