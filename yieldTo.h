#ifndef YIELDTO_H
#define YIELDTO_H

#define UNUSED(expr) do { (void)(expr); } while (0)

void marker();
void registerPreemptionHook();
void setToId();
void singleCoreOnly();
long yieldTo();

#endif //YIELDTO_H
