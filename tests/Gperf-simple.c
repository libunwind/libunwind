/* libunwind - a platform-independent unwind library
   Copyright (C) 2003-2004 Hewlett-Packard Co
	Contributed by David Mosberger-Tang <davidm@hpl.hp.com>

Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be
included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.  */

#include <memory.h>
#include <stdio.h>
#include <stdlib.h>

#include <time.h>

#include <libunwind.h>

#define panic(args...)							  \
	do { fprintf (stderr, args); exit (-1); } while (0)

long dummy;

static long iterations = 10000;
static int maxlevel = 100;

#define MB	(1024*1024)

static char big[128*MB];

static inline double
gettime (void)
{
  struct timespec ts;

  clock_gettime(CLOCK_REALTIME, &ts);
  return ts.tv_sec + 1e-9*ts.tv_nsec;
}

static int
measure_unwind (int maxlevel, double *step)
{
  double stop, start;
  unw_cursor_t cursor;
  unw_context_t uc;
  int ret, level = 0;

  unw_getcontext (&uc);
  if (unw_init_local (&cursor, &uc) < 0)
    panic ("unw_init_local() failed\n");

  start = gettime ();

  do
    {
      ret = unw_step (&cursor);
      if (ret < 0)
	panic ("unw_step() failed\n");
      ++level;
    }
  while (ret > 0);

  stop = gettime ();

  if (level <= maxlevel)
    panic ("Unwound only %d levels, expected at least %d levels",
	   level, maxlevel);

  *step = (stop - start) / (double) level;
  return 0;
}

static int
f1 (int level, int maxlevel, double *step)
{
  if (level == maxlevel)
    return measure_unwind (maxlevel, step);
  else
    /* defeat last-call/sibcall optimization */
    return f1 (level + 1, maxlevel, step) + level;
}

static void
doit (const char *label)
{
  double step, min_step, first_step, sum_step;
  int i;

  sum_step = first_step = 0.0;
  min_step = 1e99;
  for (i = 0; i < iterations; ++i)
    {
      f1 (0, maxlevel, &step);

      sum_step += step;

      if (step < min_step)
	min_step = step;

      if (i == 0)
	first_step = step;
    }
  printf ("%s: unw_step : 1st=%9.3f min=%9.3f avg=%9.3f nsec\n", label,
	  1e9*first_step, 1e9*min_step, 1e9*sum_step/iterations);
}

static long
sum (char *buf, size_t size)
{
  long s = 0;
  size_t i;

  for (i = 0; i < size; ++i)
    s += *buf++;
  return s;
}

static void
measure_init (void)
{
# define N	1000
  double stop, start, getcontext_cold, getcontext_warm, init_cold, init_warm;
  unw_cursor_t cursor[N];
  unw_context_t uc[N];
  int i;

  /* Ensure memory is paged in but not in the cache: */
  memset (cursor, 0, sizeof (cursor));
  memset (uc, 0, sizeof (uc));
  dummy = sum (big, sizeof (big));

  start = gettime ();
  for (i = 0; i < N; ++i)
    unw_getcontext (&uc[i]);
  stop = gettime ();
  getcontext_cold = (stop - start) / N;

  start = gettime ();
  for (i = 0; i < N; ++i)
    unw_init_local (&cursor[i], &uc[i]);
  stop = gettime ();
  init_cold = (stop - start) / N;

  start = gettime ();
  for (i = 0; i < N; ++i)
    unw_getcontext (&uc[0]);
  stop = gettime ();
  getcontext_warm = (stop - start) / N;

  start = gettime ();
  for (i = 0; i < N; ++i)
    unw_init_local (&cursor[0], &uc[0]);
  stop = gettime ();
  init_warm = (stop - start) / N;

  printf ("unw_getcontext : cold avg=%9.3f nsec, warm avg=%9.3f nsec\n",
	  1e9 * getcontext_cold, 1e9 * getcontext_warm);
  printf ("unw_init_local : cold avg=%9.3f nsec, warm avg=%9.3f nsec\n",
	  1e9 * init_cold, 1e9 * init_warm);
}

int
main (int argc, char **argv)
{
  if (argc > 1)
    {
      maxlevel = atol (argv[1]);
      if (argc > 2)
	iterations = atol (argv[2]);
    }

  measure_init ();

  unw_set_caching_policy (unw_local_addr_space, UNW_CACHE_NONE);
  doit ("no cache        ");

  unw_set_caching_policy (unw_local_addr_space, UNW_CACHE_GLOBAL);
  doit ("global cache    ");

  unw_set_caching_policy (unw_local_addr_space, UNW_CACHE_PER_THREAD);
  doit ("per-thread cache");

  return 0;
}
