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

  c->cfm_loc =			IA64_REG_LOC (c, UNW_IA64_CFM);
  c->loc[IA64_REG_BSP] =	IA64_REG_LOC (c, UNW_IA64_AR_BSP);
  c->loc[IA64_REG_BSPSTORE] =	IA64_REG_LOC (c, UNW_IA64_AR_BSPSTORE);
  c->loc[IA64_REG_PFS] =	IA64_REG_LOC (c, UNW_IA64_AR_PFS);
  c->loc[IA64_REG_RNAT] =	IA64_REG_LOC (c, UNW_IA64_AR_RNAT);
  c->loc[IA64_REG_IP] =		IA64_REG_LOC (c, UNW_IA64_IP);
  c->loc[IA64_REG_PRI_UNAT_MEM] = IA64_NULL_LOC; /* no primary UNaT location */
  c->loc[IA64_REG_UNAT] =	IA64_REG_LOC (c, UNW_IA64_AR_UNAT);
  c->loc[IA64_REG_PR] =		IA64_REG_LOC (c, UNW_IA64_PR);
  c->loc[IA64_REG_LC] =		IA64_REG_LOC (c, UNW_IA64_AR_LC);
  c->loc[IA64_REG_FPSR] =	IA64_REG_LOC (c, UNW_IA64_AR_FPSR);

  c->loc[IA64_REG_R4] = IA64_REG_LOC (c, UNW_IA64_GR + 4);
  c->loc[IA64_REG_R5] = IA64_REG_LOC (c, UNW_IA64_GR + 5);
  c->loc[IA64_REG_R6] = IA64_REG_LOC (c, UNW_IA64_GR + 6);
  c->loc[IA64_REG_R7] = IA64_REG_LOC (c, UNW_IA64_GR + 7);

  /* This says that each NaT bit is stored along with the
     corresponding preserved register: */
  c->loc[IA64_REG_NAT4] = IA64_LOC_REG (4, 0);
  c->loc[IA64_REG_NAT5] = IA64_LOC_REG (5, 0);
  c->loc[IA64_REG_NAT6] = IA64_LOC_REG (6, 0);
  c->loc[IA64_REG_NAT7] = IA64_LOC_REG (7, 0);

  c->loc[IA64_REG_B1] = IA64_REG_LOC (c, UNW_IA64_BR + 1);
  c->loc[IA64_REG_B2] = IA64_REG_LOC (c, UNW_IA64_BR + 2);
  c->loc[IA64_REG_B3] = IA64_REG_LOC (c, UNW_IA64_BR + 3);
  c->loc[IA64_REG_B4] = IA64_REG_LOC (c, UNW_IA64_BR + 4);
  c->loc[IA64_REG_B5] = IA64_REG_LOC (c, UNW_IA64_BR + 5);

  c->loc[IA64_REG_F2] = IA64_FPREG_LOC (c, UNW_IA64_FR + 2);
  c->loc[IA64_REG_F3] = IA64_FPREG_LOC (c, UNW_IA64_FR + 3);
  c->loc[IA64_REG_F4] = IA64_FPREG_LOC (c, UNW_IA64_FR + 4);
  c->loc[IA64_REG_F5] = IA64_FPREG_LOC (c, UNW_IA64_FR + 5);
  for (i = IA64_REG_F16; i <= IA64_REG_F31; ++i)
    c->loc[i] = IA64_FPREG_LOC (c, UNW_IA64_FR + 16 + (i - IA64_REG_F16));

  ret = ia64_get (c, c->loc[IA64_REG_IP], &c->ip);
  if (ret < 0)
    return ret;

  ret = ia64_get (c, c->cfm_loc, &c->cfm);
  if (ret < 0)
    return ret;

  ret = ia64_get (c, c->loc[IA64_REG_PR], &c->pr);
  if (ret < 0)
    return ret;

  ret = ia64_get (c, IA64_REG_LOC (c, UNW_IA64_GR + 12), &c->sp);
  if (ret < 0)
    return ret;

  c->psp = c->sp;

  ret = ia64_get (c, c->loc[IA64_REG_BSP], &c->bsp);
  if (ret < 0)
    return ret;

  ret = ia64_get (c, c->loc[IA64_REG_BSPSTORE], &bspstore);
  if (ret < 0)
    return ret;

  c->rbs_curr = c->rbs_left_edge = 0;

  /* There is no way to know the real size of the most recent
     (right-most) RBS so we'll just assume it to occupy a quarter of
     the address space (so we have a notion of "above" and "below" and
     one bit to indicate whether the backing store needs to be
     accessed via uc_access(3)).  */
  c->rbs_area[0].end = bspstore;
  c->rbs_area[0].size = ((unw_word_t) 1) << 63;	/* initial guess... */
  c->rbs_area[0].rnat_loc = IA64_REG_LOC (c, UNW_IA64_AR_RNAT);
  debug (10, "%s: initial rbs-area: [?-0x%llx), rnat@%s\n", __FUNCTION__,
	 (long long) c->rbs_area[0].end,
	 ia64_strloc (c->rbs_area[0].rnat_loc));

  c->pi.flags = 0;

  c->sigcontext_addr = 0;
  c->abi_marker = 0;

  c->hint = 0;
  c->prev_script = 0;
  c->eh_valid_mask = 0;
  c->pi_valid = 0;
  return 0;
}
