#include <assert.h>

#include "offsets.h"
#include "regs.h"
#include "rse.h"

int
unw_get_save_loc (unw_cursor_t *cursor, int reg, unw_save_loc_t *sloc)
{
  struct ia64_cursor *c = (struct ia64_cursor *) cursor;
  unw_word_t loc, reg_loc, nat_loc;

  loc = 0;		/* default to "not saved" */

  switch (reg)
    {
      /* frame registers */
    case UNW_IA64_BSP:
    case UNW_REG_SP:
    case UNW_REG_PROC_START:
    case UNW_REG_HANDLER:
    case UNW_REG_LSDA:
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
	      loc = c->uc->uc_mcontext.sc_nat;
#else
	      loc = IA64_REG_LOC (c, UNW_IA64_NAT + reg);
#endif
	    }
	  else if (reg >= 32)
	    {
	      loc = ia64_rse_rnat_addr (reg_loc);	/* XXX looks wrong */
	      if (nat_loc >= c->rbs_top)
		nat_loc = c->top_rnat_loc;
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
      reg = rotate_gr (c, reg - UNW_IA64_GR) + UNW_IA64_GR;
      loc = ia64_rse_skip_regs (c->bsp, reg - (UNW_IA64_GR + 32));
      break;

    case UNW_IA64_NAT + 32 ... UNW_IA64_NAT + 127:	/* stacked reg */
      reg = rotate_gr (c, reg - UNW_IA64_NAT) + UNW_IA64_NAT;
      loc = ia64_rse_skip_regs (c->bsp, reg - (UNW_IA64_NAT + 32));
      loc = ia64_rse_rnat_addr (loc);
      if (loc > c->rbs_top)
	loc = c->top_rnat_loc;
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
    case UNW_IA64_AR_25:
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
