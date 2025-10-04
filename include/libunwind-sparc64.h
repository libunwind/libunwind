/* libunwind - a platform-independent unwind library
   Copyright (C) 2014 Oracle Inc.
   Contributed by
     Jose E. Marchesi <jose.marchesi@oracle.com>

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

#include <inttypes.h>
#include <ucontext.h>

#define UNW_TARGET		sparc64
#define UNW_TARGET_SPARC64	1
#define UNW_TARGET_SPARC        1

#define _U_TDEP_QP_TRUE	0	/* see libunwind-dynamic.h  */

/*
 * This needs to be big enough to accommodate "struct cursor", while
 * leaving some slack for future expansion.  Changing this value will
 * require recompiling all users of this library.  Stack allocation is
 * relatively cheap and unwind-state copying is relatively rare, so we want
 * to err on making it rather too big than too small.
 *
 * To simplify this whole process, we are at least initially taking
 * the tack that UNW_SPARC64_* map straight across to the .eh_frame
 * column register numbers.  These register numbers come from gcc's
 * source in gcc/config/sparc/sparc.h and are the hard register
 * numbers used by the compiler.
 *
 * UNW_TDEP_CURSOR_LEN is in terms of unw_word_t size.  Since we have 115
 * elements in the loc array, each sized 2 * unw_word_t, plus the rest of
 * the cursor struct, this puts us at about 2 * 115 + 40 = 270.  Let's
 * round that up to 280.
 */

#define UNW_TDEP_CURSOR_LEN 280

#if __WORDSIZE==32
typedef uint32_t unw_word_t;
typedef int32_t unw_sword_t;
#define UNW_WORD_MAX UINT32_MAX
#else
typedef uint64_t unw_word_t;
typedef int64_t unw_sword_t;
#define UNW_WORD_MAX UINT64_MAX
#endif

typedef long double unw_tdep_fpreg_t;

