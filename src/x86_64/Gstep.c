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
#include "ucontext_i.h"
#include <signal.h>

PROTECTED int
unw_step (unw_cursor_t *cursor)
{
  struct cursor *c = (struct cursor *) cursor;
  int ret, i;

  /* Try DWARF-based unwinding... */
  ret = dwarf_step (&c->dwarf);

  if (unlikely (ret == -UNW_ESTOPUNWIND))
    return ret;

  if (likely (ret >= 0))
    {
      /* x86_64 ABI specifies that end of call-chain is marked with a
	 NULL RBP.  */
      if (DWARF_IS_NULL_LOC (c->dwarf.loc[RBP]))
	c->dwarf.ip = 0;
    }
  else
    {
      /* DWARF failed, let's see if we can follow the frame-chain
	 or skip over the signal trampoline.  */
      struct dwarf_loc rbp_loc, rsp_loc, rip_loc;

      Debug (13, "dwarf_step() failed (ret=%d), trying frame-chain\n", ret);

      if (unw_is_signal_frame (cursor))
	{
	  unw_word_t ucontext = c->dwarf.cfa;

	  Debug(1, "signal frame, skip over trampoline\n");

	  c->sigcontext_format = X86_64_SCF_LINUX_RT_SIGFRAME;
	  c->sigcontext_addr = c->dwarf.cfa;

	  rsp_loc = DWARF_LOC (ucontext + UC_MCONTEXT_GREGS_RSP, 0);
	  rbp_loc = DWARF_LOC (ucontext + UC_MCONTEXT_GREGS_RBP, 0);
	  rip_loc = DWARF_LOC (ucontext + UC_MCONTEXT_GREGS_RIP, 0);

	  ret = dwarf_get (&c->dwarf, rsp_loc, &c->dwarf.cfa);
	  if (ret < 0)
	    return ret;
	}
      else
	{
	  ret = dwarf_get (&c->dwarf, c->dwarf.loc[RBP], &c->dwarf.cfa);
	  if (ret < 0)
	    return ret;

	  Debug (13, "[RBP=0x%Lx] = 0x%Lx\n",
		 (unsigned long long) DWARF_GET_LOC (c->dwarf.loc[RBP]),
		 (unsigned long long) c->dwarf.cfa);

	  rbp_loc = DWARF_LOC (c->dwarf.cfa, 0);
	  rsp_loc = DWARF_NULL_LOC;
	  rip_loc = DWARF_LOC (c->dwarf.cfa + 8, 0);
	  c->dwarf.cfa += 16;
	}

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
