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

/* This shows how to use the unwind interface to modify any ancestor
   frame while still returning to the parent frame.  */

#include <signal.h>
#include <stdio.h>

#include <unwind.h>

#define panic(args...)				\
	{ fprintf (stderr, args); exit (-1); }

static void
sighandler (int signal)
{
  unw_cursor_t cursor, cursor2;
  unw_word_t rp;
  unw_context_t uc;

  printf ("caught signal %d\n", signal);

  unw_getcontext(&uc);

  if (unw_init (&cursor, &uc) < 0)
    panic ("unw_init() failed!\n");

  /* get cursor for caller of sighandler: */
  if (unw_step (&cursor) < 0)
    panic ("unw_step() failed!\n");

  cursor2 = cursor;
  while (!unw_is_signal_frame (&cursor2))
    if (unw_step (&cursor2) < 0)
      panic ("failed to find signal frame!\n");

  if (unw_get_reg (&cursor2, UNW_REG_RP, &rp) < 0)
    panic ("failed to get IP!\n");

  /* skip faulting instruction (doesn't handle MLX template) */
  ++rp;
  if (rp & 0x3 == 0x3)
    rp += 13;

  if (unw_set_reg (&cursor2, UNW_REG_RP, rp) < 0)
    panic ("failed to set IP!\n");

  unw_resume (&cursor);	/* update context & return to caller of sighandler() */

  panic ("unexpected return from unw_resume()!\n");
}

static void
doit (char *p)
{
  int ch;

  ch = *p;	/* trigger SIGSEGV */

  printf ("doit: finishing execution!\n");
}

int
main (int argc, char **argv)
{
  signal (SIGSEGV, sighandler);
  doit (0);
}
