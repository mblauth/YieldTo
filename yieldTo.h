#pragma once

#include <pthread.h>
#include <stdbool.h>

struct inSync {
  volatile bool from;
  volatile bool to;
};
struct inSync inSync;

void marker(); // optional
void registerPreemptionHook(); // optional
void setFromId();
void setToId();
void singleCoreOnly();
void syncPoint();
void yieldBack();
void yieldTo();
