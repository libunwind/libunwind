/* libunwind - a platform-independent unwind library
   Copyright (C) 2026 Ali Ahmet Memiş <aliamemis@disroot.org>

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

/* ucontext_t stores r0-r31 followed by the PC.  The PC is the return
   address.  */

#define REG(X) (LINUX_UC_MCONTEXT_OFF + 4 * (X))

	.text
	.globl _Uor1k_getcontext
	.type _Uor1k_getcontext, @function
_Uor1k_getcontext:
	l.sw	REG(0)(r3), r0
	l.sw	REG(1)(r3), r1
	l.sw	REG(2)(r3), r2
	l.sw	REG(3)(r3), r3
	l.sw	REG(4)(r3), r4
	l.sw	REG(5)(r3), r5
	l.sw	REG(6)(r3), r6
	l.sw	REG(7)(r3), r7
	l.sw	REG(8)(r3), r8
	l.sw	REG(9)(r3), r9
	l.sw	REG(10)(r3), r10
	l.sw	REG(11)(r3), r11
	l.sw	REG(12)(r3), r12
	l.sw	REG(13)(r3), r13
	l.sw	REG(14)(r3), r14
	l.sw	REG(15)(r3), r15
	l.sw	REG(16)(r3), r16
	l.sw	REG(17)(r3), r17
	l.sw	REG(18)(r3), r18
	l.sw	REG(19)(r3), r19
	l.sw	REG(20)(r3), r20
	l.sw	REG(21)(r3), r21
	l.sw	REG(22)(r3), r22
	l.sw	REG(23)(r3), r23
	l.sw	REG(24)(r3), r24
	l.sw	REG(25)(r3), r25
	l.sw	REG(26)(r3), r26
	l.sw	REG(27)(r3), r27
	l.sw	REG(28)(r3), r28
	l.sw	REG(29)(r3), r29
	l.sw	REG(30)(r3), r30
	l.sw	REG(31)(r3), r31
	l.sw	REG(32)(r3), r9
	l.ori	r11, r0, 0
	l.jr	r9
	 l.nop
	.size _Uor1k_getcontext, . - _Uor1k_getcontext

	.section .note.GNU-stack,"",@progbits
