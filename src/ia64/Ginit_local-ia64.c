/* libunwind - a platform-independent unwind library
   Copyright (C) 2001-2003 Hewlett-Packard Co
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

#include <string.h>
#include <stdlib.h>

#include "init.h"
#include "rse.h"
#include "unwind_i.h"

#ifdef UNW_REMOTE_ONLY

PROTECTED int
unw_init_local (unw_cursor_t *cursor, unw_context_t *uc)
{
  return -UNW_EINVAL;
}

#else /* !UNW_REMOTE_ONLY */

PROTECTED int
unw_init_local (unw_cursor_t *cursor, unw_context_t *uc)
{
  struct cursor *c = (struct cursor *) cursor;
  unw_word_t sol;

  if (tdep_needs_initialization)
    tdep_init ();

  Debug (2, "(cursor=%p)\n", c);

#ifdef __hpux
  {
    int ret;

    c->as = unw_local_addr_space;
    c->as_arg = uc;
    if ((ret = common_init (c)) < 0)
      return ret;

    /* On HP-UX, the context created by getcontext() points to the
       getcontext() system call stub.  Step over it: */
    return unw_step (cursor);
  }
#else
  /* The bsp value stored by getcontext() points to the *end* of the
     register frame of the initial function.  We correct for this by
     storing the adjusted value in sc_rbs_base, which isn't used by
     getcontext()/setcontext().  We can be certain that the entire
     frame is stored in a contiguous rbs-area because the frame didn't
     become part of the dirty partition until getcontext() was called
     and we know that getcontext() doesn't switch the register-backing
     store.  */
  sol = (uc->uc_mcontext.sc_ar_pfs >> 7) & 0x7f;
  uc->uc_mcontext.sc_rbs_base = ia64_rse_skip_regs (uc->uc_mcontext.sc_ar_bsp,
						    -sol);
#endif

  c->as = unw_local_addr_space;
  c->as_arg = uc;
  return common_init (c);
}

#endif /* !UNW_REMOTE_ONLY */
