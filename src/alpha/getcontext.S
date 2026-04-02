/* libunwind - a platform-independent unwind library
   Copyright (C) 2008 Google, Inc
	Contributed by Paul Pluzhnikov <ppluzhnikov@google.com>

   Modified for Alpha by Matt Turner <mattst88@gmail.com>

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

/*  int _Ualpha_getcontext(unw_tdep_context_t *ucp)

    Saves the machine context in a ucontext_t structure.
    $16 (a0) = pointer to ucontext_t.

    ucontext_t layout (on Alpha Linux):
      offset  0: uc_flags     (8 bytes)
      offset  8: uc_link      (8 bytes)
      offset 16: uc_stack     (32 bytes: ss_sp, ss_flags, ss_size, pad)
      offset 48: uc_mcontext  (= struct sigcontext)
        +0  (48): sc_onstack  (8 bytes)
        +8  (56): sc_mask     (8 bytes)
        +16 (64): sc_pc       (8 bytes)
        +24 (72): sc_ps       (8 bytes)
        +32 (80): sc_regs[32] (256 bytes)
        +288(336): sc_ownedfp (8 bytes)
        +296(344): sc_fpregs[32] (256 bytes)
        +552(600): sc_fpcr    (8 bytes)
*/

/* Offsets from ucontext_t base */
#define UC_MCONTEXT_SC_PC	64
#define UC_MCONTEXT_SC_REGS	80
#define UC_MCONTEXT_SC_FPREGS	344
#define UC_MCONTEXT_SC_FPCR	600

	.global _Ualpha_getcontext
	.type _Ualpha_getcontext, @function
