#include <stdio.h>
#include <stdlib.h>
#include "error.h"

void error(enum errorcode code, const char * message) {
  printf("Error %d: %s\n", code, message);
  exit(code);
}

void fail(enum failcode code) {
  printf("yieldTo failed\n");
  exit(code);
}