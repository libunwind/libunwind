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

#ifdef IA64_UNW_DEBUG

static const char *regname[] = {
  "<lsda>",
  "<handler>",
  "<first-ip>",
  "<fp>",
  "<sp>",
  "<rp>",
  "<ip>",
    "r0",   "gp",   "r2",   "r3",   "r4",   "r5",   "r6",   "r7",
    "r8",   "r9",  "r10",  "r11",   "sp",   "tp",  "r14",  "r15",
   "r16",  "r17",  "r18",  "r19",  "r20",  "r21",  "r22",  "r23",
   "r24",  "r25",  "r26",  "r27",  "r28",  "r29",  "r30",  "r31",
   "r32",  "r33",  "r34",  "r35",  "r36",  "r37",  "r38",  "r39",
   "r40",  "r41",  "r42",  "r43",  "r44",  "r45",  "r46",  "r47",
   "r48",  "r49",  "r50",  "r51",  "r52",  "r53",  "r54",  "r55",
   "r56",  "r57",  "r58",  "r59",  "r60",  "r61",  "r62",  "r63",
   "r64",  "r65",  "r66",  "r67",  "r68",  "r69",  "r70",  "r71",
   "r72",  "r73",  "r74",  "r75",  "r76",  "r77",  "r78",  "r79",
   "r80",  "r81",  "r82",  "r83",  "r84",  "r85",  "r86",  "r87",
   "r88",  "r89",  "r90",  "r91",  "r92",  "r93",  "r94",  "r95",
   "r96",  "r97",  "r98",  "r99", "r100", "r101", "r102", "r103",
  "r104", "r105", "r106", "r107", "r108", "r109", "r110", "r111",
  "r112", "r113", "r114", "r115", "r116", "r117", "r118", "r119",
  "r120", "r121", "r122", "r123", "r124", "r125", "r126", "r127",
  "nat0",  "nat1",   "nat2",   "nat3",   "nat4",   "nat5",   "nat6",   "nat7",
  "nat8",  "nat9",  "nat10",  "nat11",  "nat12",  "nat13",  "nat14",  "nat15",
  "nat16", "nat17",  "nat18",  "nat19",  "nat20",  "nat21",  "nat22",  "nat23",
  "nat24", "nat25",  "nat26",  "nat27",  "nat28",  "nat29",  "nat30",  "nat31",
  "nat32", "nat33",  "nat34",  "nat35",  "nat36",  "nat37",  "nat38",  "nat39",
  "nat40", "nat41",  "nat42",  "nat43",  "nat44",  "nat45",  "nat46",  "nat47",
  "nat48", "nat49",  "nat50",  "nat51",  "nat52",  "nat53",  "nat54",  "nat55",
  "nat56", "nat57",  "nat58",  "nat59",  "nat60",  "nat61",  "nat62",  "nat63",
  "nat64", "nat65",  "nat66",  "nat67",  "nat68",  "nat69",  "nat70",  "nat71",
  "nat72", "nat73",  "nat74",  "nat75",  "nat76",  "nat77",  "nat78",  "nat79",
  "nat80", "nat81",  "nat82",  "nat83",  "nat84",  "nat85",  "nat86",  "nat87",
  "nat88", "nat89",  "nat90",  "nat91",  "nat92",  "nat93",  "nat94",  "nat95",
  "nat96", "nat97",  "nat98",  "nat99", "nat100", "nat101", "nat102", "nat103",
  "nat104","nat105","nat106", "nat107", "nat108", "nat109", "nat110", "nat111",
  "nat112","nat113","nat114", "nat115", "nat116", "nat117", "nat118", "nat119",
  "nat120","nat121","nat122", "nat123", "nat124", "nat125", "nat126", "nat127",
    "f0",   "f1",   "f2",   "f3",   "f4",   "f5",   "f6",   "f7",
    "f8",   "f9",  "f10",  "f11",  "f12",  "f13",  "f14",  "f15",
   "f16",  "f17",  "f18",  "f19",  "f20",  "f21",  "f22",  "f23",
   "f24",  "f25",  "f26",  "f27",  "f28",  "f29",  "f30",  "f31",
   "f32",  "f33",  "f34",  "f35",  "f36",  "f37",  "f38",  "f39",
   "f40",  "f41",  "f42",  "f43",  "f44",  "f45",  "f46",  "f47",
   "f48",  "f49",  "f50",  "f51",  "f52",  "f53",  "f54",  "f55",
   "f56",  "f57",  "f58",  "f59",  "f60",  "f61",  "f62",  "f63",
   "f64",  "f65",  "f66",  "f67",  "f68",  "f69",  "f70",  "f71",
   "f72",  "f73",  "f74",  "f75",  "f76",  "f77",  "f78",  "f79",
   "f80",  "f81",  "f82",  "f83",  "f84",  "f85",  "f86",  "f87",
   "f88",  "f89",  "f90",  "f91",  "f92",  "f93",  "f94",  "f95",
   "f96",  "f97",  "f98",  "f99", "f100", "f101", "f102", "f103",
  "f104", "f105", "f106", "f107", "f108", "f109", "f110", "f111",
  "f112", "f113", "f114", "f115", "f116", "f117", "f118", "f119",
  "f120", "f121", "f122", "f123", "f124", "f125", "f126", "f127",
   "ar0",   "ar1",   "ar2",   "ar3",   "ar4",   "ar5",   "ar6",   "ar7",
   "ar8",   "ar9",  "ar10",  "ar11",  "ar12",  "ar13",  "ar14",  "ar15",
   "rsc",  "bsp", "bspstore", "rnat", "ar20",  "ar21",  "ar22",  "ar23",
  "ar24",  "ar25",  "ar26",  "ar27",  "ar28",  "ar29",  "ar30",  "ar31",
   "ccv",  "ar33",  "ar34",  "ar35",  "unat",  "ar37",  "ar38",  "ar39",
  "fpsr",  "ar41",  "ar42",  "ar43",  "ar44",  "ar45",  "ar46",  "ar47",
  "ar48",  "ar49",  "ar50",  "ar51",  "ar52",  "ar53",  "ar54",  "ar55",
  "ar56",  "ar57",  "ar58",  "ar59",  "ar60",  "ar61",  "ar62",  "ar63",
   "pfs",    "lc",    "ec",  "ar67",  "ar68",  "ar69",  "ar70",  "ar71",
  "ar72",  "ar73",  "ar74",  "ar75",  "ar76",  "ar77",  "ar78",  "ar79",
  "ar80",  "ar81",  "ar82",  "ar83",  "ar84",  "ar85",  "ar86",  "ar87",
  "ar88",  "ar89",  "ar90",  "ar91",  "ar92",  "ar93",  "ar94",  "ar95",
  "ar96",  "ar97",  "ar98",  "ar99", "ar100", "ar101", "ar102", "ar103",
 "ar104", "ar105", "ar106", "ar107", "ar108", "ar109", "ar110", "ar111",
 "ar112", "ar113", "ar114", "ar115", "ar116", "ar117", "ar118", "ar119",
 "ar120", "ar121", "ar122", "ar123", "ar124", "ar125", "ar126", "ar127",
    "rp",   "b1",   "b2",   "b3",   "b4",   "b5",   "b6",   "b7",
    "pr",
   "cfm",
  "current-bsp"
};

const char *
_U_ia64_regname (unw_regnum_t reg)
{
  if ((unsigned) (reg + 7) < sizeof (regname) / sizeof (regname[0]))
    return regname[reg + 7];
  else
    return "???";
}

#endif /* IA64_UNW_DEBUG */

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
