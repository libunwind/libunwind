/* libunwind - a platform-independent unwind library
   Copyright (C) 2026 libunwind contributors

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
NONINFRINGEMENT.  IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.  */

/* Layout of the Linux kernel's struct ucontext on ppc64, as used by
   getcontext.S and setcontext.S.  It is usable from assembly; Ginit.c
   checks it against the C library's ucontext_t where those functions
   implement unw_getcontext() and unw_resume().  */

#ifndef ppc64_context_i_h
#define ppc64_context_i_h

#define PPC64_UC_SIZE           1696    /* sizeof (struct ucontext) */
#define PPC64_UC_SIGMASK        40      /* uc_sigmask */
#define PPC64_UC_GREGS          232     /* uc_mcontext.gp_regs */
#define PPC64_UC_FPREGS         616     /* uc_mcontext.fp_regs */
#define PPC64_UC_VREGS          880     /* uc_mcontext.v_regs */

#define PPC64_UC_GREG(i)        (PPC64_UC_GREGS + 8 * (i))
#define PPC64_UC_NIP            PPC64_UC_GREG (32)
#define PPC64_UC_MSR            PPC64_UC_GREG (33)
#define PPC64_UC_CTR            PPC64_UC_GREG (35)
#define PPC64_UC_LINK           PPC64_UC_GREG (36)
#define PPC64_UC_XER            PPC64_UC_GREG (37)
#define PPC64_UC_CCR            PPC64_UC_GREG (38)
#define PPC64_UC_FPSCR          (PPC64_UC_FPREGS + 8 * 32)

/* *v_regs: 32 VRs, then VSCR and VRSAVE in a 16-byte slot each.  */
#define PPC64_VREGS_VSCR        (16 * 32)
#define PPC64_VREGS_VRSAVE      (16 * 33)

#define PPC64_MSR_VEC_HI        0x0200  /* MSR_VEC >> 16 */

#endif /* ppc64_context_i_h */
