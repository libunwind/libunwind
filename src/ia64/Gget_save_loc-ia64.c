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

#include <assert.h>

#include "offsets.h"
#include "regs.h"
#include "rse.h"

int
unw_get_save_loc (unw_cursor_t *cursor, int reg, unw_save_loc_t *sloc)
{
  struct cursor *c = (struct cursor *) cursor;
  unw_word_t loc, reg_loc;
  int ret;

  loc = 0;		/* default to "not saved" */

  switch (reg)
    {
      /* frame registers */
    case UNW_IA64_BSP:
    case UNW_REG_SP:
    default:
      break;

    case UNW_REG_IP:
      loc = c->ip_loc;
      break;

      /* preserved registers: */
    case UNW_IA64_GR + 4 ... UNW_IA64_GR + 7:
      loc = (&c->r4_loc)[reg - (UNW_IA64_GR + 4)];
      break;

    case UNW_IA64_NAT + 4 ... UNW_IA64_NAT + 7:
      loc = (&c->nat4_loc)[reg - (UNW_IA64_NAT + 4)];
      reg_loc = (&c->r4_loc)[reg - (UNW_IA64_NAT + 4)];
      if (IA64_IS_FP_LOC (reg_loc))
	/* NaT bit saved as a NaTVal.  */
	loc = reg_loc;
      else if (IA64_IS_MEMSTK_NAT (loc))
	loc = IA64_GET_LOC (loc) << 3;
      else
	{
	  reg = IA64_GET_LOC (loc);
	  assert (reg >= 0 && reg < 128);
	  if (!reg)
	    loc = 0;
	  else if (reg >= 4 && reg <= 7)
	    {
#ifdef UNW_LOCAL_ONLY
	      ucontext_t *uc = c->as_arg;
	      loc = uc->uc_mcontext.sc_nat;
#else
	      loc = IA64_REG_LOC (c, UNW_IA64_NAT + reg);
#endif
	    }
	  else if (reg >= 32)
	    {
	      reg = rotate_gr (c, reg);
	      ret = ia64_get_stacked (c, reg, NULL, &loc);
	      if (ret < 0)
		return ret;
	    }
	  else if (c->sigcontext_loc)
	    {
	      /* NaT bit is saved in a sigcontext.  */
	      loc = c->sigcontext_loc + SIGCONTEXT_NAT_OFF;
	    }
	}
      break;

    case UNW_IA64_AR_BSP:	loc = c->bsp_loc; break;
    case UNW_IA64_AR_BSPSTORE:	loc = c->bspstore_loc; break;
    case UNW_IA64_AR_PFS:	loc = c->pfs_loc; break;
    case UNW_IA64_AR_RNAT:	loc = c->rnat_loc; break;
    case UNW_IA64_AR_UNAT:	loc = c->unat_loc; break;
    case UNW_IA64_AR_LC:	loc = c->lc_loc; break;
    case UNW_IA64_AR_FPSR:	loc = c->fpsr_loc; break;
    case UNW_IA64_BR + 1:	loc = c->b1_loc; break;
    case UNW_IA64_BR + 2:	loc = c->b2_loc; break;
    case UNW_IA64_BR + 3:	loc = c->b3_loc; break;
    case UNW_IA64_BR + 4:	loc = c->b4_loc; break;
    case UNW_IA64_BR + 5:	loc = c->b5_loc; break;
    case UNW_IA64_CFM:		loc = c->cfm_loc; break;
    case UNW_IA64_PR:		loc = c->pr_loc; break;
      break;

    case UNW_IA64_GR + 32 ... UNW_IA64_GR + 127:	/* stacked reg */
      reg = rotate_gr (c, reg - UNW_IA64_GR);
      ret = ia64_get_stacked (c, reg, &loc, NULL);
      if (ret < 0)
	return ret;
      break;

    case UNW_IA64_NAT + 32 ... UNW_IA64_NAT + 127:	/* stacked reg */
      reg = rotate_gr (c, reg - UNW_IA64_NAT);
      ret = ia64_get_stacked (c, reg, NULL, &loc);
      break;

    case UNW_IA64_AR_EC:
      loc = c->cfm_loc;
      break;

      /* scratch & special registers: */

    case UNW_IA64_GR + 0:
    case UNW_IA64_GR + 1:				/* global pointer */
    case UNW_IA64_NAT + 0:
    case UNW_IA64_NAT + 1:				/* global pointer */
      break;

    case UNW_IA64_NAT + 2 ... UNW_IA64_NAT + 3:
    case UNW_IA64_NAT + 8 ... UNW_IA64_NAT + 31:
      loc = ia64_scratch_loc (c, reg);
      break;

    case UNW_IA64_GR + 2 ... UNW_IA64_GR + 3:
    case UNW_IA64_GR + 8 ... UNW_IA64_GR + 31:
    case UNW_IA64_BR + 0:
    case UNW_IA64_BR + 6:
    case UNW_IA64_BR + 7:
    case UNW_IA64_AR_RSC:
    case UNW_IA64_AR_CSD:
    case UNW_IA64_AR_26:
    case UNW_IA64_AR_CCV:
      loc = ia64_scratch_loc (c, reg);
      break;
    }

  memset (sloc, 0, sizeof (sloc));

  if (!loc)
    {
      sloc->type = UNW_SLT_NONE;
      return 0;
    }

#if !defined(UNW_LOCAL_ONLY)
  if (IA64_IS_REG_LOC (loc))
    {
      sloc->type = UNW_SLT_REG;
      sloc->u.regnum = IA64_GET_LOC (loc);
    }
  else
#endif
    {
      sloc->type = UNW_SLT_MEMORY;
      sloc->u.addr = loc;
    }
  return 0;
}
