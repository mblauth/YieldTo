#pragma once

#include <pthread.h>
#include <stdbool.h>

volatile bool fromInSync;
volatile bool toInSync;

void deboost(); // optional
void marker(); // optional
void registerPreemptionHook(); // optional
void setFromId();
void setToId();
void singleCoreOnly();
void syncPoint();
void yieldBack();
void yieldTo();
