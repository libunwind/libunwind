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

#define LOAD_VAL(reg)				\
	ld8 reg = [in1], 8;;			\
	tbit.nz p6, p0 = reg, 0;;		\
(p6)	ld8.s reg = [r0]


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
	LOAD_VAL(r4)
	LOAD_VAL(r5)
	LOAD_VAL(r6)
	LOAD_VAL(r7)
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

	.global save_static_to_fr
	.proc save_static_to_fr
save_static_to_fr:
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
	.restore sp
	add sp = 16, sp;;
	ldf.fill f2 = [sp]		// restore r2

	mov ar.pfs = loc0
	mov rp = loc1
	br.ret.sptk.many rp
	.endp save_static_to_fr

	/* If r4 is not a NaT, save b3 to a stacked register and
	   then save r4 in b3.  The non-NaTness of r4 is saved in
	   p1.  */

	.global save_static_to_br
	.proc save_static_to_br
save_static_to_br:
	.prologue
	.regstk 2, 5, 2, 0
	.save ar.pfs, loc0
	alloc loc0 = ar.pfs, 2, 5, 2, 0
	.save rp, loc1
	mov loc1 = rp

	.save pr, loc2
	mov loc2 = pr			// save predicates

	.spillreg b3, loc3
	mov loc3 = b3

	tnat.z p1, p2 = r4;;
	.spillreg.p p1, r4, b3
(p1)	mov b3 = r4

	.body

	LOAD_VAL(r4)
	CALL_NEXT(loc4)

(p1)	mov r4 = b3			// restore r4

	mov ar.pfs = loc0
	mov rp = loc1
	mov pr = loc2, -1
	mov b3 = loc3			// restore b3
	br.ret.sptk.many rp
	.endp save_static_to_br

	/* Spill r4 into memory and then save r5 in r4.  */

	.global save_static_to_mem
	.proc save_static_to_mem
save_static_to_mem:
	.prologue
	.regstk 2, 4, 2, 0
	.save ar.pfs, loc0
	alloc loc0 = ar.pfs, 2, 4, 2, 0
	.save rp, loc1
	mov loc1 = rp
	.save ar.unat, loc2
	mov loc2 = ar.unat

	.fframe 16
	.spillpsp r4, 16
	st8.spill [sp] = r4, -16

	.spillreg r5, r4
	mov r4 = r5

	.body

	LOAD_VAL(r5)
	CALL_NEXT(loc3)

	mov r5 = r4			// restore r5
	.restore sp
	add sp = 16, sp;;
	ld8.fill r4 = [sp]		// restore r4

	mov ar.pfs = loc0
	mov rp = loc1
	mov ar.unat = loc2		// restore ar.unat
	br.ret.sptk.many rp

	.endp save_static_to_mem
