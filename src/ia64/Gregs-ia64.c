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

#include <assert.h>

#include "offsets.h"
#include "regs.h"
#include "rse.h"
#include "unwind_i.h"

HIDDEN ia64_loc_t
ia64_scratch_loc (struct cursor *c, unw_regnum_t reg)
{
  unw_word_t sc_addr = c->sigcontext_addr;

  if (sc_addr)
    {
      switch (reg)
	{
	case UNW_IA64_NAT + 2 ... UNW_IA64_NAT + 3:
	case UNW_IA64_NAT + 8 ... UNW_IA64_NAT + 31:
	  sc_addr += LINUX_SC_NAT_OFF;
	  break;

	case UNW_IA64_GR +  2 ... UNW_IA64_GR + 3:
	case UNW_IA64_GR +  8 ... UNW_IA64_GR + 31:
	  sc_addr += LINUX_SC_GR_OFF + 8*reg;
	  break;

	case UNW_IA64_FR + 6 ... UNW_IA64_FR + 15:
	  sc_addr += LINUX_SC_FR_OFF + 16*(reg - UNW_IA64_FR);
	  return IA64_LOC_ADDR (sc_addr, IA64_LOC_TYPE_FP);

	case UNW_IA64_FR + 32 ... UNW_IA64_FR + 127:
	  sc_addr += LINUX_SC_FR_OFF + 16*(reg - UNW_IA64_FR);
	  return IA64_LOC_ADDR (sc_addr, IA64_LOC_TYPE_FP);

	case UNW_IA64_BR + 0: sc_addr += LINUX_SC_BR_OFF + 0; break;
	case UNW_IA64_BR + 6: sc_addr += LINUX_SC_BR_OFF + 6*8; break;
	case UNW_IA64_BR + 7: sc_addr += LINUX_SC_BR_OFF + 7*8; break;
	case UNW_IA64_AR_RSC: sc_addr += LINUX_SC_AR_RSC_OFF; break;
	case UNW_IA64_AR_CSD: sc_addr += LINUX_SC_AR_CSD_OFF; break;
	case UNW_IA64_AR_26:  sc_addr += LINUX_SC_AR_26_OFF; break;
	case UNW_IA64_AR_CCV: sc_addr += LINUX_SC_AR_CCV; break;
	}
      return IA64_LOC_ADDR (sc_addr, 0);
    }
  else
    return IA64_REG_LOC (c, reg);
}

