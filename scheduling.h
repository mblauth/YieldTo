#pragma once

#include <pthread.h>
#include <stdbool.h>

bool boosted(pthread_t);
bool deboosted(pthread_t);
void setBoostPriority(pthread_t);
void setRegularPriority(pthread_t);
void setDefaultRealtimeParameters();
void registerFrom(pthread_t);
void registerTo(pthread_t);
bool isFrom(pthread_t);
bool isTo(pthread_t);
char const * getName(pthread_t);
char const * selfName();
pthread_t getFrom();
pthread_t getTo();