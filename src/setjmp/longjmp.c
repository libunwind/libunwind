/* libunwind - a platform-independent unwind library
   Copyright (C) 2003-2004 Hewlett-Packard Co
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

#include "jmpbuf.h"
#include "setjmp_i.h"

void
_longjmp (jmp_buf env, int val)
{
  extern int _UI_longjmp_cont;
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
      if (sp != wp[JB_SP])
	continue;

      if (!bsp_match (&c, wp))
	continue;

      /* found the right frame: */

      assert (UNW_NUM_EH_REGS >= 2);

      if (unw_set_reg (&c, UNW_REG_EH + 0, wp[JB_RP]) < 0
	  || unw_set_reg (&c, UNW_REG_EH + 1, val) < 0
	  || unw_set_reg (&c, UNW_REG_IP,
			  (unw_word_t) &_UI_longjmp_cont))
	abort ();

      unw_resume (&c);

      abort ();
    }
  while (unw_step (&c) >= 0);

  abort ();
}

#ifdef __GNUC__
void longjmp (jmp_buf env, int val) __attribute__ ((alias ("_longjmp")));
#else

void
longjmp (jmp_buf env, int val)
{
  _longjmp (env, val);
}

#endif
