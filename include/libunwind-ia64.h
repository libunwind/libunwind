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

#ifndef LIBUNWIND_H
#define LIBUNWIND_H

#include <stdint.h>
#include <ucontext.h>

#define UNW_TARGET	ia64

typedef uint64_t unw_tdep_word_t;

typedef enum
  {
    /* Note: general registers are excepted to start with index 0.
       This convention facilitates architecture-independent
       implementation of the C++ exception handling ABI.  See
       _Unwind_SetGR() and _Unwind_GetGR() for details.  */
    UNW_IA64_GR = 0,			/* general registers (r0..r127) */
     UNW_IA64_GP = UNW_IA64_GR + 1,
     UNW_IA64_TP = UNW_IA64_GR + 13,

    UNW_IA64_NAT = UNW_IA64_GR + 128,	/* NaT registers (nat0..nat127) */

    UNW_IA64_FR = UNW_IA64_NAT + 128,	/* fp registers (f0..f127) */

    UNW_IA64_AR = UNW_IA64_FR + 128,	/* application registers (ar0..r127) */
     UNW_IA64_AR_RSC = UNW_IA64_AR + 16,
     UNW_IA64_AR_BSP = UNW_IA64_AR + 17,
     UNW_IA64_AR_BSPSTORE = UNW_IA64_AR + 18,
     UNW_IA64_AR_RNAT = UNW_IA64_AR + 19,
     UNW_IA64_AR_25 = UNW_IA64_AR + 25,	/* reserved (scratch) */
     UNW_IA64_AR_26 = UNW_IA64_AR + 26,	/* reserved (scratch) */
     UNW_IA64_AR_CCV = UNW_IA64_AR + 32,
     UNW_IA64_AR_UNAT = UNW_IA64_AR + 36,
     UNW_IA64_AR_FPSR = UNW_IA64_AR + 40,
     UNW_IA64_AR_PFS = UNW_IA64_AR + 64,
     UNW_IA64_AR_LC = UNW_IA64_AR + 65,
     UNW_IA64_AR_EC = UNW_IA64_AR + 66,

    UNW_IA64_BR = UNW_IA64_AR + 128,	/* branch registers (b0..p7) */
    UNW_IA64_PR = UNW_IA64_BR + 8,	/* predicate registers (p0..p63) */
    UNW_IA64_CFM,

    /* frame info (read-only): */
    UNW_IA64_BSP,
    UNW_IA64_IP,
    UNW_IA64_SP,

    UNW_TDEP_LAST_REG = UNW_IA64_SP,

    UNW_TDEP_IP = UNW_IA64_IP,
    UNW_TDEP_SP = UNW_IA64_SP
  }
ia64_regnum_t;

typedef struct unw_tdep_save_loc
  {
    /* Additional target-dependent info on a save location.  On IA-64,
       we could use this to specify the bit number in which a NaT bit
       gets saved.  For now, nobody wants to know this, so it's not
       currently implemented.  */
  }
unw_tdep_save_loc_t;

/* On IA-64, we can directly use ucontext_t as the unwind context.  */
typedef ucontext_t unw_tdep_context_t;

/* XXX this is not ideal: an application should not be prevented from
   using the "getcontext" name just because it's using libunwind.  We
   can't just use __getcontext() either, because that isn't exported
   by glibc...  */
#define unw_tdep_getcontext(uc)		(getcontext (uc), 0)

#define unw_tdep_is_fpreg(r)		((unsigned) ((r) - UNW_IA64_FR) < 128)

#include "libunwind-common.h"

/* This is a helper routine which the get_dyn_info_list_addr()
   callback can use to locate the special dynamic-info list entry in
   an IA-64 unwind table.  If the entry exists in the table, the
   list-address is returned.  In all other cases, 0 is returned.  */
unw_word_t _Uia64_find_dyn_list (unw_addr_space_t as,
				 void *table, size_t table_size,
				 unw_word_t segbase, void *arg);

#endif /* LIBUNWIND_H */
