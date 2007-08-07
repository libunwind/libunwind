/* libunwind - a platform-independent unwind library
   Copyright (C) 2007 Google, Inc
	Contributed by Arun Sharma <arun.sharma@google.com>

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

#include "offsets.h"

	.global _x86_64_setcontext

_x86_64_setcontext:

        /* restore fp state */
	mov    REG_OFFSET_FPREGS_PTR(%rdi),%r8
	fldenv (%r8)
	ldmxcsr FPREG_OFFSET_MXCR(%r8)

	/* restore the rest of the state */
	mov    REG_OFFSET_R8(%rdi),%r8
	mov    REG_OFFSET_R9(%rdi),%r9
	mov    REG_OFFSET_RBX(%rdi),%rbx
	mov    REG_OFFSET_RBP(%rdi),%rbp
	mov    REG_OFFSET_R12(%rdi),%r12
	mov    REG_OFFSET_R13(%rdi),%r13
	mov    REG_OFFSET_R14(%rdi),%r14
	mov    REG_OFFSET_R15(%rdi),%r15
	mov    REG_OFFSET_RSI(%rdi),%rsi
	mov    REG_OFFSET_RDX(%rdi),%rdx
	mov    REG_OFFSET_RAX(%rdi),%rax
	mov    REG_OFFSET_RCX(%rdi),%rcx
	mov    REG_OFFSET_RSP(%rdi),%rsp

        /* push the return address on the stack */
	mov    REG_OFFSET_RIP(%rdi),%rcx
	push   %rcx

	mov    REG_OFFSET_RCX(%rdi),%rcx
	mov    REG_OFFSET_RDI(%rdi),%rdi
	retq

