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

#include <string.h>

#include "rse.h"
#include "unwind_i.h"

#ifdef UNW_REMOTE_ONLY

int
unw_init_local (unw_cursor_t *cursor, ucontext_t *uc)
{
  return -UNW_EINVAL;
}

#else /* !UNW_REMOTE_ONLY */

#ifndef UNW_LOCAL_ONLY

static int
access_mem (unw_word_t addr, unw_word_t *val, int write, void *arg)
{
  if (write)
    {
      debug (100, "%s: mem[%lx] <- %lx\n", __FUNCTION__, addr, *val);
      *(unw_word_t *) addr = *val;
    }
  else
    {
      *val = *(unw_word_t *) addr;
      debug (100, "%s: mem[%lx] -> %lx\n", __FUNCTION__, addr, *val);
    }
  return 0;
}

static int
access_reg (unw_regnum_t reg, unw_word_t *val, int write, void *arg)
{
  ucontext_t *uc = arg;
  unw_word_t *addr, mask, sol;

  switch (reg)
    {
    case UNW_IA64_IP:		addr = &uc->uc_mcontext.sc_br[0]; break;
    case UNW_IA64_SP:		addr = &uc->uc_mcontext.sc_gr[12]; break;
    case UNW_IA64_CFM:		addr = &uc->uc_mcontext.sc_ar_pfs; break;

    case UNW_IA64_AR_RNAT:	addr = &uc->uc_mcontext.sc_ar_rnat; break;
    case UNW_IA64_AR_UNAT:	addr = &uc->uc_mcontext.sc_ar_unat; break;
    case UNW_IA64_AR_LC:	addr = &uc->uc_mcontext.sc_ar_lc; break;
    case UNW_IA64_AR_FPSR:	addr = &uc->uc_mcontext.sc_ar_fpsr; break;
    case UNW_IA64_PR:		addr = &uc->uc_mcontext.sc_pr; break;

    case UNW_IA64_AR_BSP:
    case UNW_IA64_AR_BSPSTORE:
      /* bsp and bspstore are equal after a flushrs.  Account for the
         fact that sc_ar_bsp points to *end* of register frame of the
         initial call frame.  */
      if (write)
	{
	  sol = (uc->uc_mcontext.sc_ar_pfs >> 7) & 0x7f;
	  uc->uc_mcontext.sc_ar_bsp = ia64_rse_skip_regs (*val, sol);
	  debug (100, "%s: %s <- %lx\n",
		 __FUNCTION__, _U_ia64_regname (reg), *val);
	}
      else
	{
	  sol = (uc->uc_mcontext.sc_ar_pfs >> 7) & 0x7f;
	  *val = ia64_rse_skip_regs (uc->uc_mcontext.sc_ar_bsp, -sol);
	  debug (100, "%s: %s -> %lx\n",
		 __FUNCTION__, _U_ia64_regname (reg), *val);
	}
      return 0;

    case UNW_IA64_GR + 4 ... UNW_IA64_GR + 7:
      addr = &uc->uc_mcontext.sc_gr[reg - UNW_IA64_GR];
      break;

    case UNW_IA64_NAT + 4 ... UNW_IA64_NAT + 7:
      mask = ((unw_word_t) 1) << (reg - UNW_IA64_NAT);
      if (write)
	{
	  if (*val)
	    uc->uc_mcontext.sc_nat |= mask;
	  else
	    uc->uc_mcontext.sc_nat &= ~mask;
	}
      else
	*val = (uc->uc_mcontext.sc_nat & mask) != 0;

      if (write)
	debug (100, "%s: %s <- %lx\n", __FUNCTION__, _U_ia64_regname (reg),
	       *val);
      else
	debug (100, "%s: %s -> %lx\n", __FUNCTION__, _U_ia64_regname (reg),
	       *val);
      return 0;

    case UNW_IA64_BR + 1 ... UNW_IA64_BR + 5:
      addr = &uc->uc_mcontext.sc_br[reg - UNW_IA64_BR];
      break;

    default:
      debug (1, "%s: bad register number %u\n", __FUNCTION__, reg);
      /* attempt to access a non-preserved register */
      return -UNW_EBADREG;
    }

  if (write)
    {
      *(unw_word_t *) addr = *val;
      debug (100, "%s: %s <- %lx\n",
	     __FUNCTION__, _U_ia64_regname (reg), *val);
    }
  else
    {
      *val = *(unw_word_t *) addr;
      debug (100, "%s: %s -> %lx\n",
	     __FUNCTION__, _U_ia64_regname (reg), *val);
    }
  return 0;
}

static int
access_fpreg (unw_regnum_t reg, unw_fpreg_t *val, int write, void *arg)
{
  ucontext_t *uc = arg;
  unw_fpreg_t *addr;

  switch (reg)
    {
    case UNW_IA64_FR+ 2 ... UNW_IA64_FR+ 5:
    case UNW_IA64_FR+16 ... UNW_IA64_FR+31:
      addr = (unw_fpreg_t *) &uc->uc_mcontext.sc_fr[reg - UNW_IA64_FR]; break;

    default:
      debug (1, "%s: bad register number %u\n", __FUNCTION__, reg);
      /* attempt to access a non-preserved register */
      return -UNW_EBADREG;
    }

  if (write)
    {
      debug (100, "%s: %s <- %016lx.%016lx\n", __FUNCTION__,
	     _U_ia64_regname (reg), val->raw.bits[1], val->raw.bits[0]);
      *(unw_fpreg_t *) addr = *val;
    }
  else
    {
      *val = *(unw_fpreg_t *) addr;
      debug (100, "%s: %s -> %016lx.%016lx\n", __FUNCTION__,
	     _U_ia64_regname (reg), val->raw.bits[1], val->raw.bits[0]);
    }
  return 0;
}

