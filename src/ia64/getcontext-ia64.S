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

#define GR(n)   (SC_GR + (n)*8)
#define BR(n)   (SC_BR + (n)*8)
#define FR(n)   (SC_FR + (n)*16)

/* This should be compatible to the libc's getcontext(), except that
   the sc->sc_mask field is always cleared and that the name is
   prefixed with _Uia64_ so we don't step on the application's
   name-space.  */

	.align 32
	.protected _Uia64_getcontext
	.global _Uia64_getcontext
	.proc _Uia64_getcontext
_Uia64_getcontext:
#if 0
	add r16 = 0x000, r32
	add r17 = 0x080, r32
	add r18 = 0x100, r32
	add r19 = 0x180, r32
	add r20 = 0x200, r32
	add r21 = 0x280, r32
	add r22 = 0x300, r32
	add r23 = 0x380, r32
	;;
	ld8 r16 = [r16]
	ld8 r17 = [r17]
	ld8 r18 = [r18]
	ld8 r19 = [r19]
	ld8 r20 = [r20]
	ld8 r21 = [r21]
	ld8 r22 = [r22]
	ld8 r23 = [r23]
	;;
	add r16 = r16, r17
	add r18 = r18, r19
	add r20 = r20, r21
	add r22 = r22, r23
	;;
	add r16 = r16, r18
	add r20 = r20, r22
	;;
	add r8 = r16, r20
	br.ret.sptk.many rp
#elif 0
	add r2 = GR(1), r32;;	st8 [r2] = r0
	add r2 = SC_FLAGS, r32;;st8 [r2] = r0
	add r2 = SC_PR, r32;;	st8 [r2] = r0
	add r2 = GR(12), r32;;	st8 [r2] = r0
	add r2 = FR(2), r32;;	stf.spill [r2] = f0
	add r2 = FR(16), r32;;	stf.spill [r2] = f0
	add r2 = FR(24), r32;;	stf.spill [r2] = f0
	add r2 = FR(31), r32;;	stf.spill [r2] = f0

	add r2 = GR(4), r32;;	st8 [r2] = r0
	add r2 = GR(5), r32;;	st8 [r2] = r0
	add r2 = GR(7), r32;;	st8 [r2] = r0
	add r2 = GR(6), r32;;	st8 [r2] = r0

	add r2 = BR(0), r32;;	st8 [r2] = r0		//  2 cycles
	add r2 = BR(1), r32;;	st8 [r2] = r0		//  2 cycles
	add r2 = BR(2), r32;;	st8 [r2] = r0		//  2 cycles
	add r2 = BR(4), r32;;	st8 [r2] = r0		//  2 cycles
	add r2 = BR(3), r32;;	st8 [r2] = r0		//  2 cycles
	add r2 = BR(5), r32;;	st8 [r2] = r0		//  2 cycles

	add r2 = SC_PFS, r32;;	st8 [r2] = r0		//  2 cycles
	add r2 = SC_LC, r32;;	st8 [r2] = r0		//  2 cycles
	add r2 = SC_UNAT, r32;;	st8 [r2] = r0		//  5 cycles
	add r2 = SC_BSP, r32;;	st8 [r2] = r0		// 12 cycles

	add r2 = SC_FPSR, r32;;	st8 [r2] = r0		// 12 cycles
	add r2 = FR(3), r32;;	stf.spill [r2] = f0

	add r2 = FR(4), r32;;	stf.spill [r2] = f0
	add r2 = FR(5), r32;;	stf.spill [r2] = f0

	add r2 = SC_RNAT, r32;;	st8 [r2] = r0		//  5 cycles

	add r2 = FR(17), r32;;	stf.spill [r2] = f0
	add r2 = FR(18), r32;;	stf.spill [r2] = f0
	add r2 = FR(19), r32;;	stf.spill [r2] = f0
	add r2 = FR(20), r32;;	stf.spill [r2] = f0
	add r2 = FR(21), r32;;	stf.spill [r2] = f0

	add r2 = SC_NAT, r32;;	st8 [r2] = r0		// last GR spill + 9 cycles

	add r2 = FR(22), r32;;	stf.spill [r2] = f0
	add r2 = FR(23), r32;;	stf.spill [r2] = f0
	add r2 = FR(25), r32;;	stf.spill [r2] = f0
	add r2 = FR(26), r32;;	stf.spill [r2] = f0
	add r2 = FR(27), r32;;	stf.spill [r2] = f0
	add r2 = FR(28), r32;;	stf.spill [r2] = f0
	add r2 = FR(29), r32;;	stf.spill [r2] = f0
	add r2 = FR(30), r32;;	stf.spill [r2] = f0

	br.ret.sptk.many rp
#elif 1
	.prologue
	alloc rPFS = ar.pfs, 1, 0, 0, 0				// M2
	mov rPR = pr						// I0, 2 cycles
	add r2 = GR(1), in0					// I1
	;;

	.save ar.unat, rUNAT
	mov.m rUNAT = ar.unat					// M2, 5 cycles
	.body
	st8.spill [r2] = r1, (SC_FLAGS - GR(1))			// M3
	dep.z rFLAGS = -1, IA64_SC_FLAG_SYNCHRONOUS_BIT, 1	// I0, 1 cycle
	;;

	mov.m rRSC = ar.rsc					// M2, 12 cycles
	st8 [r2] = rFLAGS, (SC_PR  - SC_FLAGS)			// M3
	add r3 = FR(2), in0
	;;

	mov.m rBSP = ar.bsp					// M2, 12 cycles
	st8 [r2] = rPR, (GR(12) - SC_PR)			// M3
	add r8 = FR(16), in0
	;;

	mov.m rFPSR = ar.fpsr					// M2, 12 cycles
	st8.spill [r2] = r12, (GR(4) - GR(12))			// M3
	add r9 = FR(24), in0
	;;

	stf.spill [r3] = f2					// M2
	stf.spill [r8] = f16					// M3
	add r3 = GR(7), in0
	;;

	flushrs							// M0
	stf.spill [r9] = f24, (FR(31) - FR(24))			// M2
	mov rB0 = b0						// I0, 2 cycles
	;;

	stf.spill [r9] = f31					// M2
	st8.spill [r2] = r4, (GR(5) - GR(4))			// M3, bank 1
	mov rB1 = b1						// I0, 2 cycles
	;;

