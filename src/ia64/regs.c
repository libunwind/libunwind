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

#include "config.h"

#include <assert.h>
#include "unwind_i.h"
#include "rse.h"

/* The first three 64-bit words in a signal frame contain the signal
   number, siginfo pointer, and sigcontext pointer passed to the
   signal handler.  We use this to locate the sigcontext pointer.  */
#define SIGFRAME_ARG2_OFF	0x10

unw_word_t
ia64_get_sigcontext_addr (struct ia64_cursor *c)
{
  unw_word_t addr;

  if (!(c->pi.flags & IA64_FLAG_SIGTRAMP))
    return 0;

  if (ia64_get (c, c->sp + 0x10 + SIGFRAME_ARG2_OFF, &addr) < 0)
    return 0;

  return addr;
}

/* Apply rotation to a general register.  The number REG must be in
   the range of 0-127.  */

static inline int
rotate_gr (struct ia64_cursor *c, int reg)
{
  unsigned int rrb_gr, sor;
  unw_word_t cfm;
  int preg, ret;

  ret = ia64_get (c, c->cfm_loc, &cfm);
  if (ret < 0)
    return ret;

  sor = 8 * ((cfm >> 14) & 0xf);
  rrb_gr = (cfm >> 18) & 0x7f;

  if ((unsigned) (reg - 32) > sor)
    preg = reg;		/* register not part of the rotating partition */
  else
    {
      preg = reg + rrb_gr;	/* apply rotation */
      if (preg > 32 + sor)
	preg -= sor;		/* wrap around */
    }
  debug (100, "%s: sor=%u rrb.gr=%u, r%d -> r%d\n", __FUNCTION__, sor, rrb_gr,
	 reg, preg);
  return preg;
}

/* Apply rotation to a floating-point register.  The number REG must
   be in the range of 0-127.  */

static inline int
rotate_fr (struct ia64_cursor *c, int reg)
{
  unsigned int rrb_fr;
  unw_word_t cfm;
  int preg, ret;

  ret = ia64_get (c, c->cfm_loc, &cfm);
  if (ret < 0)
    return ret;

  rrb_fr = (cfm >> 25) & 0x7f;
  if (reg < 32)
    preg = reg;		/* register not part of the rotating partition */
  else
    {
      preg = reg + rrb_fr;	/* apply rotation */
      if (preg > 127)
	preg -= 96;		/* wrap around */
    }
  debug (100, "%s: rrb.fr=%u, f%d -> f%d\n", __FUNCTION__, rrb_fr, reg, preg);
  return preg;
}

/* Apply logical-to-physical rotation.  */

static inline unw_word_t
pr_ltop (struct ia64_cursor *c, unw_word_t pr)
{
  unw_word_t rrb_pr, mask, rot, cfm;
  int ret;

  ret = ia64_get (c, c->cfm_loc, &cfm);
  if (ret < 0)
    return ret;

  rrb_pr = (cfm >> 32) & 0x3f;
  rot = pr >> 16;
  mask = ((unw_word_t) 1 << rrb_pr) - 1;
  rot = ((pr & mask) << (48 - rrb_pr)) | ((pr >> rrb_pr) & mask);
  return (pr & 0xffff) | (rot << 16);
}

/* Apply physical-to-logical rotation.  */

