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

static inline int
common_init (struct cursor *c)
{
  unw_word_t bspstore;
  int i, ret;

  c->cfm_loc =		IA64_REG_LOC (c, UNW_IA64_CFM);
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

  ret = ia64_get (c, IA64_REG_LOC (c, UNW_IA64_GR + 12), &c->sp);
  if (ret < 0)
    return ret;

  c->psp = c->sp;

  ret = ia64_get (c, c->bsp_loc, &c->bsp);
  if (ret < 0)
    return ret;

  ret = ia64_get (c, c->bspstore_loc, &bspstore);
  if (ret < 0)
    return ret;

  c->rbs_curr = c->rbs_left_edge = c->rbs_right_edge = 0;
  c->rbs_area[0].end = bspstore;
  c->rbs_area[0].size = ~(unw_word_t) 0;	/* initial guess... */
  c->rbs_area[0].rnat_loc = IA64_REG_LOC (c, UNW_IA64_AR_RNAT);
  debug (10, "%s: initial rbs-area: [?-0x%lx), rnat@0x%lx\n", __FUNCTION__,
	 (long) c->rbs_area[0].end, (long) c->rbs_area[0].rnat_loc);

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
  c->sigcontext_loc = 0;
  c->is_signal_frame = 0;

  c->hint = 0;
  c->prev_script = 0;
  c->pi_valid = 0;
  return 0;
}
