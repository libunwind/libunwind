/* libunwind - a platform-independent unwind library
   Copyright (C) 2001-2003 Hewlett-Packard Co
	Contributed by David Mosberger-Tang <davidm@hpl.hp.com>

This file is part of libunwind.

Copyright (c) 2002 Hewlett-Packard Co.

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

#include <errno.h>
#include <execinfo.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <libunwind.h>

#define panic(args...)				\
	{ fprintf (stderr, args); exit (-1); }

static void
do_backtrace (void)
{
  unw_cursor_t cursor;
  unw_proc_info_t pi;
  unw_word_t ip, sp;
  unw_context_t uc;
  char buf[512];
  int ret;

  unw_getcontext (&uc);
  if (unw_init_local (&cursor, &uc) < 0)
    panic ("unw_init_local failed!\n");

  do
    {
      unw_get_reg (&cursor, UNW_REG_IP, &ip);
      unw_get_reg (&cursor, UNW_REG_SP, &sp);
      unw_get_proc_name (&cursor, buf, sizeof (buf));
      printf ("%016lx %-32s (sp=%016lx)\n", (long) ip, buf, (long) sp);

      unw_get_proc_info (&cursor, &pi);
      printf ("\tproc=%016lx-%016lx\n\thandler=%lx lsda=%lx",
	      (long) pi.start_ip, (long) pi.end_ip,
	      (long) pi.handler, (long) pi.lsda);

#if UNW_TARGET_IA64
      {
	unw_word_t bsp;

	unw_get_reg (&cursor, UNW_IA64_BSP, &bsp);
	printf (" bsp=%lx", bsp);
      }
#endif
      printf ("\n");

      ret = unw_step (&cursor);
      if (ret < 0)
	{
	  unw_get_reg (&cursor, UNW_REG_IP, &ip);
	  printf ("FAILURE: unw_step() returned %d for ip=%lx\n",
		  ret, (long) ip);
	}
    }
  while (ret > 0);
}

static void
foo (void)
{
  void *buffer[20];
  int i, n;

  printf ("\texplicit backtrace:\n");
  do_backtrace ();

  printf ("\tvia backtrace():\n");
  n = backtrace (buffer, 20);
  for (i = 0; i < n; ++i)
    printf ("[%d] ip=%p\n", i, buffer[i]);
}

void
#ifdef UNW_TARGET_X86
sighandler (int signal, struct sigcontext sc)
#else
sighandler (int signal, void *siginfo, struct sigcontext *sc)
#endif
{
  int sp;

  printf ("sighandler: got signal %d, sp=%p", signal, &sp);
#if UNW_TARGET_IA64
  printf (" @ %lx", sc->sc_ip);
#elif UNW_TARGET_X86
  printf (" @ %lx", sc.eip);
#endif
  printf ("\n");

  do_backtrace();
}

int
main (int argc, char **argv)
{
  struct sigaction act;
  stack_t stk;

  printf ("Normal backtrace:\n");
  foo ();

  printf ("\nBacktrace across signal handler:\n");
  signal (SIGTERM, (sighandler_t) sighandler);
  kill (getpid (), SIGTERM);

  printf ("Backtrace across signal handler on alternate stack:\n");
  stk.ss_sp = malloc (SIGSTKSZ);
  if (!stk.ss_sp)
    panic ("failed to allocate SIGSTKSZ (%u) bytes\n", SIGSTKSZ);
  stk.ss_size = SIGSTKSZ;
  stk.ss_flags = 0;
  if (sigaltstack (&stk, NULL) < 0)
    panic ("sigaltstack: %s\n", strerror (errno));

  memset (&act, 0, sizeof (act));
  act.sa_handler = (void (*)(int)) sighandler;
  act.sa_flags = SA_ONSTACK;
  if (sigaction (SIGTERM, &act, NULL) < 0)
    panic ("sigaction: %s\n", strerror (errno));
  kill (getpid (), SIGTERM);

  return 0;
}
