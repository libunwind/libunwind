/* libunwind - a platform-independent unwind library
   Copyright (C) 2001-2002 Hewlett-Packard Co
	Contributed by David Mosberger-Tang <davidm@hpl.hp.com>

This file is part of libunwind.

libunwind is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2, or (at your option)
any later version.

libunwind is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

As a special exception, if you link this library with other files to
produce an executable, this library does not by itself cause the
resulting executable to be covered by the GNU General Public License.
This exception does not however invalidate any other reasons why the
executable file might be covered by the GNU General Public
License.  */

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

  /* update the IP cache: */
  ret = ia64_get (c, c->ip_loc, &ip);
  if (ret < 0)
    return ret;
  c->ip = ip;

  if ((ip & 0xf) != 0)
    {
      /* don't let obviously bad addresses pollute the cache */
      debug (1, "%s: rejecting bad ip=0x%lx\n",  __FUNCTION__, (long) c->ip);
      return -UNW_EINVALIDIP;
    }

  /* restore the cfm: */
  c->cfm_loc = c->pfs_loc;

  /* restore the bsp: */
  pr = c->pr;
  num_regs = 0;
  if ((c->pi.flags & IA64_FLAG_SIGTRAMP))
    {
      unw_word_t sigcontext_addr, sigcontext_flags;

      ret = ia64_get (c, c->sp + 0x10, &sigcontext_addr);
      if (ret < 0)
	return ret;

      ret = ia64_get (c, (sigcontext_addr + SIGCONTEXT_FLAGS_OFF),
		      &sigcontext_flags);
      if (ret < 0)
	return ret;

      if ((sigcontext_flags & IA64_SC_FLAG_IN_SYSCALL_BIT) == 0)
	{
	  unw_word_t cfm;

	  ret = ia64_get (c, c->cfm_loc, &cfm);
	  if (ret < 0)
	    return ret;

	  num_regs = cfm & 0x7f;	/* size of frame */
	}
      c->pfs_loc = (c->sp + 0x10 + SIGCONTEXT_AR_PFS_OFF);
    }
  else
    {
      ret = ia64_get (c, c->cfm_loc, &cfm);
      if (ret < 0)
	return ret;
      num_regs = (cfm >> 7) & 0x7f;	/* size of locals */
    }
  c->bsp = ia64_rse_skip_regs (c->bsp, -num_regs);

  /* restore the sp: */
  c->sp = c->psp;

  if (c->ip == prev_ip && c->sp == prev_sp && c->bsp == prev_bsp)
    {
      dprintf ("%s: ip, sp, and bsp unchanged; stopping here (ip=0x%lx)\n",
	       __FUNCTION__, (long) ip);
      STAT(unw.stat.api.unwind_time += ia64_get_itc () - start);
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
