/* libunwind - a platform-independent unwind library
   Copyright (C) 2002-2004 Hewlett-Packard Co
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

  Debug (1, "(cursor=%p, ip=0x%016llx)\n",
	 c, (unsigned long long) c->dwarf.ip);

  /* Try DWARF-based unwinding... */
  ret = dwarf_step (&c->dwarf);

  if (ret < 0 && ret != -UNW_ENOINFO)
    {
      Debug (2, "returning %d\n", ret);
      return ret;
    }

  if (likely (ret >= 0))
    {
      /* x86_64 ABI specifies that end of call-chain is marked with a
	 NULL RBP.  */
      if (DWARF_IS_NULL_LOC (c->dwarf.loc[RBP]))
	c->dwarf.ip = 0;
    }
  else
    {
      /* DWARF failed.  There isn't much of a usable frame-chain on x86-64,
	 but we do need to handle two special-cases:

	  (i) signal trampoline: Old kernels and older libcs don't
	      export the vDSO needed to get proper unwind info for the
	      trampoline.  Recognize that case by looking at the code
	      and filling in things by hand.

	  (ii) PLT (shared-library) call-stubs: PLT stubs are invoked
	      via CALLQ.  Try this for all non-signal trampoline
	      code.  */

      unw_word_t prev_ip = c->dwarf.ip, prev_cfa = c->dwarf.cfa;
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
	    {
	      Debug (2, "returning %d\n", ret);
	      return ret;
	    }

	  c->dwarf.loc[RAX] = DWARF_LOC (ucontext + UC_MCONTEXT_GREGS_RAX, 0);
	  c->dwarf.loc[RDX] = DWARF_LOC (ucontext + UC_MCONTEXT_GREGS_RDX, 0);
	  c->dwarf.loc[RCX] = DWARF_LOC (ucontext + UC_MCONTEXT_GREGS_RCX, 0);
	  c->dwarf.loc[RBX] = DWARF_LOC (ucontext + UC_MCONTEXT_GREGS_RBX, 0);
	  c->dwarf.loc[RSI] = DWARF_LOC (ucontext + UC_MCONTEXT_GREGS_RSI, 0);
	  c->dwarf.loc[RDI] = DWARF_LOC (ucontext + UC_MCONTEXT_GREGS_RDI, 0);
	  c->dwarf.loc[RBP] = DWARF_LOC (ucontext + UC_MCONTEXT_GREGS_RBP, 0);
	  c->dwarf.loc[ R8] = DWARF_LOC (ucontext + UC_MCONTEXT_GREGS_R8, 0);
	  c->dwarf.loc[ R9] = DWARF_LOC (ucontext + UC_MCONTEXT_GREGS_R9, 0);
	  c->dwarf.loc[R10] = DWARF_LOC (ucontext + UC_MCONTEXT_GREGS_R10, 0);
	  c->dwarf.loc[R11] = DWARF_LOC (ucontext + UC_MCONTEXT_GREGS_R11, 0);
	  c->dwarf.loc[R12] = DWARF_LOC (ucontext + UC_MCONTEXT_GREGS_R12, 0);
	  c->dwarf.loc[R13] = DWARF_LOC (ucontext + UC_MCONTEXT_GREGS_R13, 0);
	  c->dwarf.loc[R14] = DWARF_LOC (ucontext + UC_MCONTEXT_GREGS_R14, 0);
	  c->dwarf.loc[R15] = DWARF_LOC (ucontext + UC_MCONTEXT_GREGS_R15, 0);
	  c->dwarf.loc[RIP] = DWARF_LOC (ucontext + UC_MCONTEXT_GREGS_RIP, 0);
	}
      else
	{
	  unw_word_t rbp;

	  ret = dwarf_get (&c->dwarf, c->dwarf.loc[RBP], &rbp);
	  if (ret < 0)
	    {
	      Debug (2, "returning %d\n", ret);
	      return ret;
	    }

	  if (!rbp)
	    {
	      /* Looks like we may have reached the end of the call-chain.  */
	      rbp_loc = DWARF_NULL_LOC;
	      rsp_loc = DWARF_NULL_LOC;
	      rip_loc = DWARF_NULL_LOC;
	    }
	  else
	    {
	      unw_word_t rbp1;
	      Debug (1, "[RBP=0x%Lx] = 0x%Lx (cfa = 0x%Lx)\n",
		     (unsigned long long) DWARF_GET_LOC (c->dwarf.loc[RBP]),
		     (unsigned long long) rbp,
		     (unsigned long long) c->dwarf.cfa);

	      rbp_loc = DWARF_LOC(rbp, 0);
	      rsp_loc = DWARF_NULL_LOC;
	      rip_loc = DWARF_LOC (rbp + 8, 0);
              /* Heuristic to recognize a bogus frame pointer */
	      ret = dwarf_get (&c->dwarf, rbp_loc, &rbp1);
              if (ret || ((rbp1 - rbp) > 0x4000))
                rbp_loc = DWARF_NULL_LOC;
	      c->dwarf.cfa += 16;
	    }

	  /* Mark all registers unsaved */
	  for (i = 0; i < DWARF_NUM_PRESERVED_REGS; ++i)
	    c->dwarf.loc[i] = DWARF_NULL_LOC;
	}

      c->dwarf.loc[RBP] = rbp_loc;
      c->dwarf.loc[RSP] = rsp_loc;
      c->dwarf.loc[RIP] = rip_loc;
      c->dwarf.ret_addr_column = RIP;

      if (!DWARF_IS_NULL_LOC (c->dwarf.loc[RBP]))
	{
	  ret = dwarf_get (&c->dwarf, c->dwarf.loc[RIP], &c->dwarf.ip);
	  Debug (1, "Frame Chain [RIP=0x%Lx] = 0x%Lx\n",
		     (unsigned long long) DWARF_GET_LOC (c->dwarf.loc[RIP]),
		     (unsigned long long) c->dwarf.ip);
	  if (ret < 0)
	    {
	      Debug (2, "returning %d\n", ret);
	      return ret;
	    }
	}
      else
	c->dwarf.ip = 0;

      if (c->dwarf.ip == prev_ip && c->dwarf.cfa == prev_cfa)
	return -UNW_EBADFRAME;
    }
  ret = (c->dwarf.ip == 0) ? 0 : 1;
  Debug (2, "returning %d\n", ret);
  return ret;
}
