#include <libunwind.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <sys/resource.h>

#define panic(args...)				\
	{ fprintf (stderr, args); exit (-1); }

int verbose;

static void
do_backtrace (void)
{
  unw_cursor_t cursor;
  unw_word_t ip, sp;
  unw_context_t uc;
  int ret;

  unw_getcontext (&uc);
  if (unw_init_local (&cursor, &uc) < 0)
    panic ("unw_init_local failed!\n");

  do
    {
      unw_get_reg (&cursor, UNW_REG_IP, &ip);
      unw_get_reg (&cursor, UNW_REG_SP, &sp);

      if (verbose)
	printf ("%016lx (sp=%016lx)\n", (long) ip, (long) sp);

      ret = unw_step (&cursor);
      if (ret < 0)
	{
	  unw_get_reg (&cursor, UNW_REG_IP, &ip);
	  panic ("FAILURE: unw_step() returned %d for ip=%lx\n",
		 ret, (long) ip);
	}
    }
  while (ret > 0);
}

int
main (int argc, char **argv)
{
  struct rlimit rlim;

  verbose = argc > 1;

  rlim.rlim_cur = 0;
  rlim.rlim_max = RLIM_INFINITY;
  setrlimit (RLIMIT_DATA, &rlim);
  setrlimit (RLIMIT_AS, &rlim);

  do_backtrace ();
  return 0;
}
