/* libunwind - a platform-independent unwind library
   Copyright (C) 2004 Hewlett-Packard Co
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

	.text

#define CALL_NEXT(gp_save_reg)						\
	ld8 r2 = [in0], 8;;	/* read the next function pointer */	\
	ld8 r3 = [r2], 8;;	/* read the function's entry-point */	\
	ld8 r2 = [r2];;		/* read the function's gp */		\
	mov b6 = r3;							\
	mov gp_save_reg = gp;						\
	mov out0 = in0;							\
	mov out1 = in1;							\
	mov gp = r2;							\
	br.call.sptk.many rp = b6;;					\
	mov gp = gp_save_reg


	/* Save r4-r7 into stacked registers, load them up with the
	   values passed via the pointer in in1 and then call the
	   function passed via the pointer in in0.  */

	.global save_static_to_stacked
	.proc save_static_to_stacked
save_static_to_stacked:
	.prologue
	.regstk 2, 7, 2, 0
	.save ar.pfs, loc0
	alloc loc0 = ar.pfs, 2, 7, 2, 0
	.save rp, loc1
	mov loc1 = rp
	.spillreg r4, loc2
	mov loc2 = r4
	.spillreg r5, loc3
	mov loc3 = r5
	.spillreg r6, loc4
	mov loc4 = r6
	.spillreg r7, loc5
	mov loc5 = r7
	.body
	ld8 r4 = [in1], 8;;
	ld8 r5 = [in1], 8;;
	ld8 r6 = [in1], 8;;
	ld8 r7 = [in1], 8;;
	tbit.nz p6, p0 = r4, 0
	tbit.nz p7, p0 = r5, 0
	tbit.nz p8, p0 = r6, 0
	tbit.nz p9, p0 = r7, 0;;
(p6)	ld8.s r4 = [r0]
(p7)	ld8.s r5 = [r0]
(p8)	ld8.s r6 = [r0]
(p9)	ld8.s r7 = [r0]

	CALL_NEXT(loc6)

	mov r4 = loc2
	mov r5 = loc3
	mov r6 = loc4
	mov r7 = loc5

	mov ar.pfs = loc0
	mov rp = loc1
	br.ret.sptk.many rp
	.endp save_static_to_stacked

	/* Save f2 to the memory stack, save r4 to f2, then load
	   r4 with the value passed via in1 and call the function
	   passed via in0.  */

	.global save_static_to_fpreg
	.proc save_static_to_fpreg
save_static_to_fpreg:
	.prologue
	.regstk 2, 3, 2, 0
	.save ar.pfs, loc0
	alloc loc0 = ar.pfs, 2, 3, 2, 0
	.save rp, loc1
	mov loc1 = rp
	.fframe 16
	.spillpsp f2, 16
	stf.spill [sp] = f2, -16
	.spillreg r4, f2
	setf.sig f2 = r4

	.body

	ld8 r4 = [in1], 8;;
	tbit.nz p6, p0 = r4, 0;;
(p6)	ld8.s r4 = [r0]

	CALL_NEXT(loc2)

	getf.sig r4 = f2		// restore r4
	ldf.fill f2 = [sp], 16		// restore r2

	mov ar.pfs = loc0
	mov rp = loc1
	br.ret.sptk.many rp
	.endp save_static_to_fpreg
