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

#ifndef TDEP_IA64_H
#define TDEP_IA64_H

/* Target-dependent definitions that are internal to libunwind but need
   to be shared with target-independent code.  */

#include <libunwind.h>

#include "elf64.h"
#include "mempool.h"

enum ia64_pregnum
  {
    /* primary unat: */
    IA64_REG_PRI_UNAT_GR,
    IA64_REG_PRI_UNAT_MEM,

    /* memory stack (order matters: see build_script() */
    IA64_REG_PSP,			/* previous memory stack pointer */
    /* register stack */
    IA64_REG_BSP,			/* register stack pointer */
    IA64_REG_BSPSTORE,
    IA64_REG_PFS,			/* previous function state */
    IA64_REG_RNAT,
    /* instruction pointer: */
    IA64_REG_IP,

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

#ifdef UNW_LOCAL_ONLY

typedef unw_word_t ia64_loc_t;

#else /* !UNW_LOCAL_ONLY */

typedef struct ia64_loc
  {
    unw_word_t w0, w1;
  }
ia64_loc_t;

#endif /* !UNW_LOCAL_ONLY */

#include "ia64/script.h"

#define ABI_UNKNOWN			0
#define ABI_LINUX			1
#define ABI_HPUX			2
#define ABI_FREEBSD			3
#define ABI_OPENVMS			4
#define ABI_NSK				5	/* Tandem/HP Non-Stop Kernel */
#define ABI_WINDOWS			6

struct unw_addr_space
  {
    struct unw_accessors acc;
    int big_endian;
    int abi;	/* abi < 0 => unknown, 0 => SysV, 1 => HP-UX, 2 => Windows */
    unw_caching_policy_t caching_policy;
    uint32_t cache_generation;
    unw_word_t dyn_generation;		/* see dyn-common.h */
    unw_word_t dyn_info_list_addr;	/* (cached) dyn_info_list_addr */

    struct ia64_script_cache global_cache;
   };

/* Note: The ABI numbers in the ABI-markers (.unwabi directive) are
   not the same as the above ABI numbers (which are more
   fine-grained).  */
#define ABI_MARKER_LINUX_SIGTRAMP	((0 << 8) | 's')
#define ABI_MARKER_LINUX_INTERRUPT	((0 << 8) | 'i')
#define ABI_MARKER_HP_UX_SIGTRAMP	((1 << 8) | 1)

struct cursor
  {
    void *as_arg;		/* argument to address-space callbacks */
    unw_addr_space_t as;	/* reference to per-address-space info */

    /* IP, CFM, and predicate cache (these are always equal to the
       values stored in ip_loc, cfm_loc, and pr_loc,
       respectively).  */
    unw_word_t ip;		/* instruction pointer value */
    unw_word_t cfm;		/* current frame mask */
    unw_word_t pr;		/* current predicate values */

    /* current frame info: */
    unw_word_t bsp;		/* backing store pointer value */
    unw_word_t sp;		/* stack pointer value */
    unw_word_t psp;		/* previous sp value */
    ia64_loc_t cfm_loc;		/* cfm save location (or NULL) */
    ia64_loc_t loc[IA64_NUM_PREGS];

    unw_word_t eh_args[4];	/* exception handler arguments */
    unw_word_t sigcontext_addr;	/* address of sigcontext or 0 */
    unw_word_t sigcontext_off;	/* sigcontext-offset relative to signal sp */

    short hint;
    short prev_script;

    uint16_t abi_marker;	/* abi_marker for current frame (if any) */
    uint16_t last_abi_marker;	/* last abi_marker encountered so far */
    uint8_t eh_valid_mask;

    int pi_valid : 1;		/* is proc_info valid? */
    int pi_is_dynamic : 1;	/* proc_info found via dynamic proc info? */
    unw_proc_info_t pi;		/* info about current procedure */

    /* In case of stack-discontiguities, such as those introduced by
       signal-delivery on an alternate signal-stack (see
       sigaltstack(2)), we use the following data-structure to keep
       track of the register-backing-store areas across on which the
       current frame may be backed up.  Since there are at most 96
       stacked registers and since we only have to track the current
       frame and only areas that are not empty, this puts an upper
       limit on the # of backing-store areas we have to track.  */
    uint8_t rbs_curr;		/* index of curr. rbs-area (contains c->bsp) */
    uint8_t rbs_left_edge;	/* index of inner-most valid rbs-area */
    struct rbs_area
      {
	unw_word_t end;
	unw_word_t size;
	ia64_loc_t rnat_loc;
      }
    rbs_area[96 + 2];	/* 96 stacked regs + 1 extra stack on each side... */
};

struct ia64_global_unwind_state
  {
    int needs_initialization;

    /* Table of registers that prologues can save (and order in which
       they're saved).  */
    const unsigned char save_order[8];

    unw_word_t r0;
    unw_fpreg_t f0, f1_le, f1_be, nat_val_le;
    unw_fpreg_t nat_val_be, int_val_le, int_val_be;

    struct mempool reg_state_pool;
    struct mempool labeled_state_pool;

# if UNW_DEBUG
    long debug_level;
    const char *preg_name[IA64_NUM_PREGS];
# endif
  };

/* Platforms that support UNW_INFO_FORMAT_TABLE need to define
   tdep_search_unwind_table.  */
#define tdep_search_unwind_table(a,b,c,d,e,f)			\
		_Uia64_search_unwind_table (a, b, c, d, e, f)
#define tdep_find_proc_info(as,ip,pi,n,a)			\
		UNW_ARCH_OBJ(find_proc_info) (as, ip, pi, n, a)
#define tdep_put_unwind_info(a,b,c) 	UNW_ARCH_OBJ(put_unwind_info)(a, b, c)
#define tdep_uc_addr(uc,reg)		UNW_ARCH_OBJ(uc_addr)(uc, reg)
#define tdep_get_elf_image(a,b,c,d,e)	UNW_ARCH_OBJ(get_elf_image) (a, b, c, \
								     d, e)
#define tdep_debug_level		unw.debug_level

#define unw		UNW_ARCH_OBJ(data)

extern int tdep_find_proc_info (unw_addr_space_t as, unw_word_t ip,
				unw_proc_info_t *pi, int need_unwind_info,
				void *arg);
extern void tdep_put_unwind_info (unw_addr_space_t as,
				  unw_proc_info_t *pi, void *arg);
extern void *tdep_uc_addr (ucontext_t *uc, unw_regnum_t regnum);
extern int tdep_get_elf_image (struct elf_image *ei, pid_t pid, unw_word_t ip,
			       unsigned long *segbase, unsigned long *mapoff);

extern struct ia64_global_unwind_state unw;

#endif /* TDEP_IA64_H */
