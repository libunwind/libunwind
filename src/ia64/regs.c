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

#include <assert.h>

#include "offsets.h"
#include "regs.h"
#include "rse.h"
#include "unwind_i.h"

unw_word_t
ia64_scratch_loc (struct ia64_cursor *c, unw_regnum_t reg)
{
  unw_word_t loc = c->sigcontext_loc;

  if (loc)
    {
      switch (reg)
	{
	case UNW_IA64_NAT + 2 ... UNW_IA64_NAT + 3:
	case UNW_IA64_NAT + 8 ... UNW_IA64_NAT + 31:
	  loc += SIGCONTEXT_NAT_OFF;
	  break;

	case UNW_IA64_GR +  2 ... UNW_IA64_GR + 3:
	case UNW_IA64_GR +  8 ... UNW_IA64_GR + 31:
	  loc += SIGCONTEXT_GR_OFF + 8*reg;
	  break;

	case UNW_IA64_FR + 6 ... UNW_IA64_FR + 15:
	  loc += SIGCONTEXT_FR_OFF + 16*(reg - UNW_IA64_FR);
	  break;

	case UNW_IA64_FR + 32 ... UNW_IA64_FR + 127:
	  loc += SIGCONTEXT_FR_OFF + 16*(reg - UNW_IA64_FR);
	  break;

	case UNW_IA64_BR + 0: loc += SIGCONTEXT_BR_OFF + 0; break;
	case UNW_IA64_BR + 6: loc += SIGCONTEXT_BR_OFF + 6*8; break;
	case UNW_IA64_BR + 7: loc += SIGCONTEXT_BR_OFF + 7*8; break;
	case UNW_IA64_AR_RSC: loc += SIGCONTEXT_AR_RSC_OFF; break;
	case UNW_IA64_AR_25:  loc += SIGCONTEXT_AR_25_OFF; break;
	case UNW_IA64_AR_26:  loc += SIGCONTEXT_AR_26_OFF; break;
	case UNW_IA64_AR_CCV: loc += SIGCONTEXT_AR_CCV; break;
	}

      return loc;
    }
  else
    return IA64_REG_LOC (c, reg);
}

static inline int
update_nat (struct ia64_cursor *c, unw_word_t nat_loc, unw_word_t mask,
	    unw_word_t *valp, int write)
{
  unw_word_t nat_word;
  int ret;

  ret = ia64_get (c, nat_loc, &nat_word);
  if (ret < 0)
    return ret;

  if (write)
    {
      if (*valp)
	nat_word |= mask;
      else
	nat_word &= ~mask;
      ret = ia64_put (c, nat_loc, nat_word);
    }
  else
    *valp = (nat_word & mask) != 0;
  return ret;
}

