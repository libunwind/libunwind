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

/* glibc has a getcontext() that unw_getcontext() uses instead.  */
#if defined(__linux__) && !defined(__GLIBC__)

/*  int _Uppc64_getcontext (ucontext_t *ucp)

   Save the caller's machine state in *UCP, as getcontext() does, so that
   unwinding starts in the caller and resuming the context returns from
   this call.

   The state is captured by the kernel with swapcontext(UCP, NULL), so
   that it includes what user space cannot read itself, in particular
   the MSR, whose endianness bit setcontext() implementations that go
   through the kernel restore, and the FP, VMX and VSX state.  The
   syscall preserves the non-volatile registers, and this function does
   not set up a stack frame or a TOC pointer, so the saved r1, r2,
   r14-r31, f14-f31, v20-v31 and CR fields are the caller's.  Only the
   saved PC refers to this function: replace it with the return address.

   The pointer and the return address are kept in the protected zone
   below the stack pointer, which the kernel leaves alone, so the
   function needs neither a frame nor a non-volatile register.  */

#define FUNC _Uppc64_getcontext

#if defined(_CALL_ELF) && _CALL_ELF == 2
	.abiversion 2
	.text
	.globl	FUNC
	.type	FUNC, @function
	.p2align 4
FUNC:
#else
	/* ELFv1: the symbol names a function descriptor.  */
	.section ".opd", "aw"
	.p2align 3
	.globl	FUNC
	.type	FUNC, @function
FUNC:
	.quad	.L.FUNC, .TOC.@tocbase, 0
	.text
	.p2align 4
.L.FUNC:
#endif
	.cfi_startproc
	mflr	0
	std	3, -8(1)		/* UCP */
	std	0, -16(1)		/* return address */
	li	4, 0			/* no new context */
	li	5, PPC64_UC_SIZE
	li	0, 249			/* __NR_swapcontext */
	sc
	ld	4, -8(1)
	ld	0, -16(1)
	mtlr	0
	bso-	1f			/* the syscall failed */
	std	0, PPC64_UC_NIP(4)	/* resume after this call */
	std	0, PPC64_UC_LINK(4)
	li	3, 0
	blr
1:	li	3, -1
	blr
	.cfi_endproc
#if defined(_CALL_ELF) && _CALL_ELF == 2
	.size	FUNC, . - FUNC
#else
	.size	FUNC, 24
#endif

#endif /* __linux__ && !__GLIBC__ */

	/* We do not need executable stack.  */
	.section	.note.GNU-stack,"",@progbits
