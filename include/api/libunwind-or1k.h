/* libunwind - a platform-independent unwind library
   Copyright (C) 2008 CodeSourcery
   Copyright (C) 2012 Tommi Rantala <tt.rantala@gmail.com>

   Modified for or1k by Ali Ahmet Memiş <aliamemis@disroot.org>

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
#include <stddef.h>
#include <stdint.h>
#include <ucontext.h>

#ifndef UNW_EMPTY_STRUCT
#  define UNW_EMPTY_STRUCT uint8_t unused;
#endif

#define UNW_TARGET      or1k
#define UNW_TARGET_OR1K 1

#define _U_TDEP_QP_TRUE 0       /* see libunwind-dynamic.h  */

/* This needs to be big enough to accommodate "struct cursor", while
   leaving some slack for future expansion.  Changing this value will
   require recompiling all users of this library.  Stack allocation is
   relatively cheap and unwind-state copying is relatively rare, so we
   want to err on making it rather too big than too small.  */

#define UNW_TDEP_CURSOR_LEN     4096

typedef uint32_t unw_word_t;
typedef int32_t unw_sword_t;

typedef double unw_tdep_fpreg_t;

#define UNW_WORD_MAX UINT32_MAX

/* OpenRISC DWARF register numbers are the hardware register numbers.
   Column 32 is the PC, used as the return address of a signal frame.
   Ordinary frames return through r9.  */

typedef enum
  {
    UNW_OR1K_R0,
    UNW_OR1K_R1,
    UNW_OR1K_R2,
    UNW_OR1K_R3,
    UNW_OR1K_R4,
    UNW_OR1K_R5,
    UNW_OR1K_R6,
    UNW_OR1K_R7,
    UNW_OR1K_R8,
    UNW_OR1K_R9,
    UNW_OR1K_R10,
    UNW_OR1K_R11,
    UNW_OR1K_R12,
    UNW_OR1K_R13,
    UNW_OR1K_R14,
    UNW_OR1K_R15,
    UNW_OR1K_R16,
    UNW_OR1K_R17,
    UNW_OR1K_R18,
    UNW_OR1K_R19,
    UNW_OR1K_R20,
    UNW_OR1K_R21,
    UNW_OR1K_R22,
    UNW_OR1K_R23,
    UNW_OR1K_R24,
    UNW_OR1K_R25,
    UNW_OR1K_R26,
    UNW_OR1K_R27,
    UNW_OR1K_R28,
    UNW_OR1K_R29,
    UNW_OR1K_R30,
    UNW_OR1K_R31,

    UNW_OR1K_PC,

    UNW_TDEP_LAST_REG = UNW_OR1K_PC,

    UNW_TDEP_IP = UNW_OR1K_PC,
    UNW_TDEP_SP = UNW_OR1K_R1,
    /* The exception-data registers are r25 and r27, so they are not
       contiguous: UNW_REG_EH + 1 addresses r26, not the second one.  The
       generic libunwind-setjmp code assumes they are adjacent.  */
    UNW_TDEP_EH = UNW_OR1K_R25
  }
or1k_regnum_t;

#define UNW_TDEP_NUM_EH_REGS    2

typedef ucontext_t unw_tdep_context_t;

typedef struct unw_tdep_save_loc
  {
    /* Additional target-dependent info on a save location.  */
    UNW_EMPTY_STRUCT
  }
unw_tdep_save_loc_t;

#include "libunwind-dynamic.h"

typedef struct
  {
    /* no or1k-specific auxiliary proc-info */
    UNW_EMPTY_STRUCT
  }
unw_tdep_proc_info_t;

#include "libunwind-common.h"

#define unw_tdep_getcontext             UNW_ARCH_OBJ(getcontext)
#define unw_tdep_is_fpreg               UNW_ARCH_OBJ(is_fpreg)

extern int unw_tdep_getcontext (unw_tdep_context_t *);
extern int unw_tdep_is_fpreg (int);

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

#endif /* LIBUNWIND_H */
