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
#include <stdlib.h>

#include "init.h"
#include "rse.h"
#include "unwind_i.h"

#ifdef UNW_REMOTE_ONLY

int
unw_init_local (unw_cursor_t *cursor, ucontext_t *uc)
{
  return -UNW_EINVAL;
}

#else /* !UNW_REMOTE_ONLY */

static inline void *
uc_addr (ucontext_t *uc, int reg)
{
  void *addr;

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
    case UNW_IA64_AR_BSP:	addr = &uc->uc_mcontext.sc_rbs_base; break;
    case UNW_IA64_AR_BSPSTORE:	addr = &uc->uc_mcontext.sc_rbs_base; break;

    case UNW_IA64_GR + 4 ... UNW_IA64_GR + 7:
      addr = &uc->uc_mcontext.sc_gr[reg - UNW_IA64_GR];
      break;

    case UNW_IA64_BR + 1 ... UNW_IA64_BR + 5:
      addr = &uc->uc_mcontext.sc_br[reg - UNW_IA64_BR];
      break;

    case UNW_IA64_FR+ 2 ... UNW_IA64_FR+ 5:
    case UNW_IA64_FR+16 ... UNW_IA64_FR+31:
      addr = &uc->uc_mcontext.sc_fr[reg - UNW_IA64_FR];
      break;

    default:
      addr = NULL;
    }
  return addr;
}

#ifdef UNW_LOCAL_ONLY

void *
_U_ia64_uc_addr (ucontext_t *uc, int reg)
{
  return uc_addr (uc, reg);
}

#else /* !UNW_LOCAL_ONLY */

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
  unw_word_t *addr, mask;
  ucontext_t *uc = arg;

  if (reg >= UNW_IA64_FR && reg < UNW_IA64_FR + 128)
    goto badreg;

  if (reg >= UNW_IA64_NAT + 4 && reg <= UNW_IA64_NAT + 7)
    {
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
    }

  addr = uc_addr (uc, reg);
  if (!addr)
    goto badreg;

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

 badreg:
  debug (1, "%s: bad register number %u\n", __FUNCTION__, reg);
  return -UNW_EBADREG;
}

static int
access_fpreg (unw_regnum_t reg, unw_fpreg_t *val, int write, void *arg)
{
  ucontext_t *uc = arg;
  unw_fpreg_t *addr;

  if (reg < UNW_IA64_FR || reg >= UNW_IA64_FR + 128)
    goto badreg;

  addr = uc_addr (uc, reg);
  if (!addr)
    goto badreg;

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

 badreg:
  debug (1, "%s: bad register number %u\n", __FUNCTION__, reg);
  /* attempt to access a non-preserved register */
  return -UNW_EBADREG;
}

#endif /* !UNW_LOCAL_ONLY */

int
ia64_local_resume (unw_cursor_t *cursor, void *arg)
{
  struct ia64_cursor *c = (struct ia64_cursor *) cursor;
  unw_fpreg_t fpval;
  ucontext_t *uc = arg;
  unw_word_t val, sol;
  int i, ret;
# define SET_NAT(n)						\
  do								\
    {								\
      ret = ia64_access_reg (c, UNW_IA64_NAT + (n), &val, 0);	\
      if (ret < 0)						\
	return ret;						\
      if (val)							\
	uc->uc_mcontext.sc_nat |= (unw_word_t) 1 << n;		\
    }								\
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
  SET_NAT (4); SET_NAT(5);
  SET_NAT (6); SET_NAT(7);

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
    abort ();	/* XXX this needs to be fixed... */

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

int
unw_init_local (unw_cursor_t *cursor, ucontext_t *uc)
{
  struct ia64_cursor *c = (struct ia64_cursor *) cursor;
  unw_word_t sol;
  int ret;
  STAT(unsigned long start, flags; ++unw.stat.api.inits;
       start = ia64_get_itc ();)

  if (unw.first_time)
    {
      unw.first_time = 0;
      ia64_init ();
    }

  /* The bsp value stored by getcontext() points to the *end* of the
     register frame of the initial function.  We correct for this by
     storing the adjusted value in sc_rbs_base, which isn't used by
     getcontext()/setcontext().  */
  sol = (uc->uc_mcontext.sc_ar_pfs >> 7) & 0x7f;
  uc->uc_mcontext.sc_rbs_base = ia64_rse_skip_regs (uc->uc_mcontext.sc_ar_bsp,
						    -sol);

#ifdef UNW_LOCAL_ONLY
  c->uc = uc;
#else /* !UNW_LOCAL_ONLY */
  c->acc.arg = uc;
  c->acc.acquire_unwind_info = _U_ia64_glibc_acquire_unwind_info;
  c->acc.release_unwind_info = _U_ia64_glibc_release_unwind_info;
  c->acc.access_mem = access_mem;
  c->acc.access_reg = access_reg;
  c->acc.access_fpreg = access_fpreg;
  c->acc.resume = ia64_local_resume;
#endif /* !UNW_LOCAL_ONLY */
  ret = common_init (c);
  STAT(unw.stat.api.init_time += ia64_get_itc() - start;)
  return ret;
}

#endif /* !UNW_REMOTE_ONLY */
