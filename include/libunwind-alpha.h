/* libunwind - a platform-independent unwind library
   Copyright (C) 2002-2004 Hewlett-Packard Co
        Contributed by David Mosberger-Tang <davidm@hpl.hp.com>

   Modified for Alpha by Matt Turner <mattst88@gmail.com>

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

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

#include <sys/types.h>
#include <inttypes.h>
#include <ucontext.h>

#ifndef UNW_EMPTY_STRUCT
#  define UNW_EMPTY_STRUCT uint8_t unused;
#endif

#define UNW_TARGET              alpha
#define UNW_TARGET_ALPHA        1

#define _U_TDEP_QP_TRUE 0       /* see libunwind-dynamic.h  */

/* This needs to be big enough to accommodate "struct cursor", while
   leaving some slack for future expansion.  Changing this value will
   require recompiling all users of this library.  Stack allocation is
   relatively cheap and unwind-state copying is relatively rare, so we
   want to err on making it rather too big than too small.  */
#define UNW_TDEP_CURSOR_LEN     384

typedef uint64_t unw_word_t;
typedef int64_t unw_sword_t;

typedef double unw_tdep_fpreg_t;

#define UNW_WORD_MAX UINT64_MAX

typedef enum
  {
    /* Integer registers: DWARF numbers 0-31 */
    UNW_ALPHA_R0,       /*  v0: return value */
    UNW_ALPHA_R1,       /*  t0 */
    UNW_ALPHA_R2,       /*  t1 */
    UNW_ALPHA_R3,       /*  t2 */
    UNW_ALPHA_R4,       /*  t3 */
    UNW_ALPHA_R5,       /*  t4 */
    UNW_ALPHA_R6,       /*  t5 */
    UNW_ALPHA_R7,       /*  t6 */
    UNW_ALPHA_R8,       /*  t7 */
    UNW_ALPHA_R9,       /*  s0: callee-saved */
    UNW_ALPHA_R10,      /*  s1 */
    UNW_ALPHA_R11,      /*  s2 */
    UNW_ALPHA_R12,      /*  s3 */
    UNW_ALPHA_R13,      /*  s4 */
    UNW_ALPHA_R14,      /*  s5 */
    UNW_ALPHA_R15,      /*  s6/fp: frame pointer */
    UNW_ALPHA_R16,      /*  a0: argument */
    UNW_ALPHA_R17,      /*  a1 */
    UNW_ALPHA_R18,      /*  a2 */
    UNW_ALPHA_R19,      /*  a3 */
    UNW_ALPHA_R20,      /*  a4 */
    UNW_ALPHA_R21,      /*  a5 */
    UNW_ALPHA_R22,      /*  t8 */
    UNW_ALPHA_R23,      /*  t9 */
    UNW_ALPHA_R24,      /* t10 */
    UNW_ALPHA_R25,      /* t11 */
    UNW_ALPHA_R26,      /*  ra: return address */
    UNW_ALPHA_R27,      /* t12/pv: procedure value */
    UNW_ALPHA_R28,      /*  at: assembler temporary */
    UNW_ALPHA_R29,      /*  gp: global pointer */
    UNW_ALPHA_R30,      /*  sp: stack pointer */
    UNW_ALPHA_R31,      /* zero: hardwired zero */

    /* Floating-point registers: DWARF numbers 32-63 */
    UNW_ALPHA_F0,       /* $f0 */
    UNW_ALPHA_F1,
    UNW_ALPHA_F2,       /* callee-saved */
    UNW_ALPHA_F3,
    UNW_ALPHA_F4,
    UNW_ALPHA_F5,
    UNW_ALPHA_F6,
    UNW_ALPHA_F7,
    UNW_ALPHA_F8,
    UNW_ALPHA_F9,       /* end of callee-saved FPRs */
    UNW_ALPHA_F10,
    UNW_ALPHA_F11,
    UNW_ALPHA_F12,
    UNW_ALPHA_F13,
    UNW_ALPHA_F14,
    UNW_ALPHA_F15,
    UNW_ALPHA_F16,
    UNW_ALPHA_F17,
    UNW_ALPHA_F18,
    UNW_ALPHA_F19,
    UNW_ALPHA_F20,
    UNW_ALPHA_F21,
    UNW_ALPHA_F22,
    UNW_ALPHA_F23,
    UNW_ALPHA_F24,
    UNW_ALPHA_F25,
    UNW_ALPHA_F26,
    UNW_ALPHA_F27,
    UNW_ALPHA_F28,
    UNW_ALPHA_F29,
    UNW_ALPHA_F30,
    UNW_ALPHA_F31,      /* $f31: hardwired zero */

    /* Program counter: DWARF number 64 */
    UNW_ALPHA_PC = 64,

    UNW_TDEP_LAST_REG = UNW_ALPHA_PC,

    /* frame info (read-only) */
    UNW_ALPHA_CFA,

    UNW_TDEP_IP = UNW_ALPHA_PC,
    UNW_TDEP_SP = UNW_ALPHA_R30,

    UNW_TDEP_EH = UNW_ALPHA_R0,
  }
alpha_regnum_t;

#define UNW_TDEP_NUM_EH_REGS    2

typedef struct unw_tdep_save_loc
  {
    /* Additional target-dependent info on a save location.  */
    UNW_EMPTY_STRUCT
  }
unw_tdep_save_loc_t;

/* On Alpha, we can directly use ucontext_t as the unwind context.  */
typedef ucontext_t unw_tdep_context_t;

typedef struct
  {
    /* no Alpha-specific auxiliary proc-info */
    UNW_EMPTY_STRUCT
  }
unw_tdep_proc_info_t;

#include "libunwind-dynamic.h"
#include "libunwind-common.h"

#define unw_tdep_getcontext             UNW_ARCH_OBJ(getcontext)
#define unw_tdep_is_fpreg               UNW_ARCH_OBJ(is_fpreg)

extern int unw_tdep_getcontext (unw_tdep_context_t *);
extern int unw_tdep_is_fpreg (int);

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

#endif /* LIBUNWIND_H */
