#include "test_racing.h"
#include "tools.h"
#include "hsail_runtime.h"
#include "hsail_finalize.h"
#include "test_suite.h"
#include <stdio.h>
#include <string.h>

int test_racing_simple(char *in, char *out, hsail_runtime_t* run) {
  // CREATE program
  //LOAD module
  // Create signals
  //analyse output
  hsail_module_t* start = NULL;
  new_test_module(&start, "test_full");
  new_test_module(&start, "test_full_2");

  hsail_finalize_t fin;

  //finalize_module(start, run, &fin);

  return 0;
}
