#include <stdio.h>

/* To prevent inlining and optimizing away */
int foo(int* f) {
  return *f;
}
