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
	.body
	alloc r11 = ar.pfs, 1, 0, 0, 0
	flushrs				// save dirty partition on rbs
	add r3 = SC_MASK, r32
	;;
	st8 [r3] = r0			// clear sc->sc_mask

	mov.m rFPSR = ar.fpsr
	mov.m rRSC = ar.rsc
	add r2 = SC_GR+1*8, r32
	;;
	mov.m rBSP = ar.bsp
	.prologue
	.save ar.unat, rUNAT
	mov.m rUNAT = ar.unat
	.body
	add r3 = SC_GR+4*8, r32
	;;

.mem.offset 0,0; st8.spill [r2] = r1, (5*8 - 1*8)
.mem.offset 8,0; st8.spill [r3] = r4, 16
	mov rPFS = r11
	;;
.mem.offset 0,0; st8.spill [r2] = r5, 16
.mem.offset 8,0; st8.spill [r3] = r6, 48
	and rTMP = ~0x3, rRSC
	;;
.mem.offset 0,0; st8.spill [r2] = r7, (SC_FR+2*16-(SC_GR+7*8))
.mem.offset 8,0; st8.spill [r3] = sp, (SC_FR+3*16-(SC_GR+12*8))
	;;
	mov.m ar.rsc = rTMP 		// put RSE into enforced lazy mode
	mov.m rNAT = ar.unat
	mov.i rLC = ar.lc
	;;
	mov.m rRNAT = ar.rnat
	mov.m ar.rsc = rRSC 		// restore RSE mode
	mov rPR = pr

 	/*
	 * Rotate NaT bits by rPOS positions to the right:
	 */
	stf.spill [r2] = f2, 32
	stf.spill [r3] = f3, 32
	add rPOS = SC_GR, r32			// rPOS <- &sc_gr[0]
	;;
	stf.spill [r2] = f4, (16*16-4*16)
	stf.spill [r3] = f5, (17*16-5*16)
	extr.u rPOS = rPOS, 3, 6		// get NaT bit number for r0
	;;
	stf.spill [r2] = f16, 32
	stf.spill [r3] = f17, 32
	sub rCPOS = 64, rPOS
	;;
	stf.spill [r2] = f18, 32
	stf.spill [r3] = f19, 32
	shr.u rTMP = rNAT, rPOS
	;;
	stf.spill [r2] = f20, 32
	stf.spill [r3] = f21, 32
	shl rNAT = rNAT, rCPOS
	;;
	stf.spill [r2] = f22, 32
	stf.spill [r3] = f23, 32
	or rNAT = rNAT, rTMP
	;;
	stf.spill [r2] = f24, 32
	stf.spill [r3] = f25, 32
	mov r8 = 0
	;;
	stf.spill [r2] = f26, 32
	stf.spill [r3] = f27, 32
	mov r9 = 1
	;;
	stf.spill [r2] = f28, 32
	stf.spill [r3] = f29, 32
	mov rB0 = b0
	;;
	stf.spill [r2] = f30, 32
	stf.spill [r3] = f31, 32
	mov rB1 = b1
	;;
	mov ar.unat = rUNAT // done with integer regs; restore caller's UNaT
	add r2 = SC_NAT, r32
	add r3 = SC_BSP, r32
	;;
	st8 [r2] = rNAT, (SC_RNAT-SC_NAT)
	st8 [r3] = rBSP, (SC_UNAT-SC_BSP)
	mov rB2 = b2
	;;
	st8 [r2] = rRNAT, (SC_FPSR-SC_RNAT)
	st8 [r3] = rUNAT, (SC_PFS-SC_UNAT)
	mov rB3 = b3
	;;
	st8 [r2] = rFPSR, (SC_LC-SC_FPSR)
	st8 [r3] = rPFS, (SC_PR-SC_PFS)
	mov rB4 = b4
	;;
	st8 [r2] = rLC, (SC_BR+0*8-SC_LC)
	st8 [r3] = rPR, (SC_BR+1*8-SC_PR)
	mov rB5 = b5
	;;
	st8 [r2] = rB0, 16
	st8 [r3] = rB1, 16
	;;
	st8 [r2] = rB2, 16
	st8 [r3] = rB3, 16
	;;
	st8 [r2] = rB4
	st8 [r3] = rB5

	br.ret.sptk.many rp

	.endp _Uia64_getcontext
