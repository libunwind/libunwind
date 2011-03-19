/* libunwind - a platform-independent unwind library
   Copyright (C) 2010 by Lassi Tuura <lat@iki.fi>

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

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <errno.h>
#if HAVE_EXECINFO_H
# include <execinfo.h>
#else
  extern int backtrace (void **, int);
#endif
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <libunwind.h>

#define panic(args...)				\
	{ fprintf (stderr, args); exit (-1); }

#ifndef HAVE_SIGHANDLER_T
typedef RETSIGTYPE (*sighandler_t) (int);
#endif

int verbose;
int num_errors;

/* These variables are global because they
 * cause the signal stack to overflow */
char buf[512], name[256];
void *addresses[2][128];
unw_cursor_t cursor;
ucontext_t uc;
#if UNW_TARGET_X86_64
unw_tdep_frame_t *cache;
#endif

static void
do_backtrace (void)
{
  unw_word_t ip;
  int ret = -UNW_ENOINFO;
  int depth = 128;
  int i, n;

  if (verbose)
    printf ("\tfast backtrace:\n");

  unw_getcontext (&uc);
  if (unw_init_local (&cursor, &uc) < 0)
    panic ("unw_init_local failed!\n");

#if UNW_TARGET_X86_64
  if ((ret = unw_tdep_trace (&cursor, addresses[0], &depth, cache)) < 0)
    {
      unw_get_reg (&cursor, UNW_REG_IP, &ip);
      printf ("FAILURE: unw_tdep_trace() returned %d for ip=%lx\n", ret, (long) ip);
      ++num_errors;
    }
#endif

  if (ret < 0)
    {
      i = 0;
      do
        {
	  unw_get_reg (&cursor, UNW_REG_IP, &ip);
	  addresses[0][i] = (void *) ip;
        }
      while ((ret = unw_step (&cursor)) >= 0 && ++i < 128);

      if (ret < 0)
        {
	  unw_get_reg (&cursor, UNW_REG_IP, &ip);
          printf ("FAILURE: unw_step() returned %d for ip=%lx\n", ret, (long) ip);
          ++num_errors;
	}
    }

  if (verbose)
    for (i = 0; i < depth; ++i)
      printf ("\t #%-3d ip=%p\n", i, addresses[0][i]);

  if (verbose)
    printf ("\n\tvia backtrace():\n");

  n = backtrace (addresses[1], 128);

  if (verbose)
    for (i = 0; i < n; ++i)
	printf ("\t #%-3d ip=%p\n", i, addresses[1][i]);

  if (n != depth)
    {
      printf ("FAILURE: unw_tdep_trace() and backtrace() depths differ: %d vs. %d\n", depth, n);
      ++num_errors;
    }
  else
    for (i = 1; i < depth; ++i)
      /* Allow one in difference in comparison, trace returns adjusted addresses. */
      if (labs((unw_word_t) addresses[0][i] - (unw_word_t) addresses[1][i]) > 1)
	{
          printf ("FAILURE: unw_tdep_trace() and backtrace() addresses differ at %d: %p vs. %p\n",
		  i, addresses[0][n], addresses[1][n]);
          ++num_errors;
	}
}

void
foo (long val)
{
  do_backtrace ();
}

