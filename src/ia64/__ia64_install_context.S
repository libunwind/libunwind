/* libunwind - a platform-independent unwind library
   Copyright (C) 2001-2002 Hewlett-Packard Co
	Contributed by David Mosberger-Tang <davidm@hpl.hp.com>

This file is part of libunwind.

libunwind is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2, or (at your option)
any later version.

libunwind is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

As a special exception, if you link this library with other files to
produce an executable, this library does not by itself cause the
resulting executable to be covered by the GNU General Public License.
This exception does not however invalidate any other reasons why the
executable file might be covered by the GNU General Public
License.  */

#include "ucontext_i.h"

/*  __ia64_install_context (const ucontext_t *ucp,
			    long r15, long r16, long r17, long r18)

  Restores the machine context in UCP and thereby resumes execution
  in that context.

  This implementation in intended to be used for *synchronous* context
  switches only.  Therefore, it does not have to restore anything
  other than the PRESERVED state.  */

	.global __ia64_install_context
	.proc __ia64_install_context
__ia64_install_context:
	alloc r16 = ar.pfs, 5, 0, 0, 0
	invala
	add r2 = SC_NAT, r32
	;;
	ld8 rNAT = [r2], (SC_BSP-SC_NAT)
	add r3 = SC_RNAT, r32			// r3 <- &sc_ar_rnat
	add rPOS = SC_GR, r32			// rPOS <- &sc_gr[0]
	;;
	ld8 rBSP = [r2], (SC_UNAT-SC_BSP)
	ld8 rRNAT = [r3], (SC_FPSR-SC_RNAT)
	extr.u rPOS = rPOS, 3, 6		// get NaT bit number for r0
	;;
	ld8 rUNAT = [r2], (SC_PFS-SC_UNAT)
	ld8 rFPSR = [r3], (SC_LC-SC_FPSR)
	/*
	 * Rotate NaT bits by rPOS positions to the left:
	 */
	sub rCPOS = 64, rPOS
	;;
	ld8 rPFS = [r2], (SC_PR-SC_PFS)
	ld8 rLC = [r3], (SC_BR+0*8-SC_LC)
	shl rTMP = rNAT, rPOS
	;;
	ld8 rPR = [r2], (SC_BR+1*8-SC_PR)
	ld8 rB0 = [r3], 16
	shr.u rNAT = rNAT, rCPOS
	;;
	ld8 rB1 = [r2], 16
	ld8 rB2 = [r3], 16
	or rNAT = rNAT, rTMP
	;;
	mov.m ar.unat = rNAT
	mov.m rRSC = ar.rsc
	;;
	ld8 rB3 = [r2], 16
	ld8 rB4 = [r3], (SC_GR+1*8-(SC_BR+4*8))
	mov r15 = in1
	;;
	ld8 rB5 = [r2], (SC_GR+4*8-(SC_BR+5*8))
	ld8.fill r1 = [r3], (5*8 - 1*8)
	mov r16 = in2
	;;
	ld8.fill r4 = [r2], 16
	ld8.fill r5 = [r3], 16
	mov b0 = rB0
	;;
	ld8.fill r6 = [r2], 48
	ld8.fill r7 = [r3], (SC_FR+2*16-(SC_GR+7*8))
	mov r17 = in3
	;;
	ld8.fill sp = [r2], (SC_FR+3*16-(SC_GR+12*8))
	mov.m ar.fpsr = rFPSR
	mov.i ar.pfs = rPFS
	;;
	ldf.fill f3 = [r2], 16
	ldf.fill f2 = [r3], 48
	mov b1 = rB1
	;;
	ldf.fill f4 = [r2], (16*16-4*16)
	ldf.fill f5 = [r3], (17*16-5*16)
	mov b2 = rB2
	;;
	ldf.fill f16 = [r2], 32
	ldf.fill f17 = [r3], 32
	mov b3 = rB3
	;;
	ldf.fill f18 = [r2], 32
	ldf.fill f19 = [r3], 32
	mov b4 = rB4
	;;
	ldf.fill f20 = [r2], 32
	ldf.fill f21 = [r3], 32
	mov b5 = rB5
	;;
	ldf.fill f22 = [r2], 32
	ldf.fill f23 = [r3], 32
	mov r8 = 0
	;;
	ldf.fill f24 = [r2], 32
	ldf.fill f25 = [r3], 32
	mov r9 = 0
	;;
	ldf.fill f26 = [r2], 32
	ldf.fill f27 = [r3], 32
	dep rTMP = 0, rRSC, 16, 14	// clear ar.rsc.loadrs
	;;
	ldf.fill f28 = [r2], 32
	ldf.fill f29 = [r3], 32
	and rTMP = ~0x3, rTMP		// clear ar.rsc.mode
	;;
	ldf.fill f30 = [r2], 32
	ldf.fill f31 = [r3], 32
	mov pr = rPR, -1
	;;
	mov.m ar.rsc = rTMP		// put RSE into enforced lazy mode
	;;
	loadrs				// drop dirty partition
	mov r18 = in4
	;;
	mov.m ar.bspstore = rBSP
	mov.m ar.unat = rUNAT
	mov.i ar.lc = rLC
	;;
	mov.m ar.rnat = rRNAT
	mov.m ar.rsc = rRSC
	br.ret.sptk rp

	.endp __ia64_install_context
