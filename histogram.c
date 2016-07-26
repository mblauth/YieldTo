#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include "histogram.h"
#include "error.h"

#define bucketCount 10

/* This provides a simple linear histogram */

static volatile uint64_t minTime = LONG_MAX;
static volatile uint64_t maxTime = 0;
static volatile uint64_t step = 0;
static volatile bool calibrated = false;

static volatile uint64_t buckets[bucketCount] = {0};

static volatile bool firstValue = true;
void calibrate(__uint64_t value) {
  if (firstValue) { // ignore first value
    firstValue = false;
    return;
  }
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
  uint64_t bucket = bucketFor(value);
  if (bucket < 0) bucket = 0;
  else if (bucket > 40) bucket = 40;
  buckets[bucket]++;
}

void printHistogram() {
  printf("number of buckets: %d\n", bucketCount);
  printf("range per bucket: %llu\n", step);
  int total = 0;
  for (int i = 0; i < bucketCount; i++) {
    printf("%llu–%llu: %llu\n", minTime + (i * step), minTime + ((i + 1) * step), buckets[i]);
    total+=buckets[i];
  }
  printf("%d total measurements taken\n", total);
}
