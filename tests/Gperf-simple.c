#include <stdio.h>
#include <stdlib.h>

#include <sys/time.h>

#include <libunwind.h>

#define panic(args...)							  \
	do { fprintf (stderr, args); exit (-1); } while (0)

static inline double
gettime (void)
{
  struct timeval tv;

  gettimeofday(&tv, NULL);
  return tv.tv_sec + 1e-6*tv.tv_usec;
}

static int
measure_unwind (int maxlevel)
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

  printf (" unw_{getcontext+init_local}: %9.3f nsec, unw_step: %9.3f nsec\n",
	  1e9*(mid - start), 1e9*(stop - mid)/level);
  return 0;
}

static int
f1 (int level, int maxlevel)
{
  if (level == maxlevel)
    return measure_unwind (maxlevel);
  else
    /* defeat last-call/sibcall optimization */
    return f1 (level + 1, maxlevel) + level;
}

int
main (int argc, char **argv)
{
  int i, maxlevel = 100;

  if (argc > 1)
    maxlevel = atol (argv[1]);

  printf ("Caching: none\n");
  unw_set_caching_policy (unw_local_addr_space, UNW_CACHE_NONE);
  for (i = 0; i < 20; ++i)
    f1 (0, maxlevel);

  printf ("Caching: global\n");
  unw_set_caching_policy (unw_local_addr_space, UNW_CACHE_GLOBAL);
  for (i = 0; i < 20; ++i)
    f1 (0, maxlevel);

  printf ("Caching: per-thread\n");
  unw_set_caching_policy (unw_local_addr_space, UNW_CACHE_PER_THREAD);
  for (i = 0; i < 20; ++i)
    f1 (0, maxlevel);
  return 0;
}
