#pragma once

#include <pthread.h>
#include <stdbool.h>

extern volatile bool inSync;

void deboost(); // optional
void marker(); // optional
void registerPreemptionHook(); // optional
void setFromId();
void setToId();
void singleCoreOnly();
void syncPoint();
void yieldBack();
void yieldTo();