typedef enum
  {
    UNW_SPARC64_FIRST_GPREG = 0,

    /* General registers.  */

    UNW_SPARC64_G0 = UNW_SPARC64_FIRST_GPREG,
    UNW_SPARC64_G1,
    UNW_SPARC64_G2,
    UNW_SPARC64_G3,
    UNW_SPARC64_G4,
    UNW_SPARC64_G5,
    UNW_SPARC64_G6,
    UNW_SPARC64_G7,

    /* Output registers.  */

    UNW_SPARC64_O0 = 8,
    UNW_SPARC64_O1,
    UNW_SPARC64_O2,
    UNW_SPARC64_O3,
    UNW_SPARC64_O4,
    UNW_SPARC64_O5,
    UNW_SPARC64_O6 = 14, /* UNW_SPARC64_SP */
    UNW_SPARC64_O7,

    /* Local registers.  */

    UNW_SPARC64_L0 = 16,
    UNW_SPARC64_L1,
    UNW_SPARC64_L2,
    UNW_SPARC64_L3,
    UNW_SPARC64_L4,
    UNW_SPARC64_L5,
    UNW_SPARC64_L6,
    UNW_SPARC64_L7,

    /* Input registers.  */

    UNW_SPARC64_I0 = 24,
    UNW_SPARC64_I1,
    UNW_SPARC64_I2,
    UNW_SPARC64_I3,
    UNW_SPARC64_I4,
    UNW_SPARC64_I5,
    UNW_SPARC64_I6 = 30, /* UNW_SPARC64_FP */
    UNW_SPARC64_I7,

    UNW_SPARC64_LAST_GPREG = UNW_SPARC64_I7,
    UNW_SPARC64_FIRST_FPREG = 32,

    /* Floating-point registers.  */

    UNW_SPARC64_F0 = UNW_SPARC64_FIRST_FPREG,
    UNW_SPARC64_F1,
    UNW_SPARC64_F2,
    UNW_SPARC64_F3,
    UNW_SPARC64_F4,
    UNW_SPARC64_F5,
    UNW_SPARC64_F6,
    UNW_SPARC64_F7,

    UNW_SPARC64_F8 = 40,
    UNW_SPARC64_F9,
    UNW_SPARC64_F10,
    UNW_SPARC64_F11,
    UNW_SPARC64_F12,
    UNW_SPARC64_F13,
    UNW_SPARC64_F14,
    UNW_SPARC64_F15,

    UNW_SPARC64_F16 = 48,
    UNW_SPARC64_F17,
    UNW_SPARC64_F18,
    UNW_SPARC64_F19,
    UNW_SPARC64_F20,
    UNW_SPARC64_F21,
    UNW_SPARC64_F22,
    UNW_SPARC64_F23,

    UNW_SPARC64_F24 = 56,
    UNW_SPARC64_F25,
    UNW_SPARC64_F26,
    UNW_SPARC64_F27,
    UNW_SPARC64_F28,
    UNW_SPARC64_F29,
    UNW_SPARC64_F30,
    UNW_SPARC64_F31,

    UNW_SPARC64_F32 = 64,
    UNW_SPARC64_F33,
    UNW_SPARC64_F34,
    UNW_SPARC64_F35,
    UNW_SPARC64_F36,
    UNW_SPARC64_F37,
    UNW_SPARC64_F38,
    UNW_SPARC64_F39,

    UNW_SPARC64_F40 = 72,
    UNW_SPARC64_F41,
    UNW_SPARC64_F42,
    UNW_SPARC64_F43,
    UNW_SPARC64_F44,
    UNW_SPARC64_F45,
    UNW_SPARC64_F46,
    UNW_SPARC64_F47,

    UNW_SPARC64_F48 = 80,
    UNW_SPARC64_F49,
    UNW_SPARC64_F50,
    UNW_SPARC64_F51,
    UNW_SPARC64_F52,
    UNW_SPARC64_F53,
    UNW_SPARC64_F54,
    UNW_SPARC64_F55,

    UNW_SPARC64_F56 = 88,
    UNW_SPARC64_F57,
    UNW_SPARC64_F58,
    UNW_SPARC64_F59,
    UNW_SPARC64_F60,
    UNW_SPARC64_F61,
    UNW_SPARC64_F62,
    UNW_SPARC64_F63,

    UNW_SPARC64_LAST_FPREG = UNW_SPARC64_F63,

    /* Control and status registers.  */

    UNW_SPARC64_FCC0 = 96,
    UNW_SPARC64_FCC1,
    UNW_SPARC64_FCC2,
    UNW_SPARC64_FCC3,
    UNW_SPARC64_ICC = 100,
    UNW_SPARC64_SFP = 101,
    UNW_SPARC64_GSR = 102,

    UNW_SPARC64_PC  = 103,

    UNW_TDEP_LAST_REG = UNW_SPARC64_GSR,

    UNW_TDEP_IP = UNW_SPARC64_O7,
    UNW_TDEP_SP = UNW_SPARC64_O6,
    UNW_TDEP_EH = UNW_SPARC64_G0 /* XXX: exception handler */
  }
sparc64_regnum_t;

#define UNW_TDEP_NUM_EH_REGS	1 /* XXX */

typedef struct unw_tdep_save_loc
  {
    /* Additional target-dependent info on a save location.  */
  }
unw_tdep_save_loc_t;

  /* On sparc64, we can directly use ucontext_t as the unwind context.  */
typedef ucontext_t unw_tdep_context_t;

/* XXX this is not ideal: an application should not be prevented from
   using the "getcontext" name just because it's using libunwind.  We
   can't just use __getcontext() either, because that isn't exported
   by glibc...  */
#define unw_tdep_getcontext(uc)		(getcontext (uc), 0)

#include "libunwind-dynamic.h"

typedef struct
  {
    /* no sparc64-specific auxiliary proc-info */
  }
unw_tdep_proc_info_t;

#include "libunwind-common.h"

#define unw_tdep_is_fpreg		UNW_ARCH_OBJ(is_fpreg)
extern int unw_tdep_is_fpreg (int);

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

#endif /* LIBUNWIND_H */