static int
access_nat (struct ia64_cursor *c, unw_word_t loc, unw_word_t reg_loc,
	    unw_word_t *valp, int write)
{
  unw_word_t nat_loc = -8, mask = 0, sc_addr;
  unw_fpreg_t tmp;
  int ret, reg;

  if (IA64_IS_FP_LOC (reg_loc))
    {
      /* NaT bit is saved as a NaTVal.  This happens when a general
	 register is saved to a floating-point register.  */
      if (write)
	{
	  if (*valp)
	    {
	      if (c->pi.flags & IA64_FLAG_BIG_ENDIAN)
		ret = ia64_putfp (c, reg_loc, unw.nat_val_be);
	      else
		ret = ia64_putfp (c, reg_loc, unw.nat_val_le);
	    }
	  else
	    {
	      unw_fpreg_t tmp;

	      ret = ia64_getfp (c, reg_loc, &tmp);
	      if (ret < 0)
		return ret;

	      /* Reset the exponent to 0x1003e so that the significand
		 will be interpreted as an integer value.  */
	      if (c->pi.flags & IA64_FLAG_BIG_ENDIAN)
		tmp.raw.bits[0] = unw.int_val_be.raw.bits[0];
	      else
		tmp.raw.bits[1] = unw.int_val_le.raw.bits[1];

	      ret = ia64_putfp (c, reg_loc, tmp);
	    }
	}
      else
	{
	  ret = ia64_getfp (c, reg_loc, &tmp);
	  if (ret < 0)
	    return ret;

	  if (c->pi.flags & IA64_FLAG_BIG_ENDIAN)
	    *valp = (memcmp (&tmp, &unw.nat_val_be, sizeof (tmp)) == 0);
	  else
	    *valp = (memcmp (&tmp, &unw.nat_val_le, sizeof (tmp)) == 0);
	}
      return ret;
    }

  if (IA64_IS_MEMSTK_NAT (loc))
    {
      nat_loc = IA64_GET_LOC (loc) << 3;
      mask = (unw_word_t) 1 << ia64_rse_slot_num (reg_loc);
    }
  else
    {
      reg = IA64_GET_LOC (loc);
      assert (reg >= 0 && reg < 128);
      if (!reg)
	{
	  /* NaT bit is not saved. This happens if a general register
	     is saved to a branch register.  Since the NaT bit gets
	     lost, we need to drop it here, too.  Note that if the NaT
	     bit had been set when the save occurred, it would have
	     caused a NaT consumption fault.  */
	  if (write)
	    {
	      if (*valp)
		return -UNW_EBADREG;	/* can't set NaT bit */
	    }
	  else
	    *valp = 0;
	  return 0;
	}

      if (reg >= 4 && reg <= 7)
	{
	  /* NaT bit is saved in a NaT register.  This happens when a
	     general register is saved to another general
	     register.  */
#ifdef UNW_LOCAL_ONLY
	  mask = ((unw_word_t) 1) << reg;
	  nat_loc = (unw_word_t) &c->uc->uc_mcontext.sc_nat;
#else
	  if (write)
	    ret = ia64_put (c, IA64_REG_LOC (c, UNW_IA64_NAT + reg), *valp);
	  else
	    ret = ia64_get (c, IA64_REG_LOC (c, UNW_IA64_NAT + reg), valp);
	  return ret;
#endif
	}
      else if (reg >= 32)
	{
	  /* NaT bit is saved in a stacked register.  */
	  nat_loc = ia64_rse_rnat_addr (reg_loc);	/* XXX looks wrong */
	  if (nat_loc > c->rbs_top)
	    nat_loc = c->top_rnat_loc;
	  mask = (unw_word_t) 1 << ia64_rse_slot_num (reg_loc);
	}
      else
	{
	  /* NaT bit is saved in a scratch register.  */
	  sc_addr = c->sigcontext_loc;
	  if (sc_addr)
	    {
	      nat_loc = sc_addr + SIGCONTEXT_NAT_OFF;
	      mask = (unw_word_t) 1 << reg;
	    }
	  else
	    {
	      if (write)
		return ia64_put (c, loc, *valp);
	      else
		return ia64_get (c, loc, valp);
	    }
	}
    }
  return update_nat (c, nat_loc, mask, valp, write);
}

