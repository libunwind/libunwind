/* libunwind - a platform-independent unwind library
   Copyright (C) 2003 Hewlett-Packard Co
	Contributed by Scott Marovitch

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

#define UNW_TARGET	hppa
#define UNW_TARGET_HPPA	1

/* This needs to be big enough to accommodate "struct cursor", while
   leaving some slack for future expansion.  Changing this value will
   require recompiling all users of this library.  Stack allocation is
   relatively cheap and unwind-state copying is relatively rare, so we
   want to err on making it rather too big than too small.  */
#define UNW_TDEP_CURSOR_LEN	127

typedef uint64_t unw_tdep_word_t;

typedef enum
  {
    /* Note: general registers are excepted to start with index 0.
       This convention facilitates architecture-independent
       implementation of the C++ exception handling ABI.  See
       _Unwind_SetGR() and _Unwind_GetGR() for details.  */
    UNW_HPPA_GR = 0,
     UNW_HPPA_SP = UNW_HPPA_GR + 30,

    UNW_HPPA_FR = UNW_HPPA_GR + 32,

    UNW_HPPA_IP = UNW_HPPA_FR + 32,	/* instruction pointer */

    /* other "preserved" registers (fpsr etc.)... */

    UNW_TDEP_LAST_REG = UNW_HPPA_IP,

    UNW_TDEP_IP = UNW_HPPA_IP,
    UNW_TDEP_SP = UNW_HPPA_SP
  }
hppa_regnum_t;

typedef struct unw_tdep_save_loc
  {
    /* Additional target-dependent info on a save location.  */
  }
unw_tdep_save_loc_t;

/* On PA-RISC, we can directly use ucontext_t as the unwind context.  */
typedef ucontext_t unw_tdep_context_t;

/* XXX this is not ideal: an application should not be prevented from
   using the "getcontext" name just because it's using libunwind.  We
   can't just use __getcontext() either, because that isn't exported
   by glibc...  */
#define unw_tdep_getcontext(uc)		(getcontext (uc), 0)

/* XXX fixme: */
#define unw_tdep_is_fpreg(r)		((unsigned) ((r) - UNW_HPPA_FR) < 128)

#include "libunwind-common.h"

#endif /* LIBUNWIND_H */