static inline int
update_nat (struct cursor *c, ia64_loc_t nat_loc, unw_word_t mask,
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
access_nat (struct cursor *c, ia64_loc_t loc, ia64_loc_t reg_loc,
	    unw_word_t *valp, int write)
{
  unw_word_t mask = 0, sc_addr;
  ia64_loc_t nat_loc;
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
	      if (c->as->big_endian)
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
	      if (c->as->big_endian)
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

	  if (c->as->big_endian)
	    *valp = (memcmp (&tmp, &unw.nat_val_be, sizeof (tmp)) == 0);
	  else
	    *valp = (memcmp (&tmp, &unw.nat_val_le, sizeof (tmp)) == 0);
	}
      return ret;
    }

  if (IA64_IS_MEMSTK_NAT (loc))
    {
      nat_loc = IA64_LOC_ADDR (IA64_GET_ADDR (loc), 0);
      assert (!IA64_IS_REG_LOC (reg_loc));
      mask = (unw_word_t) 1 << ia64_rse_slot_num (IA64_GET_ADDR (reg_loc));
    }
  else
    {
      reg = IA64_GET_REG (loc);
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
	  /* NaT bit is saved in a NaT register.  This happens when
	     one general register is saved to another general
	     register.  */
#ifdef UNW_LOCAL_ONLY
	  ucontext_t *uc = c->as_arg;
	  mask = ((unw_word_t) 1) << reg;
	  nat_loc = IA64_LOC_ADDR ((unw_word_t) &uc->uc_mcontext.sc_nat, 0);
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
	  reg = rotate_gr (c, reg);
	  ret = ia64_get_stacked (c, reg, &reg_loc, &nat_loc);
	  if (ret < 0)
	    return ret;
	  assert (!IA64_IS_REG_LOC (reg_loc));
	  mask = (unw_word_t) 1 << ia64_rse_slot_num (IA64_GET_ADDR (reg_loc));
	}
      else
	{
	  /* NaT bit is saved in a scratch register.  */
	  sc_addr = c->sigcontext_addr;
	  if (sc_addr)
	    {
	      nat_loc = IA64_LOC_ADDR (sc_addr + LINUX_SC_NAT_OFF, 0);
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

HIDDEN int
ia64_access_reg (struct cursor *c, unw_regnum_t reg, unw_word_t *valp,
		 int write)
{
  ia64_loc_t loc, reg_loc, nat_loc;
  unw_word_t nat, mask, pr;
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
      if (write)
	return -UNW_EREADONLYREG;
      *valp = c->sp;
      return 0;

    case UNW_REG_IP:
      if (write)
	{
	  c->ip = *valp;	/* also update the IP cache */
	  if (c->pi_valid && (*valp < c->pi.start_ip || *valp >= c->pi.end_ip))
	    c->pi_valid = 0;	/* new IP outside of current proc */
	}
      loc = c->loc[IA64_REG_IP];
      break;

      /* preserved registers: */

    case UNW_IA64_GR + 4 ... UNW_IA64_GR + 7:
      loc = c->loc[IA64_REG_R4 + (reg - (UNW_IA64_GR + 4))];
      break;

    case UNW_IA64_NAT + 4 ... UNW_IA64_NAT + 7:
      loc = c->loc[IA64_REG_NAT4 + (reg - (UNW_IA64_NAT + 4))];
      reg_loc = c->loc[IA64_REG_R4 + (reg - (UNW_IA64_NAT + 4))];
      return access_nat (c, loc, reg_loc, valp, write);

    case UNW_IA64_AR_BSP:	loc = c->loc[IA64_REG_BSP]; break;
    case UNW_IA64_AR_BSPSTORE:	loc = c->loc[IA64_REG_BSPSTORE]; break;
    case UNW_IA64_AR_PFS:	loc = c->loc[IA64_REG_PFS]; break;
    case UNW_IA64_AR_RNAT:	loc = c->loc[IA64_REG_RNAT]; break;
    case UNW_IA64_AR_UNAT:	loc = c->loc[IA64_REG_UNAT]; break;
    case UNW_IA64_AR_LC:	loc = c->loc[IA64_REG_LC]; break;
    case UNW_IA64_AR_FPSR:	loc = c->loc[IA64_REG_FPSR]; break;
    case UNW_IA64_BR + 1:	loc = c->loc[IA64_REG_B1]; break;
    case UNW_IA64_BR + 2:	loc = c->loc[IA64_REG_B2]; break;
    case UNW_IA64_BR + 3:	loc = c->loc[IA64_REG_B3]; break;
    case UNW_IA64_BR + 4:	loc = c->loc[IA64_REG_B4]; break;
    case UNW_IA64_BR + 5:	loc = c->loc[IA64_REG_B5]; break;

    case UNW_IA64_CFM:
      if (write)
	c->cfm = *valp;	/* also update the CFM cache */
      loc = c->cfm_loc;
      break;

    case UNW_IA64_PR:
      if (write)
	{
	  c->pr = *valp;		/* update the predicate cache */
	  pr = pr_ltop (c, *valp);
	  return ia64_put (c, c->loc[IA64_REG_PR], pr);
	}
      else
	{
	  ret = ia64_get (c, c->loc[IA64_REG_PR], &pr);
	  if (ret < 0)
	    return ret;
	  *valp = pr_ptol (c, pr);
	}
      return 0;

    case UNW_IA64_GR + 32 ... UNW_IA64_GR + 127:	/* stacked reg */
      reg = rotate_gr (c, reg - UNW_IA64_GR);
      if (reg < 0)
	return -UNW_EBADREG;
      ret = ia64_get_stacked (c, reg, &loc, NULL);
      if (ret < 0)
	return ret;
      break;

    case UNW_IA64_NAT + 32 ... UNW_IA64_NAT + 127:	/* stacked reg */
      reg = rotate_gr (c, reg - UNW_IA64_NAT);
      if (reg < 0)
	return -UNW_EBADREG;
      ret = ia64_get_stacked (c, reg, &loc, &nat_loc);
      if (ret < 0)
	return ret;
      assert (!IA64_IS_REG_LOC (loc));
      mask = (unw_word_t) 1 << ia64_rse_slot_num (IA64_GET_ADDR (loc));
      return update_nat (c, nat_loc, mask, valp, write);

    case UNW_IA64_AR_EC:
      if (write)
	{
	  c->cfm = ((c->cfm & ~((unw_word_t) 0x3f << 52))
		    | ((*valp & 0x3f) << 52));
	  return ia64_put (c, c->cfm_loc, c->cfm);
	}
      else
	{
	  *valp = (c->cfm >> 52) & 0x3f;
	  return 0;
	}

      /* scratch & special registers: */

    case UNW_IA64_GR + 0:
      if (write)
	return -UNW_EREADONLYREG;
      *valp = 0;
      return 0;

    case UNW_IA64_GR + 1:				/* global pointer */
      if (write)
	return -UNW_EREADONLYREG;

      /* ensure c->pi is up-to-date: */
      if ((ret = ia64_make_proc_info (c)) < 0)
	return ret;
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
      if (c->sigcontext_addr)
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
      mask = 1 << (reg - (UNW_IA64_GR + 15));
      if (write)
	{
	  c->eh_args[reg - (UNW_IA64_GR + 15)] = *valp;
	  c->eh_valid_mask |= mask;
	  return 0;
	}
      else if ((c->eh_valid_mask & mask) != 0)
	{
	  *valp = c->eh_args[reg - (UNW_IA64_GR + 15)] = *valp;
	  return 0;
	}
      else
	loc = ia64_scratch_loc (c, reg);
      break;

    case UNW_IA64_GR +  2 ... UNW_IA64_GR + 3:
    case UNW_IA64_GR +  8 ... UNW_IA64_GR + 14:
    case UNW_IA64_GR + 19 ... UNW_IA64_GR + 31:
    case UNW_IA64_BR + 0:
    case UNW_IA64_BR + 6:
    case UNW_IA64_BR + 7:
    case UNW_IA64_AR_RSC:
    case UNW_IA64_AR_CSD:
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

HIDDEN int
ia64_access_fpreg (struct cursor *c, int reg, unw_fpreg_t *valp,
		   int write)
{
  unw_word_t flags, tmp_addr, sc_addr;
  ia64_loc_t loc;
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

      if (c->as->big_endian)
	*valp = unw.f1_be;
      else
	*valp = unw.f1_le;
      return 0;

    case UNW_IA64_FR + 2: loc = c->loc[IA64_REG_F2]; break;
    case UNW_IA64_FR + 3: loc = c->loc[IA64_REG_F3]; break;
    case UNW_IA64_FR + 4: loc = c->loc[IA64_REG_F4]; break;
    case UNW_IA64_FR + 5: loc = c->loc[IA64_REG_F5]; break;
    case UNW_IA64_FR + 16 ... UNW_IA64_FR + 31:
      loc = c->loc[IA64_REG_F16 + (reg - (UNW_IA64_FR + 16))];
      break;

    case UNW_IA64_FR + 6 ... UNW_IA64_FR + 15:
      loc = ia64_scratch_loc (c, reg);
      break;

    case UNW_IA64_FR + 32 ... UNW_IA64_FR + 127:
      sc_addr = c->sigcontext_addr;
      if (sc_addr)
	{
	  ret = ia64_get (c, IA64_LOC_ADDR (sc_addr + LINUX_SC_FLAGS_OFF, 0),
			  &flags);
	  if (ret < 0)
	    return ret;

	  if (!(flags & IA64_SC_FLAG_FPH_VALID))
	    {
	      if (write)
		{
		  /* initialize fph partition: */
		  tmp_addr = sc_addr + LINUX_SC_FR_OFF + 32*16;
		  for (i = 32; i < 128; ++i, tmp_addr += 16)
		    {
		      ret = ia64_putfp (c, IA64_LOC_ADDR (tmp_addr, 0),
					unw.f0);
		      if (ret < 0)
			return ret;
		    }
		  /* mark fph partition as valid: */
		  ret = ia64_put (c, IA64_LOC_ADDR (sc_addr
						    + LINUX_SC_FLAGS_OFF, 0),
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

    default:
      dprintf ("%s: bad register number %d\n", __FUNCTION__, reg);
      return -UNW_EBADREG;
    }

  if (write)
    return ia64_putfp (c, loc, *valp);
  else
    return ia64_getfp (c, loc, valp);
}
