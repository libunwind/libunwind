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

#include "offsets.h"
#include "rse.h"
#include "unwind_i.h"

static int
check_rbs_switch (struct cursor *c)
{
  unw_word_t saved_bsp, saved_bspstore, loadrs, ndirty;
  int ret = 0;

  saved_bsp = c->bsp;
  if (c->pi.flags & UNW_PI_FLAG_IA64_RBS_SWITCH)
    {
      /* Got ourselves a frame that has saved ar.bspstore, ar.bsp,
	 and ar.rnat, so we're all set for rbs-switching:  */
      if ((ret = ia64_get (c, c->bsp_loc, &saved_bsp)) < 0
	  || (ret = ia64_get (c, c->bspstore_loc, &saved_bspstore)))
	return ret;
    }
  else if (c->is_signal_frame
	   && c->bsp_loc == c->sigcontext_loc + SIGCONTEXT_AR_BSP_OFF)
    {
      /* When Linux delivers a signal on an alternate stack, it
	 does things a bit differently from what the unwind
	 conventions allow us to describe: instead of saving
	 ar.rnat, ar.bsp, and ar.bspstore, it saves the former two
	 plus the "loadrs" value.  Because of this, we need to
	 detect & record a potential rbs-area switch
	 manually... */

      /* If ar.bsp has been saved already AND the current bsp is
	 not equal to the saved value, then we know for sure that
	 we're past the point where the backing store has been
	 switched (and before the point where it's restored).  */
      if ((ret = ia64_get (c, c->sigcontext_loc + SIGCONTEXT_AR_BSP_OFF,
			   &saved_bsp) < 0)
	  || (ret = ia64_get (c, c->sigcontext_loc + SIGCONTEXT_LOADRS_OFF,
			      &loadrs) < 0))
	return ret;
      loadrs >>= 16;
      ndirty = ia64_rse_num_regs (c->bsp - loadrs, c->bsp);
      saved_bspstore = ia64_rse_skip_regs (saved_bsp, -ndirty);
    }

  if (saved_bsp != c->bsp)
    ret = rbs_record_switch (c, saved_bsp, saved_bspstore, c->rnat_loc);

  return ret;
}

static inline int
update_frame_state (struct cursor *c)
{
  unw_word_t prev_ip, prev_sp, prev_bsp, ip, pr, num_regs;
  int ret;

  prev_ip = c->ip;
  prev_sp = c->sp;
  prev_bsp = c->bsp;

  c->cfm_loc = c->pfs_loc;
  /* update the CFM cache: */
  ret = ia64_get (c, c->cfm_loc, &c->cfm);
  if (ret < 0)
    return ret;

  num_regs = 0;
  if (c->is_signal_frame)
    {
      ret = ia64_get (c, c->sp + 0x10 + SIGFRAME_ARG2_OFF, &c->sigcontext_loc);
      debug (100, "%s: sigcontext_loc=%lx (ret=%d)\n",
	     __FUNCTION__, c->sigcontext_loc, ret);
      if (ret < 0)
	return ret;

      c->sigcontext_off = c->sigcontext_loc - c->sp;

      if (c->ip_loc == c->sigcontext_loc + SIGCONTEXT_BR_OFF + 0*8)
	{
	  /* Earlier kernels (before 2.4.19 and 2.5.10) had buggy
	     unwind info for sigtramp.  Fix it up here.  */
	  c->ip_loc  = (c->sigcontext_loc + SIGCONTEXT_IP_OFF);
	  c->cfm_loc = (c->sigcontext_loc + SIGCONTEXT_CFM_OFF);
	  /* update the CFM cache: */
	  ret = ia64_get (c, c->cfm_loc, &c->cfm);
	  if (ret < 0)
	    return ret;
	}

      /* do what can't be described by unwind directives: */
      c->pfs_loc = (c->sigcontext_loc + SIGCONTEXT_AR_PFS_OFF);

      num_regs = c->cfm & 0x7f;		/* size of frame */
    }
  else
    num_regs = (c->cfm >> 7) & 0x7f;	/* size of locals */

  if (c->bsp_loc)
    {
      ret = check_rbs_switch (c);
      if (ret < 0)
	return ret;
    }

  c->bsp = ia64_rse_skip_regs (c->bsp, -num_regs);
  if (c->rbs_area[c->rbs_curr].end - c->bsp > c->rbs_area[c->rbs_curr].size)
    rbs_underflow (c);

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
  if (ip == 0)
    /* end of frame-chain reached */
    return 0;

  pr = c->pr;
  c->sp = c->psp;
  c->is_signal_frame = 0;

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

  c->pi_valid = 0;
  return 0;
}


int
unw_step (unw_cursor_t *cursor)
{
  struct cursor *c = (struct cursor *) cursor;
  int ret;

  ret = ia64_find_save_locs (c);
  if (ret < 0)
    return ret;

  ret = update_frame_state (c);
  if (ret < 0)
    return ret;

  return (c->ip == 0) ? 0 : 1;
}
