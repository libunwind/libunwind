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

#include "unwind_i.h"

struct ia64_global_unwind_state unw =
  {
    first_time: 1,
//    tables: &unw.kernel_table,
//    lock: SPIN_LOCK_UNLOCKED,
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
    hash : { [0 ... IA64_UNW_HASH_SIZE - 1] = -1 },
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

  for (i = 0; i < IA64_UNW_CACHE_SIZE; ++i) {
    if (i > 0)
      unw.cache[i].lru_chain = (i - 1);
    unw.cache[i].coll_chain = -1;
//    unw.cache[i].lock = RW_LOCK_UNLOCKED;
  }
  unw.lru_head = IA64_UNW_CACHE_SIZE - 1;
  unw.lru_tail = 0;
}