int
ia64_access_reg (struct ia64_cursor *c, unw_regnum_t reg, unw_word_t *valp,
		 int write)
{
  unw_word_t loc = -8, reg_loc, nat, nat_loc, cfm, mask, pr;
  int ret, readonly = 0;

  switch (reg)
    {
      /* frame registers: */

    case UNW_IA64_BSP:
      if (write)
	return -UNW_EREADONLYREG;
      *valp = c->bsp;
      return 0;

    case UNW_REG_SP:
    case UNW_REG_PROC_START:
    case UNW_REG_HANDLER:
    case UNW_REG_LSDA:
      if (write)
	return -UNW_EREADONLYREG;
      switch (reg)
	{
	case UNW_REG_SP:	*valp = c->sp; break;
	case UNW_REG_PROC_START:*valp = c->pi.proc_start; break;
	case UNW_REG_LSDA:
	  *valp = (intptr_t) (c->pi.pers_addr + 1);
	  break;
	case UNW_REG_HANDLER:
	  if (c->pi.flags & IA64_FLAG_HAS_HANDLER)
	    /* *c->pers_addr is the linkage-table offset of the word
	       that stores the address of the personality routine's
	       function descriptor.  */
	    return ia64_get (c, *c->pi.pers_addr + c->pi.gp, valp);
	  else
	    *valp = 0;
	  break;
	}
      return 0;

    case UNW_REG_IP:
      if (write)
	c->ip = *valp;	/* also update the IP cache */
      loc = c->ip_loc;
      break;

      /* preserved registers: */

    case UNW_IA64_GR + 4 ... UNW_IA64_GR + 7:
      loc = (&c->r4_loc)[reg - (UNW_IA64_GR + 4)];
      break;

    case UNW_IA64_NAT + 4 ... UNW_IA64_NAT + 7:
      loc = (&c->nat4_loc)[reg - (UNW_IA64_NAT + 4)];
      reg_loc = (&c->r4_loc)[reg - (UNW_IA64_NAT + 4)];
      return access_nat (c, loc, reg_loc, valp, write);

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

    case UNW_IA64_PR:
      if (write)
	{
	  c->pr = *valp;		/* update the predicate cache */
	  pr = pr_ltop (c, *valp);
	  return ia64_put (c, c->pr_loc, pr);
	}
      else
	{
	  ret = ia64_get (c, c->pr_loc, &pr);
	  if (ret < 0)
	    return ret;
	  *valp = pr_ptol (c, pr);
	}
      return 0;

    case UNW_IA64_GR + 32 ... UNW_IA64_GR + 127:	/* stacked reg */
      reg = rotate_gr (c, reg - UNW_IA64_GR) + UNW_IA64_GR;
      if (reg < 0)
	return -UNW_EBADREG;
      loc = ia64_rse_skip_regs (c->bsp, reg - (UNW_IA64_GR + 32));
      break;

    case UNW_IA64_NAT + 32 ... UNW_IA64_NAT + 127:	/* stacked reg */
      reg = rotate_gr (c, reg - UNW_IA64_NAT) + UNW_IA64_NAT;
      if (reg < 0)
	return -UNW_EBADREG;
      loc = ia64_rse_skip_regs (c->bsp, reg - (UNW_IA64_NAT + 32));
      nat_loc = ia64_rse_rnat_addr (loc);
      if (nat_loc > c->rbs_top)
	nat_loc = c->top_rnat_loc;
      mask = (unw_word_t) 1 << ia64_rse_slot_num (loc);
      return update_nat (c, nat_loc, mask, valp, write);

    case UNW_IA64_AR_EC:
      ret = ia64_get (c, c->cfm_loc, &cfm);
      if (ret < 0)
	return ret;
      if (write)
	ret = ia64_put (c, c->cfm_loc, ((cfm & ~((unw_word_t) 0x3f << 52))
					| (*valp & 0x3f) << 52));
      else
	*valp = (cfm >> 52) & 0x3f;
      return ret;


      /* scratch & special registers: */

    case UNW_IA64_GR + 0:
      if (write)
	return -UNW_EREADONLYREG;
      *valp = 0;
      return 0;

    case UNW_IA64_GR + 1:				/* global pointer */
      if (write)
	return -UNW_EREADONLYREG;
      *valp = c->pi.gp;
      return 0;

    case UNW_IA64_NAT + 0:
    case UNW_IA64_NAT + 1:				/* global pointer */
      if (write)
	return -UNW_EREADONLYREG;
      *valp = 0;
      return 0;

    case UNW_IA64_NAT + 2 ... UNW_IA64_NAT + 3:
    case UNW_IA64_NAT + 8 ... UNW_IA64_NAT + 31:
      loc = ia64_scratch_loc (c, reg);
      if (c->sigcontext_loc)
	{
	  mask = (unw_word_t) 1 << (reg - UNW_IA64_NAT);

	  ret = ia64_get (c, loc, &nat);
	  if (ret < 0)
	    return ret;

	  if (write)
	    {
	      if (*valp)
		nat |= mask;
	      else
		nat &= ~mask;
	      ret = ia64_put (c, loc, nat);
	    }
	  else
	    *valp = (nat & mask) != 0;
	  return ret;
	}
      break;

    case UNW_IA64_GR + 15 ... UNW_IA64_GR + 18:
      if ((c->pi.flags & IA64_FLAG_SIGTRAMP) != 0)
	loc = ia64_scratch_loc (c, reg);
      else
	{
	  if (write)
	    c->eh_args[reg - (UNW_IA64_GR + 15)] = *valp;
	  else
	    *valp = c->eh_args[reg - (UNW_IA64_GR + 15)] = *valp;
	  return 0;
	}
      break;

    case UNW_IA64_GR +  2 ... UNW_IA64_GR + 3:
    case UNW_IA64_GR +  8 ... UNW_IA64_GR + 14:
    case UNW_IA64_GR + 19 ... UNW_IA64_GR + 31:
    case UNW_IA64_BR + 0:
    case UNW_IA64_BR + 6:
    case UNW_IA64_BR + 7:
    case UNW_IA64_AR_RSC:
    case UNW_IA64_AR_25:
    case UNW_IA64_AR_26:
    case UNW_IA64_AR_CCV:
      loc = ia64_scratch_loc (c, reg);
      break;

    default:
      dprintf ("%s: bad register number %d\n", __FUNCTION__, reg);
      return -UNW_EBADREG;
    }

  if (write)
    {
      if (readonly)
	return -UNW_EREADONLYREG;
      return ia64_put (c, loc, *valp);
    }
  else
    return ia64_get (c, loc, valp);
}

