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

#include <stdio.h>
#include <stdlib.h>

#include <sys/time.h>

#include <libunwind.h>

#define ITERATIONS	10000

#define panic(args...)							  \
	do { fprintf (stderr, args); exit (-1); } while (0)

static int maxlevel = 100;

static inline double
gettime (void)
{
  struct timeval tv;

  gettimeofday(&tv, NULL);
  return tv.tv_sec + 1e-6*tv.tv_usec;
}

static int
measure_unwind (int maxlevel, double *init, double *step)
{
  double stop, mid, start;
  unw_cursor_t cursor;
  unw_context_t uc;
  int ret, level = 0;

  start = gettime ();

  unw_getcontext (&uc);
  if (unw_init_local (&cursor, &uc) < 0)
    panic ("unw_init_local() failed\n");

  mid = gettime ();

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

  *init = mid - start;
  *step = (stop - mid) / (double) level;
  return 0;
}

static int
f1 (int level, int maxlevel, double *init, double *step)
{
  if (level == maxlevel)
    return measure_unwind (maxlevel, init, step);
  else
    /* defeat last-call/sibcall optimization */
    return f1 (level + 1, maxlevel, init, step) + level;
}

static void
doit (const char *label)
{
  double init, step, min_init, first_init, min_step, first_step, sum_init, sum_step;
  int i;

  sum_init = sum_step = first_init = first_step = 0.0;
  min_init = min_step = 1e99;
  for (i = 0; i < ITERATIONS; ++i)
    {
      f1 (0, maxlevel, &init, &step);

      sum_init += init;
      sum_step += step;

      if (init < min_init)
	min_init = init;
      if (step < min_step)
	min_step = step;

      if (i == 0)
	{
	  first_init = init;
	  first_step = step;
	}
    }
  printf ("%s:\n"
	  "  unw_{getcontext+init_local}: first=%9.3f min=%9.3f avg=%9.3f nsec\n"
	  "  unw_step                   : first=%9.3f min=%9.3f avg=%9.3f nsec\n",
	  label,
	  1e9*first_init, 1e9*min_init, 1e9*sum_init/ITERATIONS,
	  1e9*first_step, 1e9*min_step, 1e9*sum_step/ITERATIONS);
}

int
main (int argc, char **argv)
{
  if (argc > 1)
    maxlevel = atol (argv[1]);

  unw_set_caching_policy (unw_local_addr_space, UNW_CACHE_NONE);
  doit ("Caching: none");

  unw_set_caching_policy (unw_local_addr_space, UNW_CACHE_GLOBAL);
  doit ("Caching: global");

  unw_set_caching_policy (unw_local_addr_space, UNW_CACHE_PER_THREAD);
  doit ("Caching: per-thread");

  return 0;
}
