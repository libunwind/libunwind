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

/*  int _Ualpha_setcontext(const ucontext_t *ucp)

    Restores the machine context from a ucontext_t structure.
    $16 (a0) = pointer to ucontext_t.
*/

/* Offsets from ucontext_t base */
#define UC_MCONTEXT_SC_PC	64
#define UC_MCONTEXT_SC_REGS	80
#define UC_MCONTEXT_SC_FPREGS	344
#define UC_MCONTEXT_SC_FPCR	600

	.global _Ualpha_setcontext
	.type _Ualpha_setcontext, @function
_Ualpha_setcontext:
	.cfi_startproc

	/* Stop unwinding through this function. */
	.cfi_undefined $26

	/* Restore FPCR */
	ldt	$f0, UC_MCONTEXT_SC_FPCR($16)
	mt_fpcr	$f0

	/* Restore floating-point registers */
	ldt	$f0,  UC_MCONTEXT_SC_FPREGS+0*8($16)
	ldt	$f1,  UC_MCONTEXT_SC_FPREGS+1*8($16)
	ldt	$f2,  UC_MCONTEXT_SC_FPREGS+2*8($16)
	ldt	$f3,  UC_MCONTEXT_SC_FPREGS+3*8($16)
	ldt	$f4,  UC_MCONTEXT_SC_FPREGS+4*8($16)
	ldt	$f5,  UC_MCONTEXT_SC_FPREGS+5*8($16)
	ldt	$f6,  UC_MCONTEXT_SC_FPREGS+6*8($16)
	ldt	$f7,  UC_MCONTEXT_SC_FPREGS+7*8($16)
	ldt	$f8,  UC_MCONTEXT_SC_FPREGS+8*8($16)
	ldt	$f9,  UC_MCONTEXT_SC_FPREGS+9*8($16)
	ldt	$f10, UC_MCONTEXT_SC_FPREGS+10*8($16)
	ldt	$f11, UC_MCONTEXT_SC_FPREGS+11*8($16)
	ldt	$f12, UC_MCONTEXT_SC_FPREGS+12*8($16)
	ldt	$f13, UC_MCONTEXT_SC_FPREGS+13*8($16)
	ldt	$f14, UC_MCONTEXT_SC_FPREGS+14*8($16)
	ldt	$f15, UC_MCONTEXT_SC_FPREGS+15*8($16)
	ldt	$f16, UC_MCONTEXT_SC_FPREGS+16*8($16)
	ldt	$f17, UC_MCONTEXT_SC_FPREGS+17*8($16)
	ldt	$f18, UC_MCONTEXT_SC_FPREGS+18*8($16)
	ldt	$f19, UC_MCONTEXT_SC_FPREGS+19*8($16)
	ldt	$f20, UC_MCONTEXT_SC_FPREGS+20*8($16)
	ldt	$f21, UC_MCONTEXT_SC_FPREGS+21*8($16)
	ldt	$f22, UC_MCONTEXT_SC_FPREGS+22*8($16)
	ldt	$f23, UC_MCONTEXT_SC_FPREGS+23*8($16)
	ldt	$f24, UC_MCONTEXT_SC_FPREGS+24*8($16)
	ldt	$f25, UC_MCONTEXT_SC_FPREGS+25*8($16)
	ldt	$f26, UC_MCONTEXT_SC_FPREGS+26*8($16)
	ldt	$f27, UC_MCONTEXT_SC_FPREGS+27*8($16)
	ldt	$f28, UC_MCONTEXT_SC_FPREGS+28*8($16)
	ldt	$f29, UC_MCONTEXT_SC_FPREGS+29*8($16)
	ldt	$f30, UC_MCONTEXT_SC_FPREGS+30*8($16)
	/* skip $f31 - hardwired zero */

	/* Load the target PC into a temp register before restoring GPRs */
	ldq	$0,  UC_MCONTEXT_SC_PC($16)

	/* Restore integer registers.
	   Must restore $16 last since it's our base pointer.
	   Use $1 as a temporary to hold the target PC. */
	mov	$0,  $1
	ldq	$0,  UC_MCONTEXT_SC_REGS+0*8($16)
	/* skip $1 - used as temp for PC */
	ldq	$2,  UC_MCONTEXT_SC_REGS+2*8($16)
	ldq	$3,  UC_MCONTEXT_SC_REGS+3*8($16)
	ldq	$4,  UC_MCONTEXT_SC_REGS+4*8($16)
	ldq	$5,  UC_MCONTEXT_SC_REGS+5*8($16)
	ldq	$6,  UC_MCONTEXT_SC_REGS+6*8($16)
	ldq	$7,  UC_MCONTEXT_SC_REGS+7*8($16)
	ldq	$8,  UC_MCONTEXT_SC_REGS+8*8($16)
	ldq	$9,  UC_MCONTEXT_SC_REGS+9*8($16)
	ldq	$10, UC_MCONTEXT_SC_REGS+10*8($16)
	ldq	$11, UC_MCONTEXT_SC_REGS+11*8($16)
	ldq	$12, UC_MCONTEXT_SC_REGS+12*8($16)
	ldq	$13, UC_MCONTEXT_SC_REGS+13*8($16)
	ldq	$14, UC_MCONTEXT_SC_REGS+14*8($16)
	ldq	$15, UC_MCONTEXT_SC_REGS+15*8($16)
	/* skip $16 - restore last */
	ldq	$17, UC_MCONTEXT_SC_REGS+17*8($16)
	ldq	$18, UC_MCONTEXT_SC_REGS+18*8($16)
	ldq	$19, UC_MCONTEXT_SC_REGS+19*8($16)
	ldq	$20, UC_MCONTEXT_SC_REGS+20*8($16)
	ldq	$21, UC_MCONTEXT_SC_REGS+21*8($16)
	ldq	$22, UC_MCONTEXT_SC_REGS+22*8($16)
	ldq	$23, UC_MCONTEXT_SC_REGS+23*8($16)
	ldq	$24, UC_MCONTEXT_SC_REGS+24*8($16)
	ldq	$25, UC_MCONTEXT_SC_REGS+25*8($16)
	ldq	$26, UC_MCONTEXT_SC_REGS+26*8($16)
	ldq	$27, UC_MCONTEXT_SC_REGS+27*8($16)
	/* skip $28 - used below */
	ldq	$29, UC_MCONTEXT_SC_REGS+29*8($16)
	ldq	$30, UC_MCONTEXT_SC_REGS+30*8($16)
	/* skip $31 - hardwired zero */

	/* Save PC in $28 (at), restore $16, then jump */
	.set	noat
	mov	$1,  $28
	ldq	$1,  UC_MCONTEXT_SC_REGS+1*8($16)
	ldq	$16, UC_MCONTEXT_SC_REGS+16*8($16)
	jmp	$31, ($28), 0
	.set	at

	.cfi_endproc
	.size _Ualpha_setcontext, . - _Ualpha_setcontext

	/* We do not need executable stack.  */
	.section	.note.GNU-stack,"",@progbits
