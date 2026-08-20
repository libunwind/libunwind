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

/*  int _Uppc32_getcontext (ucontext_t *ucp)

    Fill UCP the way the kernel fills a signal frame, so that the result
    is interchangeable with the ucontext_t a signal handler receives.

    Built only on Linux with a C library that does not provide
    getcontext(); libunwind-ppc32.h keeps using the libc one elsewhere.
    ucontext_offsets.h carries the literal offsets and checks them
    against the real ucontext_t, under the same condition as this file.  */

/* __GLIBC__ comes from <features.h>; without it the guard below would
   never fire and this would be assembled on glibc too, with the wrong
   offsets.  The header is glibc-only, hence __has_include.  */
#if defined(__has_include)
# if __has_include(<features.h>)
#  include <features.h>
# endif
#endif

#if defined(__linux__) && !defined(__GLIBC__)

#include <asm/unistd.h>
#include "ucontext_offsets.h"

	.text
	.align 2
	.globl _Uppc32_getcontext
	.type _Uppc32_getcontext,@function

_Uppc32_getcontext:
	/* r3 holds ucp.  No prologue runs, so r1 is still the caller's
	   stack pointer and LR still holds the return address.  r0 is
	   volatile and is reused as scratch once its incoming value has
	   been stored.  */

	stw	0, UC_GREG(0)(3)
	stw	1, UC_GREG(1)(3)
	stw	2, UC_GREG(2)(3)
	stw	3, UC_GREG(3)(3)
	stw	4, UC_GREG(4)(3)
	stw	5, UC_GREG(5)(3)
	stw	6, UC_GREG(6)(3)
	stw	7, UC_GREG(7)(3)
	stw	8, UC_GREG(8)(3)
	stw	9, UC_GREG(9)(3)
	stw	10, UC_GREG(10)(3)
	stw	11, UC_GREG(11)(3)
	stw	12, UC_GREG(12)(3)
	stw	13, UC_GREG(13)(3)
	stw	14, UC_GREG(14)(3)
	stw	15, UC_GREG(15)(3)
	stw	16, UC_GREG(16)(3)
	stw	17, UC_GREG(17)(3)
	stw	18, UC_GREG(18)(3)
	stw	19, UC_GREG(19)(3)
	stw	20, UC_GREG(20)(3)
	stw	21, UC_GREG(21)(3)
	stw	22, UC_GREG(22)(3)
	stw	23, UC_GREG(23)(3)
	stw	24, UC_GREG(24)(3)
	stw	25, UC_GREG(25)(3)
	stw	26, UC_GREG(26)(3)
	stw	27, UC_GREG(27)(3)
	stw	28, UC_GREG(28)(3)
	stw	29, UC_GREG(29)(3)
	stw	30, UC_GREG(30)(3)
	stw	31, UC_GREG(31)(3)

	/* NIP and LINK both become the return address: resuming this
	   context continues at the instruction after the call, and
	   common_init_ppc32() takes the initial IP from LINK.  */
	mflr	0
	stw	0, UC_GREG(UC_NIP_IDX)(3)
	stw	0, UC_GREG(UC_LINK_IDX)(3)

	mfcr	0
	stw	0, UC_GREG(UC_CCR_IDX)(3)
	mfxer	0
	stw	0, UC_GREG(UC_XER_IDX)(3)
	mfctr	0
	stw	0, UC_GREG(UC_CTR_IDX)(3)

	/* MSR is not meaningful outside a signal frame; zero it rather
	   than leave the caller's stack contents behind.  */
	li	0, 0
	stw	0, UC_GREG(UC_MSR_IDX)(3)

	/* ppc32_local_resume() hands the whole context to setcontext(),
	   and the signal-frame path copies uc_mcontext into a kernel
	   sigframe, so nothing may be left uninitialised.  */
	stw	0, UC_FLAGS_OFF(3)
	stw	0, UC_LINK_OFF(3)

	/* uc_stack, sized from the macro so that a wider stack_t cannot
	   leave a tail behind.  */
	li	11, UC_STACK_SIZE / 4
	addi	9, 3, UC_STACK_OFF
	mtctr	11
