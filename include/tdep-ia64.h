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

#ifndef TDEP_IA64_H
#define TDEP_IA64_H

/* Target-dependent definitions that are internal to libunwind but need
   to be shared with target-independent code.  */

#include <endian.h>
#include <libunwind.h>

#include "mempool.h"

enum ia64_pregnum
  {
    /* primary unat: */
    IA64_REG_PRI_UNAT_GR,
    IA64_REG_PRI_UNAT_MEM,

    /* register stack */
    IA64_REG_BSP,			/* register stack pointer */
    IA64_REG_BSPSTORE,
    IA64_REG_PFS,			/* previous function state */
    IA64_REG_RNAT,
    /* memory stack */
    IA64_REG_PSP,			/* previous memory stack pointer */
    /* return pointer: */
    IA64_REG_RP,

    /* preserved registers: */
    IA64_REG_R4, IA64_REG_R5, IA64_REG_R6, IA64_REG_R7,
    IA64_REG_NAT4, IA64_REG_NAT5, IA64_REG_NAT6, IA64_REG_NAT7,
    IA64_REG_UNAT, IA64_REG_PR, IA64_REG_LC, IA64_REG_FPSR,
    IA64_REG_B1, IA64_REG_B2, IA64_REG_B3, IA64_REG_B4, IA64_REG_B5,
    IA64_REG_F2, IA64_REG_F3, IA64_REG_F4, IA64_REG_F5,
    IA64_REG_F16, IA64_REG_F17, IA64_REG_F18, IA64_REG_F19,
    IA64_REG_F20, IA64_REG_F21, IA64_REG_F22, IA64_REG_F23,
    IA64_REG_F24, IA64_REG_F25, IA64_REG_F26, IA64_REG_F27,
    IA64_REG_F28, IA64_REG_F29, IA64_REG_F30, IA64_REG_F31,
    IA64_NUM_PREGS
  };

#include "ia64/script.h"

struct unw_addr_space
  {
    struct unw_accessors acc;
    int big_endian;
    unw_caching_policy_t caching_policy;
    uint32_t cache_generation;
    unw_word_t dyn_generation;	/* see dyn-common.h */

    struct ia64_script_cache global_cache;
   };

struct cursor
  {
    void *as_arg;		/* argument to address-space callbacks */
    unw_addr_space_t as;	/* reference to per-address-space info */

    /* IP and predicate cache (these are always equal to the values
       stored in ip_loc and pr_loc, respectively).  */
    unw_word_t ip;		/* instruction pointer value */
    unw_word_t pr;		/* current predicate values */

    /* current frame info: */
    unw_word_t bsp;		/* backing store pointer value */
    unw_word_t sp;		/* stack pointer value */
    unw_word_t psp;		/* previous sp value */
    unw_word_t cfm_loc;		/* cfm save location (or NULL) */
    unw_word_t rbs_top;		/* address of end of register backing store */
    unw_word_t top_rnat_loc;	/* location of final (topmost) RNaT word */

    /* preserved state: */
    unw_word_t bsp_loc;		/* previous bsp save location */
    unw_word_t bspstore_loc;
    unw_word_t pfs_loc;
    unw_word_t rnat_loc;
    unw_word_t ip_loc;
    unw_word_t pri_unat_loc;
    unw_word_t unat_loc;
    unw_word_t pr_loc;
    unw_word_t lc_loc;
    unw_word_t fpsr_loc;
    unw_word_t r4_loc, r5_loc, r6_loc, r7_loc;
    unw_word_t nat4_loc, nat5_loc, nat6_loc, nat7_loc;
    unw_word_t b1_loc, b2_loc, b3_loc, b4_loc, b5_loc;
    unw_word_t f2_loc, f3_loc, f4_loc, f5_loc, fr_loc[16];

    unw_word_t eh_args[4];	/* exception handler arguments */
    unw_word_t sigcontext_loc;	/* location of sigcontext or NULL */
    unw_word_t is_signal_frame;	/* is this a signal trampoline frame? */

    short hint;
    short prev_script;

    int pi_valid : 1;		/* is proc_info valid? */
    int pi_is_dynamic : 1;	/* proc_info found via dynamic proc info? */
    unw_proc_info_t pi;		/* info about current procedure */
};

struct ia64_global_unwind_state
  {
    int needs_initialization;

    /* Table of registers that prologues can save (and order in which
       they're saved).  */
    const unsigned char save_order[8];

    /* Maps a preserved register index (preg_index) to corresponding
       ucontext_t offset.  */
    unsigned short uc_off[sizeof(unw_cursor_t) / 8];

    /* Index into unw_cursor_t for preserved register i */
    unsigned short preg_index[IA64_NUM_PREGS];

    unw_fpreg_t f0, f1_le, f1_be, nat_val_le;
    unw_fpreg_t nat_val_be, int_val_le, int_val_be;

    struct mempool state_record_pool;
    struct mempool labeled_state_pool;

# if UNW_DEBUG
    long debug_level;
    const char *preg_name[IA64_NUM_PREGS];
# endif
  };

/* Platforms that support UNW_INFO_FORMAT_TABLE need to define
   tdep_search_unwind_table.  */
#define tdep_search_unwind_table(a,b,c,d,e,f)			\
		UNW_ARCH_OBJ(search_unwind_table) (a,b,c,d,e,f)
#define tdep_find_proc_info(as,ip,pi,n,a)			\
		UNW_ARCH_OBJ(find_proc_info) (as,ip,pi,n,a)
#define tdep_put_unwind_info(a,b,c) 	UNW_ARCH_OBJ(put_unwind_info)(a,b,c)
#define tdep_uc_addr(uc,reg)		UNW_ARCH_OBJ(uc_addr)(uc,reg)
#define tdep_debug_level		unw.debug_level

#define unw		UNW_ARCH_OBJ(data)

extern int tdep_search_unwind_table (unw_addr_space_t as, unw_word_t ip,
				     unw_dyn_info_t *di, unw_proc_info_t *pi,
				     int need_unwind_info, void *arg);
extern int tdep_find_proc_info (unw_addr_space_t as, unw_word_t ip,
				unw_proc_info_t *pi, int need_unwind_info,
				void *arg);
extern void tdep_put_unwind_info (unw_addr_space_t as,
				  unw_proc_info_t *pi, void *arg);
extern void *tdep_uc_addr (ucontext_t *uc, unw_regnum_t regnum);

extern struct ia64_global_unwind_state unw;

#endif /* TDEP_IA64_H */
