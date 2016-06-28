#pragma once

#include "config.h"

pthread_t tid[Background_Thread_Number];
pthread_t to;

void startThreads(void*(*)(void*),void*(*)(void*));
void joinThreads();