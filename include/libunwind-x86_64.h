/* libunwind - a platform-independent unwind library
   Copyright (C) 2002-2004 Hewlett-Packard Co
	Contributed by David Mosberger-Tang <davidm@hpl.hp.com>

   Modified for x86_64 by Max Asbock <masbock@us.ibm.com>

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

#define UNW_TARGET		x86_64
#define UNW_TARGET_X86_64	1

#define _U_TDEP_QP_TRUE	0	/* see libunwind-dynamic.h  */

/* This needs to be big enough to accommodate "struct cursor", while
   leaving some slack for future expansion.  Changing this value will
   require recompiling all users of this library.  Stack allocation is
   relatively cheap and unwind-state copying is relatively rare, so we
   want to err on making it rather too big than too small.  */
#define UNW_TDEP_CURSOR_LEN	127

typedef uint64_t unw_word_t;
typedef int64_t unw_sword_t;

typedef long double unw_tdep_fpreg_t;

typedef enum
  {
    UNW_X86_64_RAX,
    UNW_X86_64_RDX,
    UNW_X86_64_RCX,
    UNW_X86_64_RBX,
    UNW_X86_64_RSI,
    UNW_X86_64_RDI,
    UNW_X86_64_RBP,
    UNW_X86_64_RSP,
    UNW_X86_64_R8,
    UNW_X86_64_R9,
    UNW_X86_64_R10,
    UNW_X86_64_R11,
    UNW_X86_64_R12,
    UNW_X86_64_R13,
    UNW_X86_64_R14,
    UNW_X86_64_R15,
    UNW_X86_64_RIP,

    /* XXX Add other regs here */

    /* frame info (read-only) */
    UNW_X86_64_CFA,

    UNW_TDEP_LAST_REG = UNW_X86_64_RIP,

    UNW_TDEP_IP = UNW_X86_64_RIP,
    UNW_TDEP_SP = UNW_X86_64_RSP,
    UNW_TDEP_BP = UNW_X86_64_RBP,
    UNW_TDEP_EH = UNW_X86_64_RAX
  }
x86_64_regnum_t;

#define UNW_TDEP_NUM_EH_REGS	2	/* XXX Not sure what this means */

typedef struct unw_tdep_save_loc
  {
    /* Additional target-dependent info on a save location.  */
  }
unw_tdep_save_loc_t;

/* On x86_64, we can directly use ucontext_t as the unwind context.  */
typedef ucontext_t unw_tdep_context_t;

typedef struct
  {
    /* no x86-64-specific auxiliary proc-info */
  }
unw_tdep_proc_info_t;

typedef enum
  {
    UNW_X86_64_FRAME_STANDARD = -2,     /* regular rbp, rsp +/- offset */
    UNW_X86_64_FRAME_SIGRETURN = -1,    /* special sigreturn frame */
    UNW_X86_64_FRAME_OTHER = 0,         /* not cacheable (special or unrecognised) */
    UNW_X86_64_FRAME_GUESSED = 1        /* guessed it was regular, but not known */
  }
unw_tdep_frame_type_t;

typedef struct
  {
    uint64_t virtual_address;
    int64_t frame_type     : 2;  /* unw_tdep_frame_type_t classification */
    int64_t last_frame     : 1;  /* non-zero if last frame in chain */
    int64_t cfa_reg_rsp    : 1;  /* cfa dwarf base register is rsp vs. rbp */
    int64_t cfa_reg_offset : 30; /* cfa is at this offset from base register value */
    int64_t rbp_cfa_offset : 15; /* rbp saved at this offset from cfa (-1 = not saved) */
    int64_t rsp_cfa_offset : 15; /* rsp saved at this offset from cfa (-1 = not saved) */
  }
unw_tdep_frame_t;

#include "libunwind-dynamic.h"
#include "libunwind-common.h"

#define unw_tdep_getcontext		UNW_ARCH_OBJ(getcontext)
#define unw_tdep_is_fpreg		UNW_ARCH_OBJ(is_fpreg)
#define unw_tdep_trace			UNW_OBJ(trace)

extern int unw_tdep_getcontext (unw_tdep_context_t *);
extern int unw_tdep_is_fpreg (int);

extern int unw_tdep_trace (unw_cursor_t *cursor, void **addresses, int *n);

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

#endif /* LIBUNWIND_H */
