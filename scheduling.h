#pragma once

int getPriority(pthread_t thread);
enum errorcode setPriority(pthread_t thread, int priority);
void setDefaultRealtimeParameters();
