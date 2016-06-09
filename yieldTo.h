#pragma once

#include <pthread.h>
#include <stdbool.h>

#define UNUSED(expr) do { (void)(expr); } while (0)

extern volatile bool yieldedTo;
extern pthread_t to;

void deboost();
void marker();
void registerPreemptionHook();
void setFromId();
void setToId();
void singleCoreOnly();
void yieldBack();
void yieldTo();
