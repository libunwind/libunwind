/* libunwind - a platform-independent unwind library
   Copyright (C) 2002 Hewlett-Packard Co
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

#include "unwind_i.h"

static inline
common_init (struct ia64_cursor *c)
{
  int i, ret;

  c->cfm_loc =		IA64_REG_LOC (c, UNW_IA64_CFM);
  c->top_rnat_loc =	IA64_REG_LOC (c, UNW_IA64_AR_RNAT);
  c->bsp_loc =		IA64_REG_LOC (c, UNW_IA64_AR_BSP);
  c->bspstore_loc =	IA64_REG_LOC (c, UNW_IA64_AR_BSPSTORE);
  c->pfs_loc =		IA64_REG_LOC (c, UNW_IA64_AR_PFS);
  c->rnat_loc =		IA64_REG_LOC (c, UNW_IA64_AR_RNAT);
  c->ip_loc =		IA64_REG_LOC (c, UNW_IA64_IP);
  c->pri_unat_loc =	0;	/* no primary UNaT location */
  c->unat_loc =		IA64_REG_LOC (c, UNW_IA64_AR_UNAT);
  c->pr_loc =		IA64_REG_LOC (c, UNW_IA64_PR);
  c->lc_loc =		IA64_REG_LOC (c, UNW_IA64_AR_LC);
  c->fpsr_loc =		IA64_REG_LOC (c, UNW_IA64_AR_FPSR);

  c->r4_loc = IA64_REG_LOC (c, UNW_IA64_GR + 4);
  c->r5_loc = IA64_REG_LOC (c, UNW_IA64_GR + 5);
  c->r6_loc = IA64_REG_LOC (c, UNW_IA64_GR + 6);
  c->r7_loc = IA64_REG_LOC (c, UNW_IA64_GR + 7);

  /* This says that each NaT bit is stored along with the
     corresponding preserved register: */
  c->nat4_loc = IA64_LOC (4, 0);
  c->nat5_loc = IA64_LOC (5, 0);
  c->nat6_loc = IA64_LOC (6, 0);
  c->nat7_loc = IA64_LOC (7, 0);

  c->b1_loc = IA64_REG_LOC (c, UNW_IA64_BR + 1);
  c->b2_loc = IA64_REG_LOC (c, UNW_IA64_BR + 2);
  c->b3_loc = IA64_REG_LOC (c, UNW_IA64_BR + 3);
  c->b4_loc = IA64_REG_LOC (c, UNW_IA64_BR + 4);
  c->b5_loc = IA64_REG_LOC (c, UNW_IA64_BR + 5);

  c->f2_loc = IA64_FPREG_LOC (c, UNW_IA64_FR + 2);
  c->f3_loc = IA64_FPREG_LOC (c, UNW_IA64_FR + 3);
  c->f4_loc = IA64_FPREG_LOC (c, UNW_IA64_FR + 4);
  c->f5_loc = IA64_FPREG_LOC (c, UNW_IA64_FR + 5);
  for (i = 16; i <= 31; ++i)
    c->fr_loc[i - 16] = IA64_FPREG_LOC (c, UNW_IA64_FR + i);

  ret = ia64_get (c, c->pr_loc, &c->pr);
  if (ret < 0)
    return ret;

  ret = ia64_get (c, c->ip_loc, &c->ip);
  if (ret < 0)
    return ret;

  ret = ia64_get (c, IA64_REG_LOC (c, UNW_IA64_SP), &c->sp);
  if (ret < 0)
    return ret;

  c->psp = c->sp;

  ret = ia64_get (c, c->bsp_loc, &c->bsp);
  if (ret < 0)
    return ret;

  c->rbs_top = c->bsp;
  c->pi.flags = 0;

#ifdef UNW_LOCAL_ONLY
  c->eh_args[0] = c->eh_args[1] = c->eh_args[2] = c->eh_args[3] = 0;
#else
  for (i = 0; i < 4; ++i)
    {
      ret = ia64_get (c, IA64_REG_LOC (c, UNW_IA64_GR + 15 + i),
		      &c->eh_args[i]);
      if (ret < 0)
	{
	  if (ret == -UNW_EBADREG)
	    c->eh_args[i] = 0;
	  else
	    return ret;
	}
    }
#endif

#ifdef IA64_UNW_SCRIPT_CACHE
  c->hint = 0;
  c->prev_script = 0;
#endif
  return ia64_get_proc_info (c);
}