static inline unw_word_t
pr_ptol (struct ia64_cursor *c, unw_word_t pr)
{
  unw_word_t rrb_pr, mask, rot, cfm;
  int ret;

  ret = ia64_get (c, c->cfm_loc, &cfm);
  if (ret < 0)
    return ret;

  rrb_pr = 48 - ((cfm >> 32) & 0x3f);
  rot = pr >> 16;
  mask = ((unw_word_t) 1 << rrb_pr) - 1;
  rot = ((pr & mask) << (48 - rrb_pr)) | ((pr >> rrb_pr) & mask);
  return (pr & 0xffff) | (rot << 16);
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
  int ret;

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
      mask = (unw_word_t) 1 << ia64_rse_slot_num ((unsigned long *) reg_loc);
    }
  else
    {
      loc = IA64_GET_LOC (loc);
      assert (loc >= 0 && loc < 128);
      if (!loc)
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

      if (loc >= 4 && loc <= 7)
	{
	  /* NaT bit is saved in a NaT register.  This happens when a
	     general register is saved to another general
	     register.  */
	  if (write)
	    ret = ia64_put (c, UNW_IA64_NAT + loc, *valp);
	  else
	    ret = ia64_get (c, UNW_IA64_NAT + loc, valp);
	  return ret;
	}
      else if (loc >= 32)
	{
	  /* NaT bit is saved in a stacked register.  */
	  nat_loc = (unw_word_t) ia64_rse_rnat_addr ((unsigned long *)
						     reg_loc);
	  if (nat_loc > c->rbs_top)
	    nat_loc = c->top_rnat_loc;
	  mask = (unw_word_t) 1 << ia64_rse_slot_num ((unsigned long *)
						      reg_loc);
	}
      else
	{
	  /* NaT bit is saved in a scratch register.  */
	  if (!(c->pi.flags & IA64_FLAG_SIGTRAMP))
	    return -UNW_EBADREG;

	  sc_addr = ia64_get_sigcontext_addr (c);
	  if (!sc_addr)
	    return -UNW_EBADREG;

	  nat_loc = sc_addr + struct_offset (struct sigcontext, sc_nat);
	  mask = (unw_word_t) 1 << loc;
	}
    }
  return update_nat (c, nat_loc, mask, valp, write);
}

