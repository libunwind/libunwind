#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

extern int backtrace (void **, int);

static void
b (void)
{
  void *v[20];
  int i, n;

  n = backtrace(v, 20);
  for (i = 0; i < n; ++i)
    printf ("[%d] %p\n", i, v[i]);
}

static void
a (void)
{
  b();
}

static void c (void) __attribute__((constructor));

static void
c (void)
{
  b();
}

int
main (int argc, char **argv)
{
  return atexit (a);
}
