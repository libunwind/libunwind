/* libunwind - a platform-independent unwind library
   Copyright (C) 2003 Hewlett-Packard Co
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

#define SIG_BLOCK 0

	.align 32

	.global __sigsetjmp
	.global sigprocmask

	.proc __sigsetjmp

__sigsetjmp:
	.prologue
	.save ar.pfs, r35
	alloc loc1 = ar.pfs, 2, 3, 3, 0
	add r16 = 16, in0
	add out2 = 24, in0

	st8 [in0] = sp, 8		// sigjmp_buf[0] = sp
	mov out0 = SIG_BLOCK
	.save rp, loc0
	mov loc0 = rp
	.body
	;;
	st8 [in0] = loc0, 24		// sigjmp_buf[1] = rp
	st8 [r16] = in1			// sigjmp_buf[2] = savemask
	cmp.ne p6, p0 = in1, r0

	mov out1 = r0
	mov loc2 = ar.bsp
(p6)	br.call.sptk.many rp = sigprocmask	// sigjmp_buf[3] = sigmask
	;;

	st8 [in0] = loc2		// sigjmp_buf[4] = bsp
	mov rp = loc0
	nop 0

	mov r8 = 0
	mov.i ar.pfs = loc1
	br.ret.sptk.many rp

	.endp __sigsetjmp