int
ia64_access_reg (struct ia64_cursor *c, unw_regnum_t reg, unw_word_t *valp,
		 int write)
{
  unw_word_t loc = -8, reg_loc, sc_off = 0, nat, nat_loc, cfm, mask, pr;
  int ret, readonly = 0;

  switch (reg)
    {
      /* frame registers: */

    case UNW_IA64_CURRENT_BSP:
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
	  *valp = (unw_word_t) (c->pi.pers_addr + 1);
	  break;
	case UNW_REG_HANDLER:
	  if (c->pi.flags & IA64_FLAG_HAS_HANDLER)
	    /* *c->pers_addr is the linkage-table offset of the word
	       that stores the address of the personality routine's
	       function descriptor.  */
	    *valp = *(unw_word_t *) (*c->pi.pers_addr + c->pi.gp);
	  else
	    *valp = 0;
	  break;
	}
      return 0;

    case UNW_REG_IP:
      loc = c->rp_loc;
      break;

      /* preserved registers: */

    case UNW_IA64_GR + 4:	loc = c->r4_loc; break;
    case UNW_IA64_GR + 5:	loc = c->r5_loc; break;
    case UNW_IA64_GR + 6:	loc = c->r6_loc; break;
    case UNW_IA64_GR + 7:	loc = c->r7_loc; break;
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
      loc = (unw_word_t) ia64_rse_skip_regs ((unsigned long *) c->bsp,
					     reg - (UNW_IA64_GR + 32));
      break;

    case UNW_IA64_NAT + 32 ... UNW_IA64_NAT + 127:	/* stacked reg */
      reg = rotate_gr (c, reg - UNW_IA64_NAT) + UNW_IA64_NAT;
      loc = (unw_word_t) ia64_rse_skip_regs ((unsigned long *) c->bsp,
					     reg - (UNW_IA64_NAT + 32));
      nat_loc = (unw_word_t) ia64_rse_rnat_addr ((unsigned long *) loc);
      if (nat_loc > c->rbs_top)
	nat_loc = c->top_rnat_loc;
      mask = (unw_word_t) 1 << ia64_rse_slot_num ((unsigned long *) loc);
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

    case UNW_IA64_GR + 15 ... UNW_IA64_GR + 18:
      if (!(c->pi.flags & IA64_FLAG_SIGTRAMP))
	{
	  if (write)
	    c->eh_args[reg - (UNW_IA64_GR + 15)] = *valp;
	  else
	    *valp = c->eh_args[reg - (UNW_IA64_GR + 15)] = *valp;
	  return 0;
	}
      else
	sc_off = struct_offset (struct sigcontext, sc_gr[reg]);
      break;

    case UNW_IA64_GR +  2 ... UNW_IA64_GR + 3:
    case UNW_IA64_GR +  8 ... UNW_IA64_GR + 14:
    case UNW_IA64_GR + 19 ... UNW_IA64_GR + 31:
      sc_off = struct_offset (struct sigcontext, sc_gr[reg]);
      break;

    case UNW_IA64_NAT + 0:
    case UNW_IA64_NAT + 1:				/* global pointer */
      if (write)
	return -UNW_EREADONLYREG;
      *valp = 0;
      return 0;

    case UNW_IA64_NAT + 2 ... UNW_IA64_NAT + 3:
    case UNW_IA64_NAT + 8 ... UNW_IA64_NAT + 31:
      mask = (unw_word_t) 1 << (reg - UNW_IA64_NAT);
      loc = ia64_get_sigcontext_addr (c);
      if (!loc)
	return -UNW_EBADREG;

      loc += struct_offset (struct sigcontext, sc_nat);

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

    case UNW_IA64_NAT + 4 ... UNW_IA64_NAT + 7:
      loc = (&c->nat4_loc)[reg - (UNW_IA64_NAT + 4)];
      reg_loc = (&c->r4_loc)[reg - (UNW_IA64_NAT + 4)];
      return access_nat (c, loc, reg_loc, valp, write);

    case UNW_IA64_AR_RSC:
      sc_off = struct_offset (struct sigcontext, sc_ar_rsc);
      break;

#ifdef SIGCONTEXT_HAS_AR25_AND_AR26
    case UNW_IA64_AR_25:
      sc_off = struct_offset (struct sigcontext, sc_ar25);
      break;

    case UNW_IA64_AR_26:
      sc_off = struct_offset (struct sigcontext, sc_ar26);
      break;
#endif

    case UNW_IA64_AR_CCV:
      sc_off = struct_offset (struct sigcontext, sc_ar_ccv);
      break;

    default:
      dprintf ("%s: bad register number %d\n", __FUNCTION__, reg);
      return -UNW_EBADREG;
    }

  if (sc_off)
    {
      loc = ia64_get_sigcontext_addr (c);
      if (!loc)
	return -UNW_EBADREG;

      loc += sc_off;
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
      loc = ia64_get_sigcontext_addr (c);
      if (!loc)
	return -UNW_EBADREG;
      loc += struct_offset (struct sigcontext, sc_fr[reg - UNW_IA64_FR]);
      break;

    case UNW_IA64_FR + 32 ... UNW_IA64_FR + 127:
      reg = rotate_fr (c, reg - UNW_IA64_FR) + UNW_IA64_FR;
      loc = ia64_get_sigcontext_addr (c);
      if (!loc)
	return -UNW_EBADREG;

      ret = ia64_get (c, loc + struct_offset (struct sigcontext, sc_flags),
		      &flags);
      if (ret < 0)
	return ret;

      if (!(flags & IA64_SC_FLAG_FPH_VALID))
	{
	  if (write)
	    {
	      /* initialize fph partition: */
	      tmp_loc = loc + struct_offset (struct sigcontext, sc_fr[32]);
	      for (i = 32; i < 128; ++i, tmp_loc += 16)
		{
		  ret = ia64_putfp (c, tmp_loc, unw.f0);
		  if (ret < 0)
		    return ret;
		}
	      /* mark fph partition as being valid: */
	      ret = ia64_put (c, loc + struct_offset (struct sigcontext,
						      sc_flags),
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
      loc += struct_offset (struct sigcontext, sc_fr[reg - UNW_IA64_FR]);
      break;
    }

  if (write)
    return ia64_putfp (c, loc, *valp);
  else
    return ia64_getfp (c, loc, valp);
}
