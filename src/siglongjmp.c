/* libunwind - a platform-independent unwind library
   Copyright (C) 2003 Hewlett-Packard Co
	Contributed by David Mosberger-Tang <davidm@hpl.hp.com>

This file is part of libunwind.

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

#define UNW_LOCAL_ONLY

#include <assert.h>
#include <libunwind.h>
#include <setjmp.h>
#include <signal.h>
#include <stdlib.h>

#include "tdep.h"
#include "jmpbuf.h"

#if UNW_TARGET_IA64
# include "ia64/rse.h"
#endif

void
siglongjmp (sigjmp_buf env, int val)
{
  unw_word_t *wp = (unw_word_t *) env;
  extern int _UI_siglongjmp_cont;
  extern int _UI_longjmp_cont;
  unw_context_t uc;
  unw_cursor_t c;
  unw_word_t sp;
  int *cont;

  if (unw_getcontext (&uc) < 0 || unw_init_local (&c, &uc) < 0)
    abort ();

  do
    {
      if (unw_get_reg (&c, UNW_REG_SP, &sp) < 0)
	abort ();
      if (sp != wp[JB_SP])
	continue;

#if UNW_TARGET_IA64
      {
	unw_word_t bsp, pfs, sol;

	if (unw_get_reg (&c, UNW_IA64_BSP, &bsp) < 0
	    || unw_get_reg (&c, UNW_IA64_AR_PFS, &pfs) < 0)
	  abort ();

	/* simulate the effect of "br.call sigsetjmp" on ar.bsp: */
	sol = (pfs >> 7) & 0x7f;
	bsp = ia64_rse_skip_regs (bsp, sol);

	if (bsp != wp[JB_BSP])
	  continue;
      }
#endif

      /* found the right frame: */

      assert (UNW_NUM_EH_REGS >= 4 && _NSIG <= 16 * sizeof (unw_word_t));

      /* default to continuation without sigprocmask() */
      cont = &_UI_longjmp_cont;

#if UNW_TARGET_IA64
      /* On ia64 we cannot call sigprocmask() at _UI_siglongjmp_cont()
	 because the signal may have switched stacks and the old
	 stack's register-backing store may have overflown, leaving us
	 no space to allocate the stacked registers needed to call
	 sigprocmask().  Fortunately, we can just let unw_resume()
	 (via sigreturn) take care of restoring the signal-mask.
	 That's faster anyhow.

         XXX We probably should do the analogous on all architectures.  */
      if (((struct cursor *) &c)->sigcontext_addr)
	{
	  /* let unw_resume() install the desired signal mask */
	  struct cursor *cp = (struct cursor *) &c;
	  struct sigcontext *sc = (struct sigcontext *) cp->sigcontext_addr;
	  sigset_t current_mask;
	  void *mp;

	  if (wp[JB_MASK_SAVED])
	    mp = &wp[JB_MASK];
	  else
	    {
	      if (sigprocmask (SIG_BLOCK, NULL, &current_mask) < 0)
		abort ();
	      mp = &current_mask;
	    }
	  memcpy (&sc->sc_mask, mp, sizeof (sc->sc_mask));
	}
      else
#endif
	if (wp[JB_MASK_SAVED])
	  {
	    /* sigmask was saved */
	    if (unw_set_reg (&c, UNW_REG_EH + 2, wp[JB_MASK]) < 0
		|| (_NSIG > 8 * sizeof (unw_word_t)
		    && unw_set_reg (&c, UNW_REG_EH + 3, wp[JB_MASK + 1]) < 0))
	      abort ();
	    cont = &_UI_siglongjmp_cont;
	  }

      if (unw_set_reg (&c, UNW_REG_EH + 0, wp[JB_RP]) < 0
	  || unw_set_reg (&c, UNW_REG_EH + 1, val) < 0
	  || unw_set_reg (&c, UNW_REG_IP, (unw_word_t) cont))
	abort ();

      unw_resume (&c);

      abort ();
    }
  while (unw_step (&c) >= 0);

  abort ();
}
