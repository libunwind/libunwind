/* libunwind - a platform-independent unwind library
   Copyright (C) 2001-2002 Hewlett-Packard Co
	Contributed by David Mosberger-Tang <davidm@hpl.hp.com>

This file is part of libunwind.

libunwind is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2, or (at your option)
any later version.

libunwind is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.  */

#include <execinfo.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <libunwind.h>

#define panic(args...)				\
	{ fprintf (stderr, args); exit (-1); }

static void
do_backtrace (void)
{
  unw_cursor_t cursor;
  unw_word_t ip, sp;
  unw_context_t uc;

  unw_getcontext (&uc);
  if (unw_init_local (&cursor, &uc) < 0)
    panic ("unw_init_local failed!\n");

  do
    {
      unw_get_reg (&cursor, UNW_REG_IP, &ip);
      unw_get_reg (&cursor, UNW_REG_SP, &sp);
      printf ("ip=%016lx sp=%016lx\n", ip, sp);

      {
	unw_word_t proc_start, handler, lsda, bsp;

	unw_get_reg (&cursor, UNW_REG_PROC_START, &proc_start);
	unw_get_reg (&cursor, UNW_REG_HANDLER, &handler);
	unw_get_reg (&cursor, UNW_REG_LSDA, &lsda);
	unw_get_reg (&cursor, UNW_IA64_BSP, &bsp);
	printf ("\tproc_start=%016lx handler=%lx lsda=%lx bsp=%lx\n",
		proc_start, handler, lsda, bsp);
      }
    }
  while (unw_step (&cursor) > 0);
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

int
sighandler (int signal, void *siginfo, struct sigcontext *sc)
{
  void *buffer[20];
  int n;

  printf ("sighandler: got signal %d @ %lx\n", signal, sc->sc_ip);

  do_backtrace();
}

int
main (int argc, char **argv)
{
  foo ();

  signal (SIGTERM, sighandler);
  kill (getpid (), SIGTERM);
  return 0;
}
