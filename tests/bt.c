/* libunwind - a platform-independent unwind library
   Copyright (C) 2001-2002 Hewlett-Packard Co
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

#include <execinfo.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <libunwind.h>

#define panic(args...)				\
	{ fprintf (stderr, args); exit (-1); }

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
      printf ("ip=%016lx sp=%016lx\n", ip, sp);

      {
	unw_proc_info_t pi;
	unw_word_t bsp;

	unw_get_proc_info (&cursor, &pi);
	unw_get_reg (&cursor, UNW_IA64_BSP, &bsp);
	printf ("\tproc_start=%016lx handler=%lx lsda=%lx bsp=%lx\n",
		pi.start_ip, pi.end_ip, pi.lsda, bsp);
      }
      ret = unw_step (&cursor);
      if (ret < 0)
	{
	  unw_get_reg (&cursor, UNW_REG_IP, &ip);
	  printf ("FAILURE: unw_step() returned %d for ip=%lx\n", ret, ip);
	}
    }
  while (ret > 0);
}

static void
foo (void)
{
  void *buffer[20];
  int i, n;

  do_backtrace ();

  n = backtrace (buffer, 20);
  for (i = 0; i < n; ++i)
    printf ("[%d] ip=%p\n", i, buffer[i]);
}

void
sighandler (int signal, void *siginfo, struct sigcontext *sc)
{
  printf ("sighandler: got signal %d @ %lx\n", signal, sc->sc_ip);

  do_backtrace();
}

int
main (int argc, char **argv)
{
  foo ();

  signal (SIGTERM, (sighandler_t) sighandler);
  kill (getpid (), SIGTERM);
  return 0;
}
