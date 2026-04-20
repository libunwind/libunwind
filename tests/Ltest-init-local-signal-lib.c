#include <stdio.h>
#include "compiler.h"

/* To prevent inlining and optimizing away */
NO_SANITIZE_NULL int foo(volatile int* f) {
  return *f;
}
