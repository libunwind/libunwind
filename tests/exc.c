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

/* This illustrates the basics of using the unwind interface for
   exception handling.  */

#include <stdio.h>
#include <stdlib.h>
#include <libunwind.h>

#define panic(args...)				\
	{ fprintf (stderr, args); exit (-1); }

int true = 1;

static void
raise_exception (void *addr)
{
  unw_cursor_t cursor;
  unw_word_t ip;
  unw_context_t uc;

  unw_getcontext (&uc);
  if (unw_init_local (&cursor, &uc) < 0)
    panic ("unw_init_local() failed!\n");

  /* unwind to frame b(): */
  if (unw_step (&cursor) < 0)
    panic ("unw_step() failed!\n");

  /* unwind to frame a(): */
  if (unw_step (&cursor) < 0)
    panic ("unw_step() failed!\n");

  unw_get_reg (&cursor, UNW_REG_IP, &ip);
  printf ("ip = %lx\n", ip);

  if (unw_set_reg (&cursor, UNW_REG_IP, (unw_word_t) addr) < 0)
    panic ("unw_set_reg() failed!\n");

  unw_resume (&cursor);	/* transfer control to exception handler */
}

static void
b (void *addr)
{
  printf ("b() calling raise_exception()\n");
  raise_exception (addr);
}

static int
a (void)
{
  register long sp asm ("r12");
  printf("a: sp=%lx bsp=%p\n", sp, __builtin_ia64_bsp ());
  b (&&handler);
  printf ("unexpected return from func()!\n");

  if (true)
    return -1;

 handler:
  printf ("exception handler: here we go (sp=%lx, bsp=%p)...\n",
	  sp, __builtin_ia64_bsp ());
  return 0;
}

int
main (int argc, char **argv)
{
  if (a () == 0)
    printf ("test succeeded!\n");
  else
    printf ("bummer: test failed; try again?\n");
  return 0;
}
