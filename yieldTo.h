#ifndef YIELDTO_H
#define YIELDTO_H

#include "config.h"

#define UNUSED(expr) do { (void)(expr); } while (0)

#ifdef __linux__
  typedef long thread_id;
#else // PikeOS, potentially others
  typedef pthread_t thread_id;
#endif

volatile thread_id toId;

void marker();
void registerPreemptionHook();
void setToId();
void singleCoreOnly();
long yieldTo(thread_id const id);

#endif //YIELDTO_H
