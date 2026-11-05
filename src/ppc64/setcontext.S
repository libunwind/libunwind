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

#include "context_i.h"

#ifdef __linux__
#include <features.h>
#endif

/* With glibc, unw_resume() uses the C library's setcontext().  */
#if defined(__linux__) && !defined(__GLIBC__)

/*  int _Uppc64_setcontext (const ucontext_t *ucp)

   Restore the machine state saved in *UCP and continue at its PC, as
   setcontext() does.  Used by unw_resume() where the C library has no
   setcontext() that can be relied on: libucontext's goes through the
   swapcontext syscall, which qemu-user implements without restoring r2
   on ppc64, so code resumed in another module runs with the wrong TOC.

   This is done in user space, as glibc does: the signal mask with
   rt_sigprocmask, then FPSCR and the FPRs, the VRs, VSCR and VRSAVE if
   the saved MSR says the VMX state is live, and the special and general
   purpose registers.  r13, the thread pointer, is left alone.  There is
   no way to jump to the saved PC without clobbering LR or CTR from user
   space; like glibc, use CTR.  r1 is loaded last: once it points at the
   resumed frame, *UCP may be below the stack pointer and outside the
   protected zone, where a signal handler could overwrite it.  Returns -1
   only if the signal mask could not be set.

   The function is internal to the library that contains it.  */

#define FUNC _Uppc64_setcontext

#if defined(_CALL_ELF) && _CALL_ELF == 2
	.abiversion 2
	.text
	.globl	FUNC
	.hidden	FUNC
	.type	FUNC, @function
	.p2align 4
FUNC:
#else
	/* ELFv1: the symbol names a function descriptor.  */
	.section ".opd", "aw"
	.p2align 3
	.globl	FUNC
	.hidden	FUNC
	.type	FUNC, @function
FUNC:
	.quad	.L.FUNC, .TOC.@tocbase, 0
	.text
	.p2align 4
.L.FUNC:
#endif
	std	3, -8(1)		/* UCP, in the protected zone */

	/* rt_sigprocmask (SIG_SETMASK, &ucp->uc_sigmask, NULL, 8) */
	addi	4, 3, PPC64_UC_SIGMASK
	li	3, 2			/* SIG_SETMASK */
	li	5, 0
	li	6, 8			/* sizeof the kernel's sigset_t */
	li	0, 174			/* __NR_rt_sigprocmask */
	sc
	bso-	.Lerror

	/* From here on, the caller's state is gone.  */
	ld	31, -8(1)

	lfd	0, PPC64_UC_FPSCR(31)
	mtfsf	0xff, 0
	.irp	n, 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31
	lfd	\n, PPC64_UC_FPREGS + 8 * \n(31)
	.endr

	ld	0, PPC64_UC_MSR(31)
	andis.	0, 0, PPC64_MSR_VEC_HI
	beq	1f
	ld	3, PPC64_UC_VREGS(31)
	cmpdi	3, 0
	beq	1f
	.machine push
	.machine "altivec"
	li	4, PPC64_VREGS_VSCR
	lvx	0, 3, 4
	mtvscr	0
	lwz	4, PPC64_VREGS_VRSAVE(3)
	mtvrsave 4
	.irp	n, 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31
	li	4, 16 * \n
	lvx	\n, 3, 4
	.endr
	.machine pop
1:
	ld	0, PPC64_UC_NIP(31)
	mtctr	0
	ld	0, PPC64_UC_LINK(31)
	mtlr	0
	ld	0, PPC64_UC_XER(31)
	mtxer	0
	ld	0, PPC64_UC_CCR(31)
	mtcr	0
	ld	0, PPC64_UC_GREG (0)(31)
	.irp	n, 2,3,4,5,6,7,8,9,10,11,12,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30
	ld	\n, PPC64_UC_GREG (\n)(31)
	.endr
	ld	1, PPC64_UC_GREG (1)(31)
	ld	31, PPC64_UC_GREG (31)(31)
	bctr

.Lerror:
	li	3, -1
	blr
#if defined(_CALL_ELF) && _CALL_ELF == 2
	.size	FUNC, . - FUNC
#else
	.size	FUNC, 24
#endif

#endif /* __linux__ && !__GLIBC__ */

	/* We do not need executable stack.  */
	.section	.note.GNU-stack,"",@progbits
