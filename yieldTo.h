#pragma once

#include <pthread.h>
#include <stdbool.h>

void marker(); // optional
void registerPreemptionHook(); // optional
void setFromId();
void setToId();
void singleCoreOnly();
void syncPoint();
void yieldBack();
void yieldTo();
bool toIsFinished();
