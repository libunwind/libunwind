/* libunwind - a platform-independent unwind library
   Copyright (C) 2001-2002 Hewlett-Packard Co
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

#include <endian.h>
#include <stdlib.h>

#include "rse.h"
#include "unwind_i.h"

#ifdef UNW_REMOTE_ONLY

/* unw_local_addr_space is a NULL pointer in this case.  */
unw_addr_space_t unw_local_addr_space;

#else

static struct unw_addr_space local_addr_space;

unw_addr_space_t unw_local_addr_space = &local_addr_space;

static inline void *
uc_addr (ucontext_t *uc, int reg)
{
  void *addr;

  switch (reg)
    {
    case UNW_IA64_IP:		addr = &uc->uc_mcontext.sc_br[0]; break;
    case UNW_IA64_CFM:		addr = &uc->uc_mcontext.sc_ar_pfs; break;
    case UNW_IA64_AR_RNAT:	addr = &uc->uc_mcontext.sc_ar_rnat; break;
    case UNW_IA64_AR_UNAT:	addr = &uc->uc_mcontext.sc_ar_unat; break;
    case UNW_IA64_AR_LC:	addr = &uc->uc_mcontext.sc_ar_lc; break;
    case UNW_IA64_AR_FPSR:	addr = &uc->uc_mcontext.sc_ar_fpsr; break;
    case UNW_IA64_PR:		addr = &uc->uc_mcontext.sc_pr; break;
    case UNW_IA64_AR_BSP:	addr = &uc->uc_mcontext.sc_rbs_base; break;
    case UNW_IA64_AR_BSPSTORE:	addr = &uc->uc_mcontext.sc_rbs_base; break;

    case UNW_IA64_GR + 4 ... UNW_IA64_GR + 7:
    case UNW_IA64_GR + 12:
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

# ifdef UNW_LOCAL_ONLY

void *
tdep_uc_addr (ucontext_t *uc, int reg)
{
  return uc_addr (uc, reg);
}

# endif /* UNW_LOCAL_ONLY */

static void
put_unwind_info (unw_addr_space_t as, unw_proc_info_t *proc_info, void *arg)
{
  /* it's a no-op */
}

static int
get_dyn_info_list_addr (unw_addr_space_t as, unw_word_t *dyn_info_list_addr,
			void *arg)
{
  *dyn_info_list_addr = (unw_word_t) &_U_dyn_info_list;
  return 0;
}

static int
access_mem (unw_addr_space_t as, unw_word_t addr, unw_word_t *val, int write,
	    void *arg)
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
access_reg (unw_addr_space_t as, unw_regnum_t reg, unw_word_t *val, int write,
	    void *arg)
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
	debug (100, "%s: %s <- %lx\n", __FUNCTION__, unw_regname (reg), *val);
      else
	debug (100, "%s: %s -> %lx\n", __FUNCTION__, unw_regname (reg), *val);
      return 0;
    }

  addr = uc_addr (uc, reg);
  if (!addr)
    goto badreg;

  if (write)
    {
      *(unw_word_t *) addr = *val;
      debug (100, "%s: %s <- %lx\n", __FUNCTION__, unw_regname (reg), *val);
    }
  else
    {
      *val = *(unw_word_t *) addr;
      debug (100, "%s: %s -> %lx\n", __FUNCTION__, unw_regname (reg), *val);
    }
  return 0;

 badreg:
  debug (1, "%s: bad register number %u\n", __FUNCTION__, reg);
  return -UNW_EBADREG;
}

static int
access_fpreg (unw_addr_space_t as, unw_regnum_t reg, unw_fpreg_t *val,
	      int write, void *arg)
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
	     unw_regname (reg), val->raw.bits[1], val->raw.bits[0]);
      *(unw_fpreg_t *) addr = *val;
    }
  else
    {
      *val = *(unw_fpreg_t *) addr;
      debug (100, "%s: %s -> %016lx.%016lx\n", __FUNCTION__,
	     unw_regname (reg), val->raw.bits[1], val->raw.bits[0]);
    }
  return 0;

 badreg:
  debug (1, "%s: bad register number %u\n", __FUNCTION__, reg);
  /* attempt to access a non-preserved register */
  return -UNW_EBADREG;
}

HIDDEN void
ia64_local_addr_space_init (void)
{
  memset (&local_addr_space, 0, sizeof (local_addr_space));
  ia64_script_cache_init (&local_addr_space.global_cache);
  local_addr_space.big_endian = (__BYTE_ORDER == __BIG_ENDIAN);
  local_addr_space.caching_policy = UNW_CACHE_GLOBAL;
  local_addr_space.acc.find_proc_info = UNW_ARCH_OBJ (find_proc_info);
  local_addr_space.acc.put_unwind_info = put_unwind_info;
  local_addr_space.acc.get_dyn_info_list_addr = get_dyn_info_list_addr;
  local_addr_space.acc.access_mem = access_mem;
  local_addr_space.acc.access_reg = access_reg;
  local_addr_space.acc.access_fpreg = access_fpreg;
  local_addr_space.acc.resume = ia64_local_resume;
}

#endif /* !UNW_REMOTE_ONLY */
