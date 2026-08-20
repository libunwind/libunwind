/* libunwind - a platform-independent unwind library

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

/*  int _Uppc32_setcontext (const ucontext_t *ucp)

    Counterpart of getcontext.S, for C libraries that do not provide
    setcontext().  Does not return.

    NIP is loaded into CTR and jumped through, so the context's own CTR
    value is not restored.  Reaching the target costs either LR or CTR,
    and LR is the one that matters here: ppc32_local_resume() resumes a
    frame that may return through it.

    This provides what unw_resume() needs; it is not meant as a general
    purpose replacement for the libc setcontext().  */

/* See getcontext.S: __GLIBC__ is not visible in a .S translation unit
   unless <features.h> is pulled in first.  */
#if defined(__has_include)
# if __has_include(<features.h>)
#  include <features.h>
# endif
#endif

#if defined(__linux__) && !defined(__GLIBC__)

#include "ucontext_offsets.h"

	.text
	.align 2
	.globl _Uppc32_setcontext
	.type _Uppc32_setcontext,@function

_Uppc32_setcontext:
	/* Follow uc_regs rather than assuming the mcontext sits at a fixed
	   offset, the way uc_addr() and Gresume.c reach it.  From here r3
	   addresses the mcontext and keeps doing so until the very last
	   load; r0 is scratch.  */
	lwz	3, UC_REGS_PTR_OFF(3)

	lwz	0, UC_MREG(UC_CCR_IDX)(3)
	mtcr	0
	lwz	0, UC_MREG(UC_XER_IDX)(3)
	mtxer	0
	lwz	0, UC_MREG(UC_LINK_IDX)(3)
	mtlr	0

	/* Jump target.  */
	lwz	0, UC_MREG(UC_NIP_IDX)(3)
	mtctr	0

	lwz	2, UC_MREG(2)(3)
	lwz	4, UC_MREG(4)(3)
	lwz	5, UC_MREG(5)(3)
	lwz	6, UC_MREG(6)(3)
	lwz	7, UC_MREG(7)(3)
	lwz	8, UC_MREG(8)(3)
	lwz	9, UC_MREG(9)(3)
	lwz	10, UC_MREG(10)(3)
	lwz	11, UC_MREG(11)(3)
	lwz	12, UC_MREG(12)(3)
	lwz	13, UC_MREG(13)(3)
	lwz	14, UC_MREG(14)(3)
	lwz	15, UC_MREG(15)(3)
	lwz	16, UC_MREG(16)(3)
	lwz	17, UC_MREG(17)(3)
	lwz	18, UC_MREG(18)(3)
	lwz	19, UC_MREG(19)(3)
	lwz	20, UC_MREG(20)(3)
	lwz	21, UC_MREG(21)(3)
	lwz	22, UC_MREG(22)(3)
	lwz	23, UC_MREG(23)(3)
	lwz	24, UC_MREG(24)(3)
	lwz	25, UC_MREG(25)(3)
	lwz	26, UC_MREG(26)(3)
	lwz	27, UC_MREG(27)(3)
	lwz	28, UC_MREG(28)(3)
	lwz	29, UC_MREG(29)(3)
	lwz	30, UC_MREG(30)(3)
	lwz	31, UC_MREG(31)(3)

	/* r0, r1 and r3 last.  Restoring the stack pointer any earlier
	   would leave the remaining loads reading the context from below
	   the new SP, where a signal frame may land: ppc32 SysV has no red
	   zone.  Only the r3 load sits in that window now.  */
	lwz	0, UC_MREG(0)(3)
	lwz	1, UC_MREG(1)(3)
	lwz	3, UC_MREG(3)(3)
	bctr

	.size _Uppc32_setcontext,.-_Uppc32_setcontext

#endif /* __linux__ && !__GLIBC__ */

#if defined(__linux__) && defined(__ELF__)
	.section .note.GNU-stack,"",@progbits
#endif