static int
resume (unw_cursor_t *cursor, void *arg)
{
  struct ia64_cursor *c = (struct ia64_cursor *) cursor;
  unw_fpreg_t fpval;
  ucontext_t *uc = arg;
  unw_word_t val, sol;
  int i, ret;
# define SET_NAT(n, r)					\
  do							\
    {							\
      ret = ia64_get (c, c->r, &val);			\
      if (ret < 0)					\
	return ret;					\
      if (val)						\
	uc->uc_mcontext.sc_nat |= (unw_word_t) 1 << n;	\
    }							\
  while (0)
# define SET_REG(f, r)				\
  do						\
    {						\
      ret = ia64_get (c, c->r, &val);		\
      if (ret < 0)				\
	return ret;				\
      uc->uc_mcontext.f = val;			\
    }						\
  while (0)
# define SET_FPREG(f, r)				\
  do							\
    {							\
      ret = ia64_getfp (c, c->r, &fpval);		\
      if (ret < 0)					\
	return ret;					\
      uc->uc_mcontext.f.u.bits[0] = fpval.raw.bits[0];	\
      uc->uc_mcontext.f.u.bits[1] = fpval.raw.bits[1];	\
    }							\
  while (0)

  SET_REG (sc_ar_pfs, pfs_loc);
  SET_REG (sc_br[0], ip_loc);
  SET_REG (sc_pr, pr_loc);
  SET_REG (sc_ar_rnat, rnat_loc);
  SET_REG (sc_ar_lc, lc_loc);
  SET_REG (sc_ar_fpsr, fpsr_loc);

  SET_REG (sc_gr[4], r4_loc); SET_REG(sc_gr[5], r5_loc);
  SET_REG (sc_gr[6], r6_loc); SET_REG(sc_gr[7], r7_loc);
  uc->uc_mcontext.sc_nat = 0;
  SET_NAT (4, nat4_loc); SET_NAT(5, nat5_loc);
  SET_NAT (6, nat6_loc); SET_NAT(7, nat7_loc);

  SET_REG (sc_br[1], b1_loc);
  SET_REG (sc_br[2], b2_loc);
  SET_REG (sc_br[3], b3_loc);
  SET_REG (sc_br[4], b4_loc);
  SET_REG (sc_br[5], b5_loc);
  SET_FPREG (sc_fr[2], f2_loc);
  SET_FPREG (sc_fr[3], f3_loc);
  SET_FPREG (sc_fr[4], f4_loc);
  SET_FPREG (sc_fr[5], f5_loc);
  for (i = 16; i < 32; ++i)
    SET_FPREG (sc_fr[i], fr_loc[i - 16]);

  if ((c->pi.flags & IA64_FLAG_SIGTRAMP) != 0)
    fprintf (stderr, "%s: fix me!!\n", __FUNCTION__);

  /* Account for the fact that __ia64_install_context() returns via
     br.ret, which will decrement bsp by size-of-locals.  */
  sol = (uc->uc_mcontext.sc_ar_pfs >> 7) & 0x7f;
  uc->uc_mcontext.sc_ar_bsp = ia64_rse_skip_regs (c->bsp, sol);

  uc->uc_mcontext.sc_flags = 0;
  uc->uc_mcontext.sc_gr[1] = c->pi.gp;
  uc->uc_mcontext.sc_gr[12] = c->psp;

  __ia64_install_context (uc, c->eh_args[0], c->eh_args[1], c->eh_args[2],
			  c->eh_args[3]);
}

#endif /* !UNW_LOCAL_ONLY */

int
unw_init_local (unw_cursor_t *cursor, ucontext_t *uc)
{
  STAT(unsigned long start, flags; ++unw.stat.api.inits;
       start = ia64_get_itc ();)
  int ret;

  if (unw.first_time)
    {
      unw.first_time = 0;
      ia64_init ();
    }

#ifdef UNW_LOCAL_ONLY
  {
    struct ia64_cursor *c = (struct ia64_cursor *) cursor;
    unw_word_t bsp, sol;

#   error this needs updating/testing

    c->uc = uc;

    /* What we do here is initialize the unwind cursor so unwinding
       starts at parent of the function that created the ucontext_t.  */

    c->sp = c->psp = uc->uc_mcontext.sc_gr[12];
    c->cfm_loc = &uc->uc_mcontext.sc_ar_pfs;
    bsp = uc->uc_mcontext.sc_ar_bsp;
    sol = (*c->cfm_loc >> 7) & 0x7f;
    c->bsp = ia64_rse_skip_regs (bsp, -sol);
    c->ip = uc->uc_mcontext.sc_br[0];
    c->pr = uc->uc_mcontext.sc_pr;

    ret = ia64_find_save_locs (c);
  }
#else /* !UNW_LOCAL_ONLY */
  {
    unw_accessors_t a;

    a.arg = uc;
    a.acquire_unwind_info = ia64_glibc_acquire_unwind_info;
    a.release_unwind_info = ia64_glibc_release_unwind_info;
    a.access_mem = access_mem;
    a.access_reg = access_reg;
    a.access_fpreg = access_fpreg;
    a.resume = resume;
    ret = unw_init_remote (cursor, &a);
  }
#endif /* !UNW_LOCAL_ONLY */
  STAT(unw.stat.api.init_time += ia64_get_itc() - start;)
  return ret;
}

#endif /* !UNW_REMOTE_ONLY */
