/* libunwind - a platform-independent unwind library
   Copyright (C) 2002-2003 Hewlett-Packard Co
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

#include "unwind_i.h"
#include "offsets.h"

PROTECTED int
unw_step (unw_cursor_t *cursor)
{
  struct cursor *c = (struct cursor *) cursor;
  int ret, i;

  /* Try DWARF-based unwinding... */
  ret = dwarf_step (&c->dwarf);

  if (unlikely (ret < 0))
    {
      /* DWARF failed, let's see if we can follow the frame-chain
	 or skip over the signal trampoline.  */
      struct dwarf_loc ebp_loc, eip_loc;

      Debug (14, "dwarf_step() failed (ret=%d), trying frame-chain\n",
	     ret);

      if (unw_is_signal_frame(cursor))
	{
	  /* XXX This code is Linux-specific! */

	  /* c->esp points at the arguments to the handler.  Without
	     SA_SIGINFO, the arguments consist of a signal number
	     followed by a struct sigcontext.  With SA_SIGINFO, the
	     arguments consist a signal number, a siginfo *, and a
	     ucontext *. */
	  unw_word_t siginfo_ptr_addr = c->dwarf.cfa + 4;
	  unw_word_t sigcontext_ptr_addr = c->dwarf.cfa + 8;
	  unw_word_t siginfo_ptr, sigcontext_ptr;
	  struct dwarf_loc esp_loc, siginfo_ptr_loc, sigcontext_ptr_loc;

	  siginfo_ptr_loc = DWARF_LOC (siginfo_ptr_addr, 0);
	  sigcontext_ptr_loc = DWARF_LOC (sigcontext_ptr_addr, 0);
	  ret = (dwarf_get (&c->dwarf, siginfo_ptr_loc, &siginfo_ptr)
		 | dwarf_get (&c->dwarf, sigcontext_ptr_loc, &sigcontext_ptr));
	  if (ret < 0)
	    return 0;
	  if (siginfo_ptr < c->dwarf.cfa
	      || siginfo_ptr > c->dwarf.cfa + 256
	      || sigcontext_ptr < c->dwarf.cfa
	      || sigcontext_ptr > c->dwarf.cfa + 256)
	    {
	      /* Not plausible for SA_SIGINFO signal */
	      unw_word_t sigcontext_addr = c->dwarf.cfa + 4;
	      esp_loc = DWARF_LOC (sigcontext_addr + LINUX_SC_ESP_OFF, 0);
	      ebp_loc = DWARF_LOC (sigcontext_addr + LINUX_SC_EBP_OFF, 0);
	      eip_loc = DWARF_LOC (sigcontext_addr + LINUX_SC_EIP_OFF, 0);
	    }
	  else
	    {
	      /* If SA_SIGINFO were not specified, we actually read
		 various segment pointers instead.  We believe that at
		 least fs and _fsh are always zero for linux, so it is
		 not just unlikely, but impossible that we would end
		 up here. */
	      esp_loc = DWARF_LOC (sigcontext_ptr + LINUX_UC_ESP_OFF, 0);
	      ebp_loc = DWARF_LOC (sigcontext_ptr + LINUX_UC_EBP_OFF, 0);
	      eip_loc = DWARF_LOC (sigcontext_ptr + LINUX_UC_EIP_OFF, 0);
	    }
	  ret = dwarf_get (&c->dwarf, esp_loc, &c->dwarf.cfa);
	  if (ret < 0)
	    return 0;
	}
      else
	{
	  ret = dwarf_get (&c->dwarf, c->dwarf.loc[EBP], &c->dwarf.cfa);
	  if (ret < 0)
	    return ret;

	  Debug (14, "[EBP=0x%lx] = 0x%lx\n",
		 (long) DWARF_GET_LOC (c->dwarf.loc[EBP]),
		 (long) c->dwarf.cfa);

	  ebp_loc = DWARF_LOC (c->dwarf.cfa, 0);
	  eip_loc = DWARF_LOC (c->dwarf.cfa + 4, 0);
	  c->dwarf.cfa += 8;
	}
      /* Mark all registers unsaved, since we don't know where they
	 are saved (if at all), except for the EBP and EIP.  */
      for (i = 0; i < DWARF_NUM_PRESERVED_REGS; ++i)
	c->dwarf.loc[i] = DWARF_NULL_LOC;
      c->dwarf.loc[EBP] = ebp_loc;
      c->dwarf.loc[EIP] = eip_loc;
      c->dwarf.ret_addr_column = EIP;

      if (!DWARF_IS_NULL_LOC (c->dwarf.loc[EBP]))
	{
	  ret = dwarf_get (&c->dwarf, c->dwarf.loc[EIP], &c->dwarf.ip);
	  if (ret < 0)
	    return ret;
	}
      else
	c->dwarf.ip = 0;
    }
  return (c->dwarf.ip == 0) ? 0 : 1;
}
