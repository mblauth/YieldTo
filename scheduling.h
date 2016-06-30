#pragma once

int getPriority(pthread_t thread);
void setBoostPriority(pthread_t thread);
void setRegularPriority(pthread_t thread);
void setDefaultRealtimeParameters();
