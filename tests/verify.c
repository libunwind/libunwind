/* libunwind - a platform-independent unwind library
   Copyright (C) 2001-2002 Hewlett-Packard Co
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
