/* libunwind - a platform-independent unwind library
   Copyright (c) 2003-2004 Hewlett-Packard Development Company, L.P.
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

#include "dwarf.h"
#include "tdep.h"

static int
update_frame_state (struct dwarf_cursor *c, unw_word_t prev_cfa)
{
  unw_word_t prev_ip, ip;
  int ret;

  prev_ip = c->ip;

  /* Update the IP cache (do this first: if we reach the end of the
     frame-chain, the rest of the info may not be valid/useful
     anymore. */
  ret = dwarf_get (c, c->loc[c->ret_addr_column], &ip);
  if (ret < 0)
    return ret;
  c->ip = ip;

#if 0
  /* ??? fix me---perhaps move to where we have convenient access to
     code_align? */
  if ((ip & (c->code_align - 1)) != 0)
    {
      /* don't let obviously bad addresses pollute the cache */
      Debug (1, "rejecting bad ip=0x%lx\n", (long) c->ip);
      return -UNW_EINVALIDIP;
    }
#endif
  if (ip == 0)
    /* end of frame-chain reached */
    return 0;

#if 0
  num_regs = 0;
  if (unlikely (c->abi_marker))
    {
      c->last_abi_marker = c->abi_marker;
      switch (c->abi_marker)
	{
	case ABI_MARKER_LINUX_SIGTRAMP:
	case ABI_MARKER_OLD_LINUX_SIGTRAMP:
	  c->as->abi = ABI_LINUX;
	  if ((ret = linux_sigtramp (c, &num_regs)) < 0)
	    return ret;
	  break;

	case ABI_MARKER_OLD_LINUX_INTERRUPT:
	case ABI_MARKER_LINUX_INTERRUPT:
	  c->as->abi = ABI_LINUX;
	  if ((ret = linux_interrupt (c, &num_regs, c->abi_marker)) < 0)
	    return ret;
	  break;

	case ABI_MARKER_HP_UX_SIGTRAMP:
	  c->as->abi = ABI_HPUX;
	  if ((ret = hpux_sigtramp (c, &num_regs)) < 0)
	    return ret;
	  break;

	default:
	  Debug (1, "unknown ABI marker: ABI=%u, context=%u\n",
		 c->abi_marker >> 8, c->abi_marker & 0xff);
	  return -UNW_EINVAL;
	}
      Debug (10, "sigcontext_addr=%lx (ret=%d)\n",
	     (unsigned long) c->sigcontext_addr, ret);

      c->sigcontext_off = c->sigcontext_addr - c->cfa;

      /* update the IP cache: */
      if ((ret = ia64_get (c, c->loc[IA64_REG_IP], &ip)) < 0)
 	return ret;
      c->ip = ip;
      if (ip == 0)
	/* end of frame-chain reached */
	return 0;
    }
  else
    num_regs = (c->cfm >> 7) & 0x7f;	/* size of locals */

  c->abi_marker = 0;
#endif

  if (c->ip == prev_ip && c->cfa == prev_cfa)
    {
      dprintf ("%s: ip and cfa unchanged; stopping (ip=0x%lx cfa=0x%lx)\n",
	       __FUNCTION__, (long) prev_ip, (long) prev_cfa);
      return -UNW_EBADFRAME;
    }

  c->pi_valid = 0;
  return 0;
}

HIDDEN int
dwarf_step (struct dwarf_cursor *c)
{
  unw_word_t prev_cfa = c->cfa;
  int ret;

  if ((ret = dwarf_find_save_locs (c)) >= 0
      && (ret = update_frame_state (c, prev_cfa)) >= 0)
    ret = (c->ip == 0) ? 0 : 1;

  Debug (15, "returning %d\n", ret);
  return ret;
}
