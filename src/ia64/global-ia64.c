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

#include "unwind_i.h"

struct ia64_global_unwind_state unw =
  {
    needs_initialization: 1,
    save_order: {
      IA64_REG_RP, IA64_REG_PFS, IA64_REG_PSP, IA64_REG_PR,
      IA64_REG_UNAT, IA64_REG_LC, IA64_REG_FPSR, IA64_REG_PRI_UNAT_GR
    },
    preg_index: {
      struct_offset (struct cursor, pri_unat_loc)/8, /* PRI_UNAT_GR */
      struct_offset (struct cursor, pri_unat_loc)/8, /* PRI_UNAT_MEM */
      struct_offset (struct cursor, bsp_loc)/8,
      struct_offset (struct cursor, bspstore_loc)/8,
      struct_offset (struct cursor, pfs_loc)/8,
      struct_offset (struct cursor, rnat_loc)/8,
      struct_offset (struct cursor, psp)/8,
      struct_offset (struct cursor, ip_loc)/8,
      struct_offset (struct cursor, r4_loc)/8,
      struct_offset (struct cursor, r5_loc)/8,
      struct_offset (struct cursor, r6_loc)/8,
      struct_offset (struct cursor, r7_loc)/8,
      struct_offset (struct cursor, nat4_loc)/8,
      struct_offset (struct cursor, nat5_loc)/8,
      struct_offset (struct cursor, nat6_loc)/8,
      struct_offset (struct cursor, nat7_loc)/8,
      struct_offset (struct cursor, unat_loc)/8,
      struct_offset (struct cursor, pr_loc)/8,
      struct_offset (struct cursor, lc_loc)/8,
      struct_offset (struct cursor, fpsr_loc)/8,
      struct_offset (struct cursor, b1_loc)/8,
      struct_offset (struct cursor, b2_loc)/8,
      struct_offset (struct cursor, b3_loc)/8,
      struct_offset (struct cursor, b4_loc)/8,
      struct_offset (struct cursor, b5_loc)/8,
      struct_offset (struct cursor, f2_loc)/8,
      struct_offset (struct cursor, f3_loc)/8,
      struct_offset (struct cursor, f4_loc)/8,
      struct_offset (struct cursor, f5_loc)/8,
      struct_offset (struct cursor, fr_loc[16 - 16])/8,
      struct_offset (struct cursor, fr_loc[17 - 16])/8,
      struct_offset (struct cursor, fr_loc[18 - 16])/8,
      struct_offset (struct cursor, fr_loc[19 - 16])/8,
      struct_offset (struct cursor, fr_loc[20 - 16])/8,
      struct_offset (struct cursor, fr_loc[21 - 16])/8,
      struct_offset (struct cursor, fr_loc[22 - 16])/8,
      struct_offset (struct cursor, fr_loc[23 - 16])/8,
      struct_offset (struct cursor, fr_loc[24 - 16])/8,
      struct_offset (struct cursor, fr_loc[25 - 16])/8,
      struct_offset (struct cursor, fr_loc[26 - 16])/8,
      struct_offset (struct cursor, fr_loc[27 - 16])/8,
      struct_offset (struct cursor, fr_loc[28 - 16])/8,
      struct_offset (struct cursor, fr_loc[29 - 16])/8,
      struct_offset (struct cursor, fr_loc[30 - 16])/8,
      struct_offset (struct cursor, fr_loc[31 - 16])/8,
    },
#if UNW_DEBUG
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

#ifndef UNW_REMOTE_ONLY

/* Note: It might be tempting to use the LSDA to store
	 _U_dyn_info_list, but that wouldn't work because the
	 unwind-info is generally mapped read-only.  */

unw_dyn_info_list_t _U_dyn_info_list;

/* Now create a special unwind-table entry which makes it easy for an
   unwinder to locate the dynamic registration list.  The special
   entry covers address range [0-0) and is therefore guaranteed to
   be the first in the unwind-table.  */
asm (
"	.section \".IA_64.unwind_info\", \"a\"\n"
".info:	data8 (1<<48) | 1\n"	/* v1, length==1 (8-byte word) */
"	data8 0\n"		/* 8 empty .prologue directives (nops) */
"	data8 0\n"		/* personality routine (ignored) */
"	string \"dyn-list\"\n"	/* lsda */
"	data8 @segrel(_U_dyn_info_list)\n"
"	.previous\n"

"	.section \".IA_64.unwind\", \"a\"\n"
"	data8 0, 0, @segrel(.info)\n"
"	.previous\n");

#endif

HIDDEN void
ia64_init (void)
{
  extern void unw_hash_index_t_is_too_narrow (void);
  extern void _ULia64_local_addr_space_init (void);
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

  mi_init ();

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

  if (8*sizeof(unw_hash_index_t) < IA64_LOG_UNW_HASH_SIZE)
    unw_hash_index_t_is_too_narrow ();

  _Uia64_local_addr_space_init ();
  _ULia64_local_addr_space_init ();
}