int
ia64_access_fpreg (struct ia64_cursor *c, int reg, unw_fpreg_t *valp,
		   int write)
{
  unw_word_t loc = -8, flags, tmp_loc;
  int ret, i;

  switch (reg)
    {
    case UNW_IA64_FR + 0:
      if (write)
	return -UNW_EREADONLYREG;
      *valp = unw.f0;
      return 0;

    case UNW_IA64_FR + 1:
      if (write)
	return -UNW_EREADONLYREG;

      if (c->pi.flags & IA64_FLAG_BIG_ENDIAN)
	*valp = unw.f1_be;
      else
	*valp = unw.f1_le;
      return 0;

    case UNW_IA64_FR + 2: loc = c->f2_loc; break;
    case UNW_IA64_FR + 3: loc = c->f3_loc; break;
    case UNW_IA64_FR + 4: loc = c->f4_loc; break;
    case UNW_IA64_FR + 5: loc = c->f5_loc; break;
    case UNW_IA64_FR + 16 ... UNW_IA64_FR + 31:
      loc = c->fr_loc[reg - (UNW_IA64_FR + 16)];
      break;

    case UNW_IA64_FR + 6 ... UNW_IA64_FR + 15:
      loc = ia64_scratch_loc (c, reg);
      break;

    case UNW_IA64_FR + 32 ... UNW_IA64_FR + 127:
      loc = c->sigcontext_loc;
      if (loc)
	{
	  ret = ia64_get (c, loc + SIGCONTEXT_FLAGS_OFF, &flags);
	  if (ret < 0)
	    return ret;

	  if (!(flags & IA64_SC_FLAG_FPH_VALID))
	    {
	      if (write)
		{
		  /* initialize fph partition: */
		  tmp_loc = loc + SIGCONTEXT_FR_OFF + 32*16;
		  for (i = 32; i < 128; ++i, tmp_loc += 16)
		    {
		      ret = ia64_putfp (c, tmp_loc, unw.f0);
		      if (ret < 0)
			return ret;
		    }
		  /* mark fph partition as valid: */
		  ret = ia64_put (c, loc + SIGCONTEXT_FLAGS_OFF,
				  flags | IA64_SC_FLAG_FPH_VALID);
		  if (ret < 0)
		    return ret;
		}
	      else
		{
		  *valp = unw.f0;
		  return 0;
		}
	    }
	}
      reg = rotate_fr (c, reg - UNW_IA64_FR) + UNW_IA64_FR;
      loc = ia64_scratch_loc (c, reg);
      break;
    }

  if (write)
    return ia64_putfp (c, loc, *valp);
  else
    return ia64_getfp (c, loc, valp);
}
