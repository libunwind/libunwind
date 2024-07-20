/* libunwind - a platform-independent unwind library
   Copyright (C) 2006-2007 IBM
   Contributed by
     Corey Ashford <cjashfor@us.ibm.com>
     Jose Flavio Aguilar Paulino <jflavio@br.ibm.com> <joseflavio@gmail.com>

   Copied from libunwind-x86_64.h, modified slightly for building
   frysk successfully on ppc64, by Wu Zhou <woodzltc@cn.ibm.com>
   Will be replaced when libunwind is ready on ppc64 platform.

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
#include <stdint.h>
#include <ucontext.h>

#ifndef UNW_EMPTY_STRUCT
#  define UNW_EMPTY_STRUCT uint8_t unused;
#endif

#define UNW_TARGET              ppc32
#define UNW_TARGET_PPC32        1

#define _U_TDEP_QP_TRUE 0       /* see libunwind-dynamic.h  */

/*
 * This needs to be big enough to accommodate "struct cursor", while
 * leaving some slack for future expansion.  Changing this value will
 * require recompiling all users of this library.  Stack allocation is
 * relatively cheap and unwind-state copying is relatively rare, so we want
 * to err on making it rather too big than too small.
 *
 * UNW_PPC32_* register numbers match the DWARF register numbers used by
 * GCC in .eh_frame data (see gcc/config/rs6000/rs6000.h).
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
# define UNW_WORD_MAX UINT32_MAX
#else
typedef uint64_t unw_word_t;
typedef int64_t unw_sword_t;
# define UNW_WORD_MAX UINT64_MAX
#endif

typedef double unw_tdep_fpreg_t;

typedef enum
  {
    UNW_PPC32_R0,
    UNW_PPC32_R1, /* called STACK_POINTER in gcc */
    UNW_PPC32_R2,
    UNW_PPC32_R3,
    UNW_PPC32_R4,
    UNW_PPC32_R5,
    UNW_PPC32_R6,
    UNW_PPC32_R7,
    UNW_PPC32_R8,
    UNW_PPC32_R9,
    UNW_PPC32_R10,
    UNW_PPC32_R11, /* called STATIC_CHAIN in gcc */
    UNW_PPC32_R12,
    UNW_PPC32_R13,
    UNW_PPC32_R14,
    UNW_PPC32_R15,
    UNW_PPC32_R16,
    UNW_PPC32_R17,
    UNW_PPC32_R18,
    UNW_PPC32_R19,
    UNW_PPC32_R20,
    UNW_PPC32_R21,
    UNW_PPC32_R22,
    UNW_PPC32_R23,
    UNW_PPC32_R24,
    UNW_PPC32_R25,
    UNW_PPC32_R26,
    UNW_PPC32_R27,
    UNW_PPC32_R28,
    UNW_PPC32_R29,
    UNW_PPC32_R30,
    UNW_PPC32_R31, /* called HARD_FRAME_POINTER in gcc */

    UNW_PPC32_F0 = 32,
    UNW_PPC32_F1,
    UNW_PPC32_F2,
    UNW_PPC32_F3,
    UNW_PPC32_F4,
    UNW_PPC32_F5,
    UNW_PPC32_F6,
    UNW_PPC32_F7,
    UNW_PPC32_F8,
    UNW_PPC32_F9,
    UNW_PPC32_F10,
    UNW_PPC32_F11,
    UNW_PPC32_F12,
    UNW_PPC32_F13,
    UNW_PPC32_F14,
    UNW_PPC32_F15,
    UNW_PPC32_F16,
    UNW_PPC32_F17,
    UNW_PPC32_F18,
    UNW_PPC32_F19,
    UNW_PPC32_F20,
    UNW_PPC32_F21,
    UNW_PPC32_F22,
    UNW_PPC32_F23,
    UNW_PPC32_F24,
    UNW_PPC32_F25,
    UNW_PPC32_F26,
    UNW_PPC32_F27,
    UNW_PPC32_F28,
    UNW_PPC32_F29,
    UNW_PPC32_F30,
    UNW_PPC32_F31,
    /* Note that there doesn't appear to be an .eh_frame register column
       for the FPSCR register.  I don't know why this is.  Since .eh_frame
       info is what this implementation uses for unwinding, we have no way
       to unwind this register, and so we will not expose an FPSCR register
       number in the libunwind API.
     */

    UNW_PPC32_LR = 65,
    UNW_PPC32_CTR = 66,

    UNW_PPC32_CCR = 68,

    UNW_PPC32_XER = 76,

    /* frame info (read-only) */
    UNW_PPC32_NIP = 77,

    UNW_TDEP_LAST_REG = UNW_PPC32_NIP,

    UNW_TDEP_IP = UNW_PPC32_NIP,
    UNW_TDEP_SP = UNW_PPC32_R1,
    UNW_TDEP_EH = UNW_PPC32_R12
  }
ppc32_regnum_t;

/*
 * According to David Edelsohn, GNU gcc uses R3, R4, R5, and maybe R6 for
 * passing parameters to exception handlers.
 */

#define UNW_TDEP_NUM_EH_REGS    4

typedef struct unw_tdep_save_loc
  {
    /* Additional target-dependent info on a save location.  */
    UNW_EMPTY_STRUCT
  }
unw_tdep_save_loc_t;

/* On ppc32 Linux, glibc's getcontext() sets uc_mcontext.uc_regs to
   uc_reg_space + ((16 - offsetof(ucontext_t, uc_reg_space) % 16) % 16),
   a fixed offset that only lands on a 16-byte boundary when ucontext_t
   itself is 16-byte aligned.  mcontext_t requires 16-byte alignment (it
   contains vrregset_t __attribute__((aligned(16)))), so force the needed
   alignment here to avoid misaligned-pointer UB on every getcontext call.  */
typedef ucontext_t unw_tdep_context_t __attribute__((aligned(16)));

/* XXX this is not ideal: an application should not be prevented from
   using the "getcontext" name just because it's using libunwind.  We
   can't just use __getcontext() either, because that isn't exported
   by glibc...  */
#if defined(__linux__) && defined(__GLIBC__)
/* On ppc32 Linux, glibc's getcontext() saves the NIP as an address inside
   getcontext itself (via an internal `bcl` to read the current PC), rather
   than the caller's return address.  That adds an extra frame to every
   unwind, shifting results versus other architectures (e.g. x86_64) whose
   getcontext saves the caller's return address as the instruction pointer.
   The saved LR (gregs[36]) is the caller's return address, so overwrite
   NIP (gregs[32]) with it to put the cursor in the caller's frame.  */
#define unw_tdep_getcontext(uc)                                        \
  (getcontext (uc),                                                    \
   (uc)->uc_mcontext.uc_regs->gregs[32]                                \
     = (uc)->uc_mcontext.uc_regs->gregs[36],                           \
   0)
#else
#define unw_tdep_getcontext(uc)         (getcontext (uc), 0)
#endif

#include "libunwind-dynamic.h"

typedef struct
  {
    /* no ppc32-specific auxiliary proc-info */
    UNW_EMPTY_STRUCT
  }
unw_tdep_proc_info_t;

#include "libunwind-common.h"

#define unw_tdep_is_fpreg               UNW_ARCH_OBJ(is_fpreg)
extern int unw_tdep_is_fpreg (int);

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

#endif /* LIBUNWIND_H */