3:	stw	0, 0(9)
	addi	9, 9, 4
	bdnz	3b

#if !defined(_SOFT_FLOAT) && !defined(__NO_FPRS__)
	stfd	0, UC_FPREG(0)(3)
	stfd	1, UC_FPREG(1)(3)
	stfd	2, UC_FPREG(2)(3)
	stfd	3, UC_FPREG(3)(3)
	stfd	4, UC_FPREG(4)(3)
	stfd	5, UC_FPREG(5)(3)
	stfd	6, UC_FPREG(6)(3)
	stfd	7, UC_FPREG(7)(3)
	stfd	8, UC_FPREG(8)(3)
	stfd	9, UC_FPREG(9)(3)
	stfd	10, UC_FPREG(10)(3)
	stfd	11, UC_FPREG(11)(3)
	stfd	12, UC_FPREG(12)(3)
	stfd	13, UC_FPREG(13)(3)
	stfd	14, UC_FPREG(14)(3)
	stfd	15, UC_FPREG(15)(3)
	stfd	16, UC_FPREG(16)(3)
	stfd	17, UC_FPREG(17)(3)
	stfd	18, UC_FPREG(18)(3)
	stfd	19, UC_FPREG(19)(3)
	stfd	20, UC_FPREG(20)(3)
	stfd	21, UC_FPREG(21)(3)
	stfd	22, UC_FPREG(22)(3)
	stfd	23, UC_FPREG(23)(3)
	stfd	24, UC_FPREG(24)(3)
	stfd	25, UC_FPREG(25)(3)
	stfd	26, UC_FPREG(26)(3)
	stfd	27, UC_FPREG(27)(3)
	stfd	28, UC_FPREG(28)(3)
	stfd	29, UC_FPREG(29)(3)
	stfd	30, UC_FPREG(30)(3)
	stfd	31, UC_FPREG(31)(3)
#else
	/* e500 and other soft-float cores have no FPRs to save; clear the
	   area so the sigframe copy does not pick up stack garbage.  */
	li	11, 32
	addi	9, 3, UC_FPREGS_OFF
	mtctr	11
1:	stw	0, 0(9)
	stw	0, 4(9)
	addi	9, 9, UC_FPREG_SIZE
	bdnz	1b
#endif

	/* The kernel points uc_regs at the embedded mcontext; uc_addr()
	   dereferences it, so it has to be set here too.  */
	addi	0, 3, UC_MCONTEXT_OFF
	stw	0, UC_REGS_PTR_OFF(3)

	/* uc_sigmask has to hold the current mask: ppc32_local_resume()
	   feeds it straight to sigprocmask().  glibc's getcontext() does
	   the same syscall.  r31 is scratch here because its incoming
	   value is already stored above; it is reloaded before returning.  */
	/* The kernel fills only the first UC_KERNEL_SIGSET_SIZE bytes of
	   uc_sigmask; clear the whole field first so the tail is defined.  */
	addi	9, 3, UC_SIGMASK_OFF
	li	11, UC_SIGMASK_SIZE / 4
	mtctr	11
2:	stw	0, 0(9)
	addi	9, 9, 4
	bdnz	2b

	mr	31, 3
	addi	5, 31, UC_SIGMASK_OFF
	li	3, 0			/* SIG_BLOCK */
	li	4, 0			/* set == NULL */
	li	6, UC_KERNEL_SIGSET_SIZE
	li	0, __NR_rt_sigprocmask
	sc

	lwz	0, UC_GREG(UC_LINK_IDX)(31)
	mtlr	0
	lwz	31, UC_GREG(31)(31)

	li	3, 0
	blr

	.size _Uppc32_getcontext,.-_Uppc32_getcontext

#endif /* __linux__ && !__GLIBC__ */

	/* Non-executable stack.  */
#if defined(__linux__) && defined(__ELF__)
	.section .note.GNU-stack,"",@progbits
#endif