_Ualpha_getcontext:
	.cfi_startproc

	/* Save the return address as PC */
	stq	$26, UC_MCONTEXT_SC_PC($16)

	/* Save integer registers in sc_regs[0..30] */
	stq	$0,  UC_MCONTEXT_SC_REGS+0*8($16)
	stq	$1,  UC_MCONTEXT_SC_REGS+1*8($16)
	stq	$2,  UC_MCONTEXT_SC_REGS+2*8($16)
	stq	$3,  UC_MCONTEXT_SC_REGS+3*8($16)
	stq	$4,  UC_MCONTEXT_SC_REGS+4*8($16)
	stq	$5,  UC_MCONTEXT_SC_REGS+5*8($16)
	stq	$6,  UC_MCONTEXT_SC_REGS+6*8($16)
	stq	$7,  UC_MCONTEXT_SC_REGS+7*8($16)
	stq	$8,  UC_MCONTEXT_SC_REGS+8*8($16)
	stq	$9,  UC_MCONTEXT_SC_REGS+9*8($16)
	stq	$10, UC_MCONTEXT_SC_REGS+10*8($16)
	stq	$11, UC_MCONTEXT_SC_REGS+11*8($16)
	stq	$12, UC_MCONTEXT_SC_REGS+12*8($16)
	stq	$13, UC_MCONTEXT_SC_REGS+13*8($16)
	stq	$14, UC_MCONTEXT_SC_REGS+14*8($16)
	stq	$15, UC_MCONTEXT_SC_REGS+15*8($16)
	stq	$16, UC_MCONTEXT_SC_REGS+16*8($16)
	stq	$17, UC_MCONTEXT_SC_REGS+17*8($16)
	stq	$18, UC_MCONTEXT_SC_REGS+18*8($16)
	stq	$19, UC_MCONTEXT_SC_REGS+19*8($16)
	stq	$20, UC_MCONTEXT_SC_REGS+20*8($16)
	stq	$21, UC_MCONTEXT_SC_REGS+21*8($16)
	stq	$22, UC_MCONTEXT_SC_REGS+22*8($16)
	stq	$23, UC_MCONTEXT_SC_REGS+23*8($16)
	stq	$24, UC_MCONTEXT_SC_REGS+24*8($16)
	stq	$25, UC_MCONTEXT_SC_REGS+25*8($16)
	stq	$26, UC_MCONTEXT_SC_REGS+26*8($16)
	stq	$27, UC_MCONTEXT_SC_REGS+27*8($16)
	.set	noat
	stq	$28, UC_MCONTEXT_SC_REGS+28*8($16)
	.set	at
	stq	$29, UC_MCONTEXT_SC_REGS+29*8($16)
	stq	$30, UC_MCONTEXT_SC_REGS+30*8($16)
	stq	$31, UC_MCONTEXT_SC_REGS+31*8($16)

	/* Save floating-point registers in sc_fpregs[0..31] */
	stt	$f0,  UC_MCONTEXT_SC_FPREGS+0*8($16)
	stt	$f1,  UC_MCONTEXT_SC_FPREGS+1*8($16)
	stt	$f2,  UC_MCONTEXT_SC_FPREGS+2*8($16)
	stt	$f3,  UC_MCONTEXT_SC_FPREGS+3*8($16)
	stt	$f4,  UC_MCONTEXT_SC_FPREGS+4*8($16)
	stt	$f5,  UC_MCONTEXT_SC_FPREGS+5*8($16)
	stt	$f6,  UC_MCONTEXT_SC_FPREGS+6*8($16)
	stt	$f7,  UC_MCONTEXT_SC_FPREGS+7*8($16)
	stt	$f8,  UC_MCONTEXT_SC_FPREGS+8*8($16)
	stt	$f9,  UC_MCONTEXT_SC_FPREGS+9*8($16)
	stt	$f10, UC_MCONTEXT_SC_FPREGS+10*8($16)
	stt	$f11, UC_MCONTEXT_SC_FPREGS+11*8($16)
	stt	$f12, UC_MCONTEXT_SC_FPREGS+12*8($16)
	stt	$f13, UC_MCONTEXT_SC_FPREGS+13*8($16)
	stt	$f14, UC_MCONTEXT_SC_FPREGS+14*8($16)
	stt	$f15, UC_MCONTEXT_SC_FPREGS+15*8($16)
	stt	$f16, UC_MCONTEXT_SC_FPREGS+16*8($16)
	stt	$f17, UC_MCONTEXT_SC_FPREGS+17*8($16)
	stt	$f18, UC_MCONTEXT_SC_FPREGS+18*8($16)
	stt	$f19, UC_MCONTEXT_SC_FPREGS+19*8($16)
	stt	$f20, UC_MCONTEXT_SC_FPREGS+20*8($16)
	stt	$f21, UC_MCONTEXT_SC_FPREGS+21*8($16)
	stt	$f22, UC_MCONTEXT_SC_FPREGS+22*8($16)
	stt	$f23, UC_MCONTEXT_SC_FPREGS+23*8($16)
	stt	$f24, UC_MCONTEXT_SC_FPREGS+24*8($16)
	stt	$f25, UC_MCONTEXT_SC_FPREGS+25*8($16)
	stt	$f26, UC_MCONTEXT_SC_FPREGS+26*8($16)
	stt	$f27, UC_MCONTEXT_SC_FPREGS+27*8($16)
	stt	$f28, UC_MCONTEXT_SC_FPREGS+28*8($16)
	stt	$f29, UC_MCONTEXT_SC_FPREGS+29*8($16)
	stt	$f30, UC_MCONTEXT_SC_FPREGS+30*8($16)
	stt	$f31, UC_MCONTEXT_SC_FPREGS+31*8($16)

	/* Save FPCR */
	mf_fpcr	$f0
	stt	$f0, UC_MCONTEXT_SC_FPCR($16)
	ldt	$f0, UC_MCONTEXT_SC_FPREGS+0*8($16)	/* restore $f0 */

	/* Return 0 */
	clr	$0
	ret	$31, ($26), 1

	.cfi_endproc
	.size _Ualpha_getcontext, . - _Ualpha_getcontext

	/* We do not need executable stack.  */
	.section	.note.GNU-stack,"",@progbits
