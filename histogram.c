#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include "histogram.h"
#include "error.h"

#define bucketCount 40

/* This provides a simple linear histogram */

static volatile uint64_t minTime = LONG_MAX;
static volatile uint64_t maxTime = 0;
static volatile uint64_t step = 0;
static volatile bool calibrated = false;

static volatile uint64_t buckets[bucketCount] = {0};

void calibrate(__uint64_t value) {
  if (calibrated) error(alreadyCalibrated);
  if (value < minTime) minTime = value;
  if (value > maxTime) maxTime = value;
}

uint64_t bucketFor(uint64_t value) {
  return (value-minTime)/step;
}

void logValue(uint64_t value) {
  if (!calibrated) {
    step = (maxTime-minTime)/bucketCount;
    calibrated = true;
  }
  buckets[bucketFor(value)]++;
}

void printHistogram() {
  printf("number of buckets: %d\n", bucketCount);
  printf("range per bucket: %llu\n", step);
  for (int i = 0; i < bucketCount; i++)
    printf("%llu–%llu: %llu\n", minTime+(i*step), minTime+((i+1)*step), buckets[i]);
}
