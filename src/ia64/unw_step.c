/* libunwind - a platform-independent unwind library
   Copyright (C) 2001-2002 Hewlett-Packard Co
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

#include "offsets.h"
#include "rse.h"
#include "unwind_i.h"

static inline int
update_frame_state (struct ia64_cursor *c)
{
  unw_word_t prev_ip, prev_sp, prev_bsp, ip, pr, num_regs, cfm;
  int ret;

  prev_ip = c->ip;
  prev_sp = c->sp;
  prev_bsp = c->bsp;

  c->cfm_loc = c->pfs_loc;

  num_regs = 0;
  if ((c->pi.flags & IA64_FLAG_SIGTRAMP) != 0)
    {
      ret = ia64_get (c, c->sp + 0x10 + SIGFRAME_ARG2_OFF, &c->sigcontext_loc);
      debug (100, "%s: sigcontext_loc=%lx (ret=%d)\n",
	     __FUNCTION__, c->sigcontext_loc, ret);
      if (ret < 0)
	return ret;

      if (c->ip_loc == c->sigcontext_loc + SIGCONTEXT_BR_OFF + 0*8)
	{
	  /* Earlier kernels (before 2.4.19 and 2.5.10) had buggy
	     unwind info for sigtramp.  Fix it up here.  */
	  c->ip_loc  = (c->sigcontext_loc + SIGCONTEXT_IP_OFF);
	  c->cfm_loc = (c->sigcontext_loc + SIGCONTEXT_CFM_OFF);
	}

      /* do what can't be described by unwind directives: */
      c->pfs_loc = (c->sigcontext_loc + SIGCONTEXT_AR_PFS_OFF);

      ret = ia64_get (c, c->cfm_loc, &cfm);
      if (ret < 0)
	return ret;

      num_regs = cfm & 0x7f;		/* size of frame */
    }
  else
    {
      ret = ia64_get (c, c->cfm_loc, &cfm);
      if (ret < 0)
	return ret;
      num_regs = (cfm >> 7) & 0x7f;	/* size of locals */
    }
  c->bsp = ia64_rse_skip_regs (c->bsp, -num_regs);

  /* update the IP cache: */
  ret = ia64_get (c, c->ip_loc, &ip);
  if (ret < 0)
    return ret;
  c->ip = ip;

  if ((ip & 0xc) != 0)
    {
      /* don't let obviously bad addresses pollute the cache */
      debug (1, "%s: rejecting bad ip=0x%lx\n",  __FUNCTION__, (long) c->ip);
      return -UNW_EINVALIDIP;
    }

  pr = c->pr;
  c->sp = c->psp;

  if (c->ip == prev_ip && c->sp == prev_sp && c->bsp == prev_bsp)
    {
      dprintf ("%s: ip, sp, and bsp unchanged; stopping here (ip=0x%lx)\n",
	       __FUNCTION__, (long) ip);
      return -UNW_EBADFRAME;
    }

  /* as we unwind, the saved ar.unat becomes the primary unat: */
  c->pri_unat_loc = c->unat_loc;

  /* restore the predicates: */
  ret = ia64_get (c, c->pr_loc, &c->pr);
  if (ret < 0)
    return ret;

  return ia64_get_proc_info (c);
}


int
unw_step (unw_cursor_t *cursor)
{
  struct ia64_cursor *c = (struct ia64_cursor *) cursor;
  int ret;

  ret = ia64_find_save_locs (c);
  if (ret < 0)
    return ret;

  ret = update_frame_state (c);
  if (ret < 0)
    return ret;

  return (c->ip == 0) ? 0 : 1;
}
