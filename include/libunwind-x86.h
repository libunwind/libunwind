/* libunwind - a platform-independent unwind library
   Copyright (C) 2002-2003 Hewlett-Packard Co
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

#define UNW_TARGET	x86
#define UNW_TARGET_X86	1

/* This needs to be big enough to accommodate "struct cursor", while
   leaving some slack for future expansion.  Changing this value will
   require recompiling all users of this library.  Stack allocation is
   relatively cheap and unwind-state copying is relatively rare, so we
   want to err on making it rather too big than too small.  */
#define UNW_TDEP_CURSOR_LEN	127

typedef uint32_t unw_tdep_word_t;

typedef long double unw_tdep_fpreg_t;

typedef enum
  {
    /* Note: general registers are excepted to start with index 0.
       This convention facilitates architecture-independent
       implementation of the C++ exception handling ABI.  See
       _Unwind_SetGR() and _Unwind_GetGR() for details.  */
    UNW_X86_EAX,
    UNW_X86_EBX,
    UNW_X86_ECX,
    UNW_X86_EDX,
    UNW_X86_ESI,
    UNW_X86_EDI,
    UNW_X86_EBP,
    UNW_X86_EIP,
    UNW_X86_ESP,

    UNW_TDEP_LAST_REG = UNW_X86_ESP,

    UNW_TDEP_IP = UNW_X86_EIP,
    UNW_TDEP_SP = UNW_X86_ESP,
    UNW_TDEP_EH = UNW_X86_EAX
  }
x86_regnum_t;

#define UNW_TDEP_NUM_EH_REGS	2	/* eax and ebx are exception args */

typedef struct unw_tdep_save_loc
  {
    /* Additional target-dependent info on a save location.  */
  }
unw_tdep_save_loc_t;

/* On x86, we can directly use ucontext_t as the unwind context.  */
typedef ucontext_t unw_tdep_context_t;

/* XXX this is not ideal: an application should not be prevented from
   using the "getcontext" name just because it's using libunwind.  We
   can't just use __getcontext() either, because that isn't exported
   by glibc...  */
#define unw_tdep_getcontext(uc)		(getcontext (uc), 0)

/* XXX fixme: */
#define unw_tdep_is_fpreg(r)		((unsigned) ((r) - UNW_X86_FR) < 128)

#include "libunwind-common.h"

#endif /* LIBUNWIND_H */