.mem.offset 0,0; st8.spill [r2] = r5, (GR(6) - GR(5))		// M4, bank 0
.mem.offset 8,0; st8.spill [r3] = r7, (BR(0) - GR(7))		// M3, bank 0
	mov rB2 = b2						// I0, 2 cycles
	;;

	st8.spill [r2] = r6, (BR(1) - GR(6))			// M2, bank 1
	st8 [r3] = rB0, (BR(4) - BR(0))				// M3, bank 1
	mov rB4 = b4						// I0, 2 cycles
	;;

	mov.m rNAT = ar.unat					// M2, 5 cycles
	st8 [r2] = rB1, (BR(2) - BR(1))				// M3, bank 0
	mov rB3 = b3
	;;

	st8 [r2] = rB2, (BR(3) - BR(2))				// M2, bank 1
	st8 [r3] = rB4, (SC_LC - BR(4))				// M3, bank 1
	mov rB5 = b5						// I0, 2 cycles
	;;

	and rTMP = ~0x3, rRSC					// M0
	add rPOS = GR(0), in0	// rPOS <- &sc_gr[0]		// M1
	mov.i rLC = ar.lc					// I0, 2 cycles
	;;

	mov.m ar.rsc = rTMP	// put RSE into lazy mode	// M2, ?? cycles
	st8 [r2] = rB3, (BR(5) - BR(3))				// M3, bank 0
	extr.u rPOS = rPOS, 3, 6 // get NaT bitnr for r0	// I0
	;;

	mov.m rRNAT = ar.rnat					// M2, 5 cycles
	st8 [r2] = rB5, (SC_PFS - BR(5))			// M3, bank 0
	sub rCPOS = 64, rPOS					// I0
	;;

	st8 [r2] = rPFS, (SC_UNAT - SC_PFS)			// M2
	st8 [r3] = rLC, (SC_BSP - SC_LC)			// M3
	shr.u rTMP = rNAT, rPOS					// I0, 3 cycles
	;;

	st8 [r2] = rUNAT, (SC_FPSR - SC_UNAT)			// M2
	st8 [r3] = rBSP						// M3
	add r8 = FR(3), in0
	;;

	st8 [r2] = rFPSR, (SC_RNAT - SC_FPSR)			// M2
	stf.spill [r8] = f3, (FR(4) - FR(3))			// M3
	add r9 = FR(5), in0
	;;

	stf.spill [r8] = f4, (FR(17) - FR(4))			// M2
	stf.spill [r9] = f5, (FR(19) - FR(5))			// M3
	shl rNAT = rNAT, rCPOS					// I0, 3 cycles
	;;

	st8 [r2] = rRNAT, (SC_NAT - SC_RNAT)			// M2
	stf.spill [r8] = f17, (FR(18) - FR(17))			// M3
	nop.i 0
	;;

	stf.spill [r8] = f18, (FR(20) - FR(18))			// M2
	stf.spill [r9] = f19, (FR(21) - FR(19))			// M3
	nop.i 0
	;;

	stf.spill [r8] = f20, (FR(22) - FR(20))			// M2
	stf.spill [r9] = f21, (FR(23) - FR(21))			// M3
	or rNAT = rNAT, rTMP					// I0
	;;

	st8 [r2] = rNAT						// M2
	stf.spill [r8] = f22, (FR(25) - FR(22))			// M3
	;;
	stf.spill [r9] = f23, (FR(26) - FR(23))			// M2
	stf.spill [r8] = f25, (FR(27) - FR(25))			// M3
	;;
	stf.spill [r9] = f26, (FR(28) - FR(26))			// M2
	stf.spill [r8] = f27, (FR(29) - FR(27))			// M3
	;;
	mov.m ar.rsc = rRSC	// restore RSE mode		// M2
	stf.spill [r9] = f28, (FR(30) - FR(28))			// M3
	;;
	mov.m ar.unat = rUNAT	// restore caller's UNaT	// M2
	stf.spill [r8] = f29					// M3
	;;
	stf.spill [r9] = f30					// M2
	mov r8 = 0
	br.ret.sptk.many rp
#elif 0
	.prologue
	alloc rPFS = ar.pfs, 1, 0, 0, 0
	add r3 = SC_MASK, r32
	;;
	st8 [r3] = r0			// clear sc->sc_mask

	flushrs				// save dirty partition on rbs

	mov.m rFPSR = ar.fpsr
	mov.m rRSC = ar.rsc
	add r2 = SC_GR+1*8, r32
	;;
	mov.m rBSP = ar.bsp
	.save ar.unat, rUNAT
	mov.m rUNAT = ar.unat
	.body
	add r3 = SC_GR+4*8, r32
	;;

.mem.offset 0,0; st8.spill [r2] = r1, (5*8 - 1*8)
.mem.offset 8,0; st8.spill [r3] = r4, 16
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
#endif

	.endp _Uia64_getcontext
