#ifndef YIELDTO_H
#define YIELDTO_H

#define UNUSED(expr) do { (void)(expr); } while (0)

void deboost();
void marker();
void registerPreemptionHook();
void setFromId();
void setToId();
void singleCoreOnly();
void yieldBack();
void yieldTo();

#endif //YIELDTO_H
