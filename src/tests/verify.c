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

#include <stdio.h>
#include <stdlib.h>
#include <unwind.h>

#define panic(args...)				\
	{ fprintf (stderr, args); exit (-1); }

static void
init_state (unw_context_t *ucp)
{
  int i;

  ucp->uc_mcontext.sc_flags = 0;

  ucp->uc_mcontext.sc_ar_ccv = random ();
  ucp->uc_mcontext.sc_ar_lc = random ();
  ucp->uc_mcontext.sc_pr = random ();

#if 0
  ucp->uc_mcontext.sc_ip = xxx;
  ucp->uc_mcontext.sc_cfm = xxx;
  ucp->uc_mcontext.sc_um = xxx;
  ucp->uc_mcontext.sc_ar_rsc = xxx;
  ucp->uc_mcontext.sc_ar_bsp = xxx;
  ucp->uc_mcontext.sc_ar_rnat = xxx;
  ucp->uc_mcontext.sc_ar_unat = xxx;
  ucp->uc_mcontext.sc_ar_fpsr = xxx;
  ucp->uc_mcontext.sc_ar_pfs = xxx;
#endif

  /* initialize static registers without trashing gp (r1), sp (r12),
     or tp (r13). */
  for (i = 2; i < 32; ++i)
    {
      if (i != 12 && i != 13)
	{
	  ucp->uc_mcontext.sc_gr[i] = random ();
	  ucp->uc_mcontext.sc_nat |= (random () & 1) << i;
	}
    }

#if 0
  /* initialize stacked registers: */
  for (i = 32; i < 128; ++i)
    {
      xxx;
    }
#endif

  for (i = 0; i < 8; ++i)
    ucp->uc_mcontext.sc_br[i] = random ();

  for (i = 0; i < 128; ++i)
    {
      ucp->uc_mcontext.sc_fr[i].u.bits[0] = random ();
      ucp->uc_mcontext.sc_fr[i].u.bits[0] = random ();
    }
#if 0
  ucp->uc_mcontext.sc_rbs_base = xxx;
  ucp->uc_mcontext.sc_loadrs = xxx;
  ucp->uc_mcontext.sc_ar25 = xxx;
  ucp->uc_mcontext.sc_ar26 = xxx;
#endif
}

static void
check_state (ucontext_t *orig_state, unw_cursor_t *c)
{
  unw_word_t val;

  unw_get_reg (c, UNW_REG_IP, &val);
  printf ("IP: orig=%016lx now=%016lx\n", orig_state->uc_mcontext.sc_ip, val);
}

static void
setup_context (ucontext_t *unwind_ucp)
{
  asm volatile ("mov ar.fpsr = %0" :: "r"(0x9804c8a70033f));

  init_state (unwind_ucp);
  setcontext (unwind_ucp);
}

static void
check (ucontext_t *unwind_ucp, ucontext_t *setup_ucp,
       void (*doit) (ucontext_t *))
{
  swapcontext (unwind_ucp, setup_ucp);
  (*doit) (unwind_ucp);
}

static void
test1 (ucontext_t *orig_state)
{
  unw_cursor_t cursor;
  ucontext_t uc;

  getcontext (&uc);
  if (unw_init_local (&cursor, &uc) < 0)
    panic ("unw_init_local failed\n");

  if (unw_step (&cursor) < 0)
    panic ("unw_step failed\n");

  check_state (orig_state, &cursor);
}

int
main (int argc, char **argv)
{
  ucontext_t unwind_uc, setup_uc;
  unsigned char stack_mem[256*1024];

  setup_uc.uc_stack.ss_sp = stack_mem;
  setup_uc.uc_stack.ss_flags = 0;
  setup_uc.uc_stack.ss_size = sizeof (stack_mem);
  makecontext (&setup_uc, (void (*) (void)) setup_context,
	       2, &setup_uc, &unwind_uc);
  check (&unwind_uc, &setup_uc, test1);
  return 0;
}
