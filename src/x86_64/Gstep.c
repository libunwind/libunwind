/* libunwind - a platform-independent unwind library
   Copyright (C) 2002-2003 Hewlett-Packard Co
	Contributed by David Mosberger-Tang <davidm@hpl.hp.com>

   Modified for x86_64 by Max Asbock <masbock@us.ibm.com>

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

#include "unwind_i.h"
#include <signal.h>

PROTECTED int
unw_step (unw_cursor_t *cursor)
{
  struct cursor *c = (struct cursor *) cursor;
  int ret, i;

  /* Only attempt DWARF based unwinding since DWARF frame info is
     always expected to be there. With the exception of signal frames. */

  if (!unw_is_signal_frame (cursor))
    {
      if ((ret = dwarf_step (&c->dwarf)) < 0)
        {
          Debug(1, "dwarf step failed\n");
          return ret;
        }
    }
  else
    {
      struct dwarf_loc rbp_loc, rsp_loc, rip_loc;
      struct ucontext *ucontext = (struct ucontext *)c->dwarf.cfa;
      unw_word_t *uc_gregs = &ucontext->uc_mcontext.gregs[0];

      Debug(1, "signal frame, skip over trampoline\n");

      c->sigcontext_format = X86_64_SCF_LINUX_RT_SIGFRAME;
      c->sigcontext_addr = c->dwarf.cfa;

#if 0
      /* XXX this needs to be fixed for cross-compilation --davidm */
      rsp_loc = DWARF_LOC ((unw_word_t)&uc_gregs[REG_RSP], 0);
      rbp_loc = DWARF_LOC ((unw_word_t)&uc_gregs[REG_RBP], 0);
      rip_loc = DWARF_LOC ((unw_word_t)&uc_gregs[REG_RIP], 0);
#endif

      ret = dwarf_get (&c->dwarf, rsp_loc, &c->dwarf.cfa);
      if (ret < 0)
        return ret;

      /* Mark all registers unsaved */
      for (i = 0; i < DWARF_NUM_PRESERVED_REGS; ++i)
	c->dwarf.loc[i] = DWARF_NULL_LOC;

      c->dwarf.loc[RBP] = rbp_loc;
      c->dwarf.loc[RSP] = rsp_loc;
      c->dwarf.loc[RIP] = rip_loc;
      c->dwarf.ret_addr_column = RIP;

      if (!DWARF_IS_NULL_LOC (c->dwarf.loc[RBP]))
	{
	  ret = dwarf_get (&c->dwarf, c->dwarf.loc[RIP], &c->dwarf.ip);
	  if (ret < 0)
	    return ret;
	}
      else
	c->dwarf.ip = 0;
    }
  return (c->dwarf.ip == 0) ? 0 : 1;
}
