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

extern int UNW_ARCH_OBJ (search_unwind_table) (unw_addr_space_t as,
					       unw_word_t ip,
					       unw_dyn_info_t *di,
					       unw_proc_info_t *pi,
					       int need_unwind_info,
					       void *arg);
extern void UNW_ARCH_OBJ (put_unwind_info) (unw_addr_space_t as,
					    unw_proc_info_t *pi, void *arg);

/* Platforms that support UNW_INFO_FORMAT_TABLE need to define
   tdep_search_unwind_table.  */
#define tdep_search_unwind_table(a,b,c,d,e,f)	\
		UNW_ARCH_OBJ(search_unwind_table) (a,b,c,d,e,f)
#define tdep_put_unwind_info(a,b,c) \
		UNW_ARCH_OBJ(put_unwind_info)(a,b,c)

#endif /* TDEP_IA64_H */