void
bar (long v)
{
  extern long f (long);
  int arr[v];

  /* This is a vain attempt to use up lots of registers to force
     the frame-chain info to be saved on the memory stack on ia64.
     It happens to work with gcc v3.3.4 and gcc v3.4.1 but perhaps
     not with any other compiler.  */
  foo (f (arr[0]) + (f (v) + (f (v) + (f (v) + (f (v) + (f (v) + (f (v)
       + (f (v) + (f (v) + (f (v) + (f (v) + (f (v) + (f (v) + (f (v)
       + (f (v) + (f (v) + (f (v) + (f (v) + (f (v) + (f (v) + (f (v)
       + (f (v) + (f (v) + (f (v) + (f (v) + (f (v) + (f (v) + (f (v)
       + (f (v) + (f (v) + (f (v) + (f (v) + (f (v) + (f (v) + (f (v)
       + (f (v) + (f (v) + (f (v) + (f (v) + (f (v) + (f (v) + (f (v)
       + (f (v) + (f (v) + (f (v) + (f (v) + (f (v) + (f (v) + (f (v)
       + (f (v) + (f (v) + (f (v) + (f (v) + (f (v) + (f (v) + (f (v)
       + (f (v) + (f (v) + (f (v) + (f (v) + (f (v) + (f (v) + (f (v)
       + (f (v) + (f (v) + (f (v) + (f (v) + (f (v) + (f (v) + (f (v)
       + (f (v) + (f (v) + (f (v) + (f (v) + (f (v) + (f (v) + (f (v)
       + (f (v) + (f (v) + (f (v) + (f (v) + (f (v) + (f (v) + (f (v)
       + (f (v) + (f (v) + (f (v) + (f (v) + (f (v) + (f (v) + (f (v)
       + (f (v) + (f (v) + (f (v) + (f (v) + (f (v) + (f (v) + (f (v)
       + (f (v) + (f (v) + (f (v) + (f (v) + (f (v) + (f (v) + (f (v)
       + (f (v) + (f (v) + (f (v) + (f (v) + (f (v) + (f (v) + (f (v)
       + (f (v) + (f (v) + (f (v) + (f (v) + (f (v) + (f (v) + f (v))
       ))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))
       )))))))))))))))))))))))))))))))))))))))))))))))))))))));
}

void
sighandler (int signal, void *siginfo, void *context)
{
  ucontext_t *uc = context;
  int sp;

  if (verbose)
    {
      printf ("sighandler: got signal %d, sp=%p", signal, &sp);
#if UNW_TARGET_IA64
# if defined(__linux__)
      printf (" @ %lx", uc->uc_mcontext.sc_ip);
# else
      {
	uint16_t reason;
	uint64_t ip;

	__uc_get_reason (uc, &reason);
	__uc_get_ip (uc, &ip);
	printf (" @ %lx (reason=%d)", ip, reason);
      }
# endif
#elif UNW_TARGET_X86
#if defined __linux__
      printf (" @ %lx", (unsigned long) uc->uc_mcontext.gregs[REG_EIP]);
#elif defined __FreeBSD__
      printf (" @ %lx", (unsigned long) uc->uc_mcontext.mc_eip);
#endif
#elif UNW_TARGET_X86_64
#if defined __linux__
      printf (" @ %lx", (unsigned long) uc->uc_mcontext.gregs[REG_RIP]);
#elif defined __FreeBSD__
      printf (" @ %lx", (unsigned long) uc->uc_mcontext.mc_rip);
#endif
#endif
      printf ("\n");
    }
  do_backtrace();
}

int
main (int argc, char **argv)
{
  struct sigaction act;
  stack_t stk;

#if UNW_TARGET_X86_64
  cache = unw_tdep_make_frame_cache (0);
#endif

  verbose = (argc > 1);

  if (verbose)
    printf ("Normal backtrace:\n");

  bar (1);

  memset (&act, 0, sizeof (act));
  act.sa_handler = (void (*)(int)) sighandler;
  act.sa_flags = SA_SIGINFO;
  if (sigaction (SIGTERM, &act, NULL) < 0)
    panic ("sigaction: %s\n", strerror (errno));

  if (verbose)
    printf ("\nBacktrace across signal handler:\n");
  kill (getpid (), SIGTERM);

  if (verbose)
    printf ("\nBacktrace across signal handler on alternate stack:\n");
  stk.ss_sp = malloc (SIGSTKSZ);
  if (!stk.ss_sp)
    panic ("failed to allocate SIGSTKSZ (%u) bytes\n", SIGSTKSZ);
  stk.ss_size = SIGSTKSZ;
  stk.ss_flags = 0;
  if (sigaltstack (&stk, NULL) < 0)
    panic ("sigaltstack: %s\n", strerror (errno));

  memset (&act, 0, sizeof (act));
  act.sa_handler = (void (*)(int)) sighandler;
  act.sa_flags = SA_ONSTACK | SA_SIGINFO;
  if (sigaction (SIGTERM, &act, NULL) < 0)
    panic ("sigaction: %s\n", strerror (errno));
  kill (getpid (), SIGTERM);

  if (num_errors > 0)
    {
      fprintf (stderr, "FAILURE: detected %d errors\n", num_errors);
      exit (-1);
    }

#if UNW_TARGET_X86_64
  unw_tdep_free_frame_cache (cache);
#endif

  if (verbose)
    printf ("SUCCESS.\n");
  return 0;
}
