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

#include "ucontext_i.h"

#define GR(n)	(SC_GR + (n)*8)
#define BR(n)	(SC_BR + (n)*8)
#define FR(n)	(SC_FR + (n)*16)

/* This should be compatible to the libc's getcontext(), except that
   the sc->sc_mask field is always cleared and that the name is
   prefixed with _Uia64_ so we don't step on the application's
   name-space.  */

	.align 32
	.protected _Uia64_getcontext
	.global _Uia64_getcontext
	.proc _Uia64_getcontext
_Uia64_getcontext:
	.prologue
	alloc rPFS = ar.pfs, 1, 0, 0, 0
	flushrs				// save dirty partition on rbs
	add r2 = SC_MASK, r32
	;;
	st8 [r2] = r0			// clear sc->sc_mask
	mov.m rFPSR = ar.fpsr
	add r2 = GR(1), r32
	;;
	mov.m rRSC = ar.rsc
	mov.m rBSP = ar.bsp
	mov rPR = pr

	.save ar.unat, rUNAT
	mov.m rUNAT = ar.unat
	.body
	st8.spill [r2] = r1, (GR(12) - GR(1))
	add r3 = SC_NAT, r32
	;;
	st8.spill [r2] = sp, (SC_PR - GR(12))
	lfetch.fault.nt1 [r3]		// prefetch nat...ar.lc
	adds r3 = FR(2), r32
	;;
	st8 [r2] = rPR
	stf.spill [r3] = f2, (FR(16) - FR(2))
	add r2 = FR(24), r32
	;;
	stf.spill [r2] = f24, (FR(31) - FR(24))
	stf.spill [r3] = f16
	add r3 = GR(4), r32
	;;
	stf.spill [r2] = f31
	st8.spill [r3] = r4, (GR(6) - GR(4))
	add r2 = GR(5), r32
	;;
.mem.offset 0,0; st8.spill [r2] = r5, (GR(7) - GR(5))
.mem.offset 8,0; st8.spill [r3] = r6
	and rTMP = ~0x3, rRSC
	;;
	st8.spill [r2] = r7
	mov.m ar.rsc = rTMP 		// put RSE into enforced lazy mode
	mov.i rLC = ar.lc
	;;
	mov.m rNAT = ar.unat
	mov.m rRNAT = ar.rnat
	add r2 = FR(3), r32
	;;
	mov.m ar.rsc = rRSC 		// restore RSE mode
	stf.spill [r2] = f3, (FR(4) - FR(3))
	add r3 = FR(5), r32
	;;
	stf.spill [r2] = f4, (FR(17) - FR(4))
	stf.spill [r3] = f5, (FR(18) - FR(5))
 	/*
	 * Rotate NaT bits by rPOS positions to the right:
	 */
	add rPOS = GR(0), r32			// rPOS <- &sc_gr[0]
	;;
	stf.spill [r2] = f17, (FR(19) - FR(17))
	stf.spill [r3] = f18, (FR(20) - FR(18))
	extr.u rPOS = rPOS, 3, 6		// get NaT bit number for r0
	;;
	stf.spill [r2] = f19, (FR(21) - FR(19))
	stf.spill [r3] = f20, (FR(22) - FR(20))
	sub rCPOS = 64, rPOS
	;;
	stf.spill [r2] = f21, (FR(23) - FR(21))
	stf.spill [r3] = f22, (FR(25) - FR(22))
	shr.u rTMP = rNAT, rPOS
	;;
	stf.spill [r2] = f23, (FR(26) - FR(23))
	stf.spill [r3] = f25, (FR(27) - FR(25))
	shl rNAT = rNAT, rCPOS
	;;
	stf.spill [r2] = f26, (FR(28) - FR(26))
	stf.spill [r3] = f27, (FR(29) - FR(27))
	or rNAT = rNAT, rTMP
	;;
	stf.spill [r2] = f28, (FR(30) - FR(28))
	stf.spill [r3] = f29
	mov rB0 = b0
	;;
	stf.spill [r2] = f30
	mov ar.unat = rUNAT // done with integer regs; restore caller's UNaT
	mov rB1 = b1
	;;
	add r2 = SC_NAT, r32
	add r3 = SC_BSP, r32
	mov rB2 = b2
	;;
	st8 [r2] = rNAT, (SC_RNAT - SC_NAT)
	st8 [r3] = rBSP, (SC_UNAT - SC_BSP)
	mov rB3 = b3
	;;
	st8 [r2] = rRNAT, (SC_FPSR - SC_RNAT)
	st8 [r3] = rUNAT, (SC_PFS - SC_UNAT)
	mov rB4 = b4
	;;
	st8 [r2] = rFPSR, (SC_LC - SC_FPSR)
	st8 [r3] = rPFS, (BR(0) - SC_PFS)
	mov rB5 = b5
	;;
	st8 [r2] = rLC, (BR(1) - SC_LC)
	st8 [r3] = rB0, (BR(2) - BR(0))
	;;
	st8 [r2] = rB1, (BR(3) - BR(1))
	st8 [r3] = rB2, (BR(4) - BR(2))
	;;
	st8 [r2] = rB3, (BR(5) - BR(3))
	st8 [r3] = rB4
	;;
	st8 [r2] = rB5
	mov r8 = 0
	br.ret.sptk.many rp

	.endp _Uia64_getcontext
