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

#include <stdlib.h>

#include "rse.h"
#include "unwind_i.h"

struct ia64_global_unwind_state unw =
  {
    first_time: 1,
    save_order: {
      IA64_REG_RP, IA64_REG_PFS, IA64_REG_PSP, IA64_REG_PR,
      IA64_REG_UNAT, IA64_REG_LC, IA64_REG_FPSR, IA64_REG_PRI_UNAT_GR
    },
    preg_index: {
      struct_offset (struct ia64_cursor, pri_unat_loc)/8, /* PRI_UNAT_GR */
      struct_offset (struct ia64_cursor, pri_unat_loc)/8, /* PRI_UNAT_MEM */
      struct_offset (struct ia64_cursor, bsp_loc)/8,
      struct_offset (struct ia64_cursor, bspstore_loc)/8,
      struct_offset (struct ia64_cursor, pfs_loc)/8,
      struct_offset (struct ia64_cursor, rnat_loc)/8,
      struct_offset (struct ia64_cursor, psp)/8,
      struct_offset (struct ia64_cursor, ip_loc)/8,
      struct_offset (struct ia64_cursor, r4_loc)/8,
      struct_offset (struct ia64_cursor, r5_loc)/8,
      struct_offset (struct ia64_cursor, r6_loc)/8,
      struct_offset (struct ia64_cursor, r7_loc)/8,
      struct_offset (struct ia64_cursor, nat4_loc)/8,
      struct_offset (struct ia64_cursor, nat5_loc)/8,
      struct_offset (struct ia64_cursor, nat6_loc)/8,
      struct_offset (struct ia64_cursor, nat7_loc)/8,
      struct_offset (struct ia64_cursor, unat_loc)/8,
      struct_offset (struct ia64_cursor, pr_loc)/8,
      struct_offset (struct ia64_cursor, lc_loc)/8,
      struct_offset (struct ia64_cursor, fpsr_loc)/8,
      struct_offset (struct ia64_cursor, b1_loc)/8,
      struct_offset (struct ia64_cursor, b2_loc)/8,
      struct_offset (struct ia64_cursor, b3_loc)/8,
      struct_offset (struct ia64_cursor, b4_loc)/8,
      struct_offset (struct ia64_cursor, b5_loc)/8,
      struct_offset (struct ia64_cursor, f2_loc)/8,
      struct_offset (struct ia64_cursor, f3_loc)/8,
      struct_offset (struct ia64_cursor, f4_loc)/8,
      struct_offset (struct ia64_cursor, f5_loc)/8,
      struct_offset (struct ia64_cursor, fr_loc[16 - 16])/8,
      struct_offset (struct ia64_cursor, fr_loc[17 - 16])/8,
      struct_offset (struct ia64_cursor, fr_loc[18 - 16])/8,
      struct_offset (struct ia64_cursor, fr_loc[19 - 16])/8,
      struct_offset (struct ia64_cursor, fr_loc[20 - 16])/8,
      struct_offset (struct ia64_cursor, fr_loc[21 - 16])/8,
      struct_offset (struct ia64_cursor, fr_loc[22 - 16])/8,
      struct_offset (struct ia64_cursor, fr_loc[23 - 16])/8,
      struct_offset (struct ia64_cursor, fr_loc[24 - 16])/8,
      struct_offset (struct ia64_cursor, fr_loc[25 - 16])/8,
      struct_offset (struct ia64_cursor, fr_loc[26 - 16])/8,
      struct_offset (struct ia64_cursor, fr_loc[27 - 16])/8,
      struct_offset (struct ia64_cursor, fr_loc[28 - 16])/8,
      struct_offset (struct ia64_cursor, fr_loc[29 - 16])/8,
      struct_offset (struct ia64_cursor, fr_loc[30 - 16])/8,
      struct_offset (struct ia64_cursor, fr_loc[31 - 16])/8,
    },
#if IA64_UNW_DEBUG
    debug_level: 0,
    preg_name: {
      "pri_unat_gr", "pri_unat_mem", "bsp", "bspstore", "ar.pfs", "ar.rnat",
      "psp", "rp",
      "r4", "r5", "r6", "r7",
      "ar.unat", "pr", "ar.lc", "ar.fpsr",
      "b1", "b2", "b3", "b4", "b5",
      "f2", "f3", "f4", "f5",
      "f16", "f17", "f18", "f19", "f20", "f21", "f22", "f23",
      "f24", "f25", "f26", "f27", "f28", "f29", "f30", "f31"
    }
#endif
};

unw_addr_space_t unw_local_addr_space;

#ifndef UNW_REMOTE_ONLY

static struct unw_addr_space local_addr_space;

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
_Uia64_uc_addr (ucontext_t *uc, int reg)
{
  return uc_addr (uc, reg);
}

# endif /* UNW_LOCAL_ONLY */

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

#endif /* !UNW_REMOTE_ONLY */

void
ia64_init (void)
{
  extern void unw_hash_index_t_is_too_narrow (void);
  extern void unw_cursor_t_is_too_small (void);
  uint8_t f1_bytes[16] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff,
    0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
  };
  uint8_t nat_val_bytes[16] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0xff, 0xfe,
    0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
  };
  uint8_t int_val_bytes[16] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0xff, 0xfe,
    0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
  };
  uint8_t *lep, *bep;
  long i;

#if IA64_UNW_DEBUG
  {
    const char *str = getenv ("UNW_DEBUG_LEVEL");

    if (str)
      unw.debug_level = atoi (str);
  }
#endif

  mempool_init (&unw.state_record_pool, sizeof (struct ia64_state_record), 0);
  mempool_init (&unw.labeled_state_pool,
		sizeof (struct ia64_labeled_state), 0);

  unw.f0.raw.bits[0] = 0;
  unw.f0.raw.bits[1] = 0;

  lep = (uint8_t *) &unw.f1_le + 16;
  bep = (uint8_t *) &unw.f1_be;
  for (i = 0; i < 16; ++i)
    {
      *--lep = f1_bytes[i];
      *bep++ = f1_bytes[i];
    }

  lep = (uint8_t *) &unw.nat_val_le + 16;
  bep = (uint8_t *) &unw.nat_val_be;
  for (i = 0; i < 16; ++i)
    {
      *--lep = nat_val_bytes[i];
      *bep++ = nat_val_bytes[i];
    }

  lep = (uint8_t *) &unw.int_val_le + 16;
  bep = (uint8_t *) &unw.int_val_be;
  for (i = 0; i < 16; ++i)
    {
      *--lep = int_val_bytes[i];
      *bep++ = int_val_bytes[i];
    }

  if (sizeof (struct ia64_cursor) > sizeof (unw_cursor_t))
    unw_cursor_t_is_too_small();

  if (8*sizeof(unw_hash_index_t) < IA64_LOG_UNW_HASH_SIZE)
    unw_hash_index_t_is_too_narrow();


#ifndef UNW_REMOTE_ONLY
  memset (&local_addr_space, 0, sizeof (local_addr_space));
  ia64_script_cache_init (&local_addr_space.global_cache);
  local_addr_space.caching_policy = UNW_CACHE_GLOBAL;
  local_addr_space.acc.find_proc_info = _Uia64_find_proc_info;
  local_addr_space.acc.access_mem = access_mem;
  local_addr_space.acc.access_reg = access_reg;
  local_addr_space.acc.access_fpreg = access_fpreg;
  local_addr_space.acc.resume = ia64_local_resume;
  unw_local_addr_space = &local_addr_space;
#endif
}
