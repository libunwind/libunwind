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

#include <libunwind.h>
#include <setjmp.h>
#include <signal.h>
#include <stdlib.h>

#if UNW_TARGET_IA64
# include "ia64/rse.h"
#endif

void
_longjmp (jmp_buf env, int val)
{
  extern int _UI_siglongjmp_cont;
  sigset_t current_mask;
  unw_context_t uc;
  unw_cursor_t c;
  unw_word_t sp;
  unw_word_t *wp = (unw_word_t *) env;

  if (unw_getcontext (&uc) < 0 || unw_init_local (&c, &uc) < 0)
    abort ();

  do
    {
      if (unw_get_reg (&c, UNW_REG_SP, &sp) < 0)
	abort ();
      if (sp != wp[0])
	continue;

#if UNW_TARGET_IA64
      {
	unw_word_t bsp, pfs, sol;

	if (unw_get_reg (&c, UNW_IA64_BSP, &bsp) < 0
	    || unw_get_reg (&c, UNW_IA64_AR_PFS, &pfs) < 0)
	  abort ();

	/* simulate the effect of "br.call setjmp" on ar.bsp: */
	sol = (pfs >> 7) & 0x7f;
	bsp = ia64_rse_skip_regs (bsp, sol);

	if (bsp != wp[2])
	  continue;
      }
#endif

      /* found the right frame: */

      if (sigprocmask (SIG_BLOCK, NULL, &current_mask) < 0)
	abort ();

      if (unw_set_reg (&c, UNW_REG_EH_ARG0, wp[1]) < 0
	  || unw_set_reg (&c, UNW_REG_EH_ARG1, val) < 0
	  || unw_set_reg (&c, UNW_REG_EH_ARG2,
			  ((unw_word_t *) &current_mask)[0]) < 0
	  || unw_set_reg (&c, UNW_REG_IP,
			  (unw_word_t) &_UI_siglongjmp_cont))
	abort ();

      if (_NSIG > 8 * sizeof (unw_word_t))
	{
	  if (_NSIG > 16 * sizeof (unw_word_t))
	    abort ();
	  if (unw_set_reg (&c, UNW_REG_EH_ARG3,
			   ((unw_word_t *) &current_mask)[1]) < 0)
	    abort ();
	}

      unw_resume (&c);

      abort ();
    }
  while (unw_step (&c) >= 0);

  abort ();
}

void
longjmp (jmp_buf env, int val)
{
  _longjmp (env, val);
}
