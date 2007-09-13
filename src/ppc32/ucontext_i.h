/* libunwind - a platform-independent unwind library
   Copyright (C) 2006-2007 IBM
   Contributed by
     Corey Ashford <cjashfor@us.ibm.com>
     Jose Flavio Aguilar Paulino <jflavio@br.ibm.com> <joseflavio@gmail.com>

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

#ifndef ucontext_i_h
#define ucontext_i_h

#include <ucontext.h>

/* These values were derived by reading
   /usr/src/linux-2.6.18-1.8/arch/um/include/sysdep-ppc/ptrace.h and
   /usr/src/linux-2.6.18-1.8/arch/powerpc/kernel/ppc32.h
*/

#define NIP_IDX		32
#define MSR_IDX		33
#define ORIG_GPR3_IDX	34
#define CTR_IDX		35
#define LINK_IDX	36
#define XER_IDX		37
#define CCR_IDX		38
#define SOFTE_IDX	39
#define TRAP_IDX	40
#define DAR_IDX		41
#define DSISR_IDX	42
#define RESULT_IDX	43

#define VSCR_IDX        32
#define VRSAVE_IDX      33

/* These are dummy structures used only for obtaining the offsets of the
   various structure members. */
static ucontext_t dmy_ctxt;
static vrregset_t dmy_vrregset;

#define UC_MCONTEXT_GREGS_R0 ((void *)&dmy_ctxt.uc_mcontext.uc_regs->gregs[0] - (void *)&dmy_ctxt)
#define UC_MCONTEXT_GREGS_R1 ((void *)&dmy_ctxt.uc_mcontext.uc_regs->gregs[1] - (void *)&dmy_ctxt)
#define UC_MCONTEXT_GREGS_R2 ((void *)&dmy_ctxt.uc_mcontext.uc_regs->gregs[2] - (void *)&dmy_ctxt)
#define UC_MCONTEXT_GREGS_R3 ((void *)&dmy_ctxt.uc_mcontext.uc_regs->gregs[3] - (void *)&dmy_ctxt)
#define UC_MCONTEXT_GREGS_R4 ((void *)&dmy_ctxt.uc_mcontext.uc_regs->gregs[4] - (void *)&dmy_ctxt)
#define UC_MCONTEXT_GREGS_R5 ((void *)&dmy_ctxt.uc_mcontext.uc_regs->gregs[5] - (void *)&dmy_ctxt)
#define UC_MCONTEXT_GREGS_R6 ((void *)&dmy_ctxt.uc_mcontext.uc_regs->gregs[6] - (void *)&dmy_ctxt)
#define UC_MCONTEXT_GREGS_R7 ((void *)&dmy_ctxt.uc_mcontext.uc_regs->gregs[7] - (void *)&dmy_ctxt)
#define UC_MCONTEXT_GREGS_R8 ((void *)&dmy_ctxt.uc_mcontext.uc_regs->gregs[8] - (void *)&dmy_ctxt)
#define UC_MCONTEXT_GREGS_R9 ((void *)&dmy_ctxt.uc_mcontext.uc_regs->gregs[9] - (void *)&dmy_ctxt)
#define UC_MCONTEXT_GREGS_R10 ((void *)&dmy_ctxt.uc_mcontext.uc_regs->gregs[10] - (void *)&dmy_ctxt)
#define UC_MCONTEXT_GREGS_R11 ((void *)&dmy_ctxt.uc_mcontext.uc_regs->gregs[11] - (void *)&dmy_ctxt)
#define UC_MCONTEXT_GREGS_R12 ((void *)&dmy_ctxt.uc_mcontext.uc_regs->gregs[12] - (void *)&dmy_ctxt)
#define UC_MCONTEXT_GREGS_R13 ((void *)&dmy_ctxt.uc_mcontext.uc_regs->gregs[13] - (void *)&dmy_ctxt)
#define UC_MCONTEXT_GREGS_R14 ((void *)&dmy_ctxt.uc_mcontext.uc_regs->gregs[14] - (void *)&dmy_ctxt)
#define UC_MCONTEXT_GREGS_R15 ((void *)&dmy_ctxt.uc_mcontext.uc_regs->gregs[15] - (void *)&dmy_ctxt)
#define UC_MCONTEXT_GREGS_R16 ((void *)&dmy_ctxt.uc_mcontext.uc_regs->gregs[16] - (void *)&dmy_ctxt)
#define UC_MCONTEXT_GREGS_R17 ((void *)&dmy_ctxt.uc_mcontext.uc_regs->gregs[17] - (void *)&dmy_ctxt)
#define UC_MCONTEXT_GREGS_R18 ((void *)&dmy_ctxt.uc_mcontext.uc_regs->gregs[18] - (void *)&dmy_ctxt)
#define UC_MCONTEXT_GREGS_R19 ((void *)&dmy_ctxt.uc_mcontext.uc_regs->gregs[19] - (void *)&dmy_ctxt)
#define UC_MCONTEXT_GREGS_R20 ((void *)&dmy_ctxt.uc_mcontext.uc_regs->gregs[20] - (void *)&dmy_ctxt)
#define UC_MCONTEXT_GREGS_R21 ((void *)&dmy_ctxt.uc_mcontext.uc_regs->gregs[21] - (void *)&dmy_ctxt)
#define UC_MCONTEXT_GREGS_R22 ((void *)&dmy_ctxt.uc_mcontext.uc_regs->gregs[22] - (void *)&dmy_ctxt)
#define UC_MCONTEXT_GREGS_R23 ((void *)&dmy_ctxt.uc_mcontext.uc_regs->gregs[23] - (void *)&dmy_ctxt)
#define UC_MCONTEXT_GREGS_R24 ((void *)&dmy_ctxt.uc_mcontext.uc_regs->gregs[24] - (void *)&dmy_ctxt)
#define UC_MCONTEXT_GREGS_R25 ((void *)&dmy_ctxt.uc_mcontext.uc_regs->gregs[25] - (void *)&dmy_ctxt)
#define UC_MCONTEXT_GREGS_R26 ((void *)&dmy_ctxt.uc_mcontext.uc_regs->gregs[26] - (void *)&dmy_ctxt)
#define UC_MCONTEXT_GREGS_R27 ((void *)&dmy_ctxt.uc_mcontext.uc_regs->gregs[27] - (void *)&dmy_ctxt)
#define UC_MCONTEXT_GREGS_R28 ((void *)&dmy_ctxt.uc_mcontext.uc_regs->gregs[28] - (void *)&dmy_ctxt)
#define UC_MCONTEXT_GREGS_R29 ((void *)&dmy_ctxt.uc_mcontext.uc_regs->gregs[29] - (void *)&dmy_ctxt)
#define UC_MCONTEXT_GREGS_R30 ((void *)&dmy_ctxt.uc_mcontext.uc_regs->gregs[30] - (void *)&dmy_ctxt)
#define UC_MCONTEXT_GREGS_R31 ((void *)&dmy_ctxt.uc_mcontext.uc_regs->gregs[31] - (void *)&dmy_ctxt)
#define UC_MCONTEXT_GREGS_NIP ((void *)&dmy_ctxt.uc_mcontext.uc_regs->gregs[NIP_IDX] - (void *)&dmy_ctxt)
#define UC_MCONTEXT_GREGS_MSR ((void *)&dmy_ctxt.uc_mcontext.uc_regs->gregs[MSR_IDX] - (void *)&dmy_ctxt)
#define UC_MCONTEXT_GREGS_ORIG_GPR3 ((void *)&dmy_ctxt.uc_mcontext.uc_regs->gregs[ORIG_GPR3_IDX] - (void *)&dmy_ctxt)
#define UC_MCONTEXT_GREGS_CTR ((void *)&dmy_ctxt.uc_mcontext.uc_regs->gregs[CTR_IDX] - (void *)&dmy_ctxt)
#define UC_MCONTEXT_GREGS_LINK ((void *)&dmy_ctxt.uc_mcontext.uc_regs->gregs[LINK_IDX] - (void *)&dmy_ctxt)
#define UC_MCONTEXT_GREGS_XER ((void *)&dmy_ctxt.uc_mcontext.uc_regs->gregs[XER_IDX] - (void *)&dmy_ctxt)
#define UC_MCONTEXT_GREGS_CCR ((void *)&dmy_ctxt.uc_mcontext.uc_regs->gregs[CCR_IDX] - (void *)&dmy_ctxt)
#define UC_MCONTEXT_GREGS_SOFTE ((void *)&dmy_ctxt.uc_mcontext.uc_regs->gregs[SOFTE_IDX] - (void *)&dmy_ctxt)
#define UC_MCONTEXT_GREGS_TRAP ((void *)&dmy_ctxt.uc_mcontext.uc_regs->gregs[TRAP_IDX] - (void *)&dmy_ctxt)
#define UC_MCONTEXT_GREGS_DAR ((void *)&dmy_ctxt.uc_mcontext.uc_regs->gregs[DAR_IDX] - (void *)&dmy_ctxt)
#define UC_MCONTEXT_GREGS_DSISR ((void *)&dmy_ctxt.uc_mcontext.uc_regs->gregs[DSISR_IDX] - (void *)&dmy_ctxt)
#define UC_MCONTEXT_GREGS_RESULT ((void *)&dmy_ctxt.uc_mcontext.uc_regs->gregs[RESULT_IDX] - (void *)&dmy_ctxt)

#define UC_MCONTEXT_FREGS_R0 ((void *)&dmy_ctxt.uc_mcontext.uc_regs->fpregs.fpregs[0] - (void *)&dmy_ctxt)
#define UC_MCONTEXT_FREGS_R1 ((void *)&dmy_ctxt.uc_mcontext.uc_regs->fpregs.fpregs[1] - (void *)&dmy_ctxt)
#define UC_MCONTEXT_FREGS_R2 ((void *)&dmy_ctxt.uc_mcontext.uc_regs->fpregs.fpregs[2] - (void *)&dmy_ctxt)
#define UC_MCONTEXT_FREGS_R3 ((void *)&dmy_ctxt.uc_mcontext.uc_regs->fpregs.fpregs[3] - (void *)&dmy_ctxt)
#define UC_MCONTEXT_FREGS_R4 ((void *)&dmy_ctxt.uc_mcontext.uc_regs->fpregs.fpregs[4] - (void *)&dmy_ctxt)
#define UC_MCONTEXT_FREGS_R5 ((void *)&dmy_ctxt.uc_mcontext.uc_regs->fpregs.fpregs[5] - (void *)&dmy_ctxt)
#define UC_MCONTEXT_FREGS_R6 ((void *)&dmy_ctxt.uc_mcontext.uc_regs->fpregs.fpregs[6] - (void *)&dmy_ctxt)
#define UC_MCONTEXT_FREGS_R7 ((void *)&dmy_ctxt.uc_mcontext.uc_regs->fpregs.fpregs[7] - (void *)&dmy_ctxt)
#define UC_MCONTEXT_FREGS_R8 ((void *)&dmy_ctxt.uc_mcontext.uc_regs->fpregs.fpregs[8] - (void *)&dmy_ctxt)
#define UC_MCONTEXT_FREGS_R9 ((void *)&dmy_ctxt.uc_mcontext.uc_regs->fpregs.fpregs[9] - (void *)&dmy_ctxt)
#define UC_MCONTEXT_FREGS_R10 ((void *)&dmy_ctxt.uc_mcontext.uc_regs->fpregs.fpregs[10] - (void *)&dmy_ctxt)
#define UC_MCONTEXT_FREGS_R11 ((void *)&dmy_ctxt.uc_mcontext.uc_regs->fpregs.fpregs[11] - (void *)&dmy_ctxt)
#define UC_MCONTEXT_FREGS_R12 ((void *)&dmy_ctxt.uc_mcontext.uc_regs->fpregs.fpregs[12] - (void *)&dmy_ctxt)
#define UC_MCONTEXT_FREGS_R13 ((void *)&dmy_ctxt.uc_mcontext.uc_regs->fpregs.fpregs[13] - (void *)&dmy_ctxt)
#define UC_MCONTEXT_FREGS_R14 ((void *)&dmy_ctxt.uc_mcontext.uc_regs->fpregs.fpregs[14] - (void *)&dmy_ctxt)
#define UC_MCONTEXT_FREGS_R15 ((void *)&dmy_ctxt.uc_mcontext.uc_regs->fpregs.fpregs[15] - (void *)&dmy_ctxt)
#define UC_MCONTEXT_FREGS_R16 ((void *)&dmy_ctxt.uc_mcontext.uc_regs->fpregs.fpregs[16] - (void *)&dmy_ctxt)
#define UC_MCONTEXT_FREGS_R17 ((void *)&dmy_ctxt.uc_mcontext.uc_regs->fpregs.fpregs[17] - (void *)&dmy_ctxt)
#define UC_MCONTEXT_FREGS_R18 ((void *)&dmy_ctxt.uc_mcontext.uc_regs->fpregs.fpregs[18] - (void *)&dmy_ctxt)
#define UC_MCONTEXT_FREGS_R19 ((void *)&dmy_ctxt.uc_mcontext.uc_regs->fpregs.fpregs[19] - (void *)&dmy_ctxt)
#define UC_MCONTEXT_FREGS_R20 ((void *)&dmy_ctxt.uc_mcontext.uc_regs->fpregs.fpregs[20] - (void *)&dmy_ctxt)
#define UC_MCONTEXT_FREGS_R21 ((void *)&dmy_ctxt.uc_mcontext.uc_regs->fpregs.fpregs[21] - (void *)&dmy_ctxt)
#define UC_MCONTEXT_FREGS_R22 ((void *)&dmy_ctxt.uc_mcontext.uc_regs->fpregs.fpregs[22] - (void *)&dmy_ctxt)
#define UC_MCONTEXT_FREGS_R23 ((void *)&dmy_ctxt.uc_mcontext.uc_regs->fpregs.fpregs[23] - (void *)&dmy_ctxt)
#define UC_MCONTEXT_FREGS_R24 ((void *)&dmy_ctxt.uc_mcontext.uc_regs->fpregs.fpregs[24] - (void *)&dmy_ctxt)
#define UC_MCONTEXT_FREGS_R25 ((void *)&dmy_ctxt.uc_mcontext.uc_regs->fpregs.fpregs[25] - (void *)&dmy_ctxt)
#define UC_MCONTEXT_FREGS_R26 ((void *)&dmy_ctxt.uc_mcontext.uc_regs->fpregs.fpregs[26] - (void *)&dmy_ctxt)
#define UC_MCONTEXT_FREGS_R27 ((void *)&dmy_ctxt.uc_mcontext.uc_regs->fpregs.fpregs[27] - (void *)&dmy_ctxt)
#define UC_MCONTEXT_FREGS_R28 ((void *)&dmy_ctxt.uc_mcontext.uc_regs->fpregs.fpregs[28] - (void *)&dmy_ctxt)
#define UC_MCONTEXT_FREGS_R29 ((void *)&dmy_ctxt.uc_mcontext.uc_regs->fpregs.fpregs[29] - (void *)&dmy_ctxt)
#define UC_MCONTEXT_FREGS_R30 ((void *)&dmy_ctxt.uc_mcontext.uc_regs->fpregs.fpregs[30] - (void *)&dmy_ctxt)
#define UC_MCONTEXT_FREGS_R31 ((void *)&dmy_ctxt.uc_mcontext.uc_regs->fpregs.fpregs[31] - (void *)&dmy_ctxt)
#define UC_MCONTEXT_FREGS_FPSCR ((void *)&dmy_ctxt.uc_mcontext.uc_regs->fpregs.fpregs[32] - (void *)&dmy_ctxt)

#define UC_MCONTEXT_V_REGS ((void *)&dmy_ctxt.uc_mcontext.v_regs - (void *)&dmy_ctxt)

#define UC_MCONTEXT_VREGS_R0 ((void *)&dmy_vrregset.vrregs[0] - (void *)&dmy_vrregset)
#define UC_MCONTEXT_VREGS_R1 ((void *)&dmy_vrregset.vrregs[1] - (void *)&dmy_vrregset)
#define UC_MCONTEXT_VREGS_R2 ((void *)&dmy_vrregset.vrregs[2] - (void *)&dmy_vrregset)
#define UC_MCONTEXT_VREGS_R3 ((void *)&dmy_vrregset.vrregs[3] - (void *)&dmy_vrregset)
#define UC_MCONTEXT_VREGS_R4 ((void *)&dmy_vrregset.vrregs[4] - (void *)&dmy_vrregset)
#define UC_MCONTEXT_VREGS_R5 ((void *)&dmy_vrregset.vrregs[5] - (void *)&dmy_vrregset)
#define UC_MCONTEXT_VREGS_R6 ((void *)&dmy_vrregset.vrregs[6] - (void *)&dmy_vrregset)
#define UC_MCONTEXT_VREGS_R7 ((void *)&dmy_vrregset.vrregs[7] - (void *)&dmy_vrregset)
#define UC_MCONTEXT_VREGS_R8 ((void *)&dmy_vrregset.vrregs[8] - (void *)&dmy_vrregset)
#define UC_MCONTEXT_VREGS_R9 ((void *)&dmy_vrregset.vrregs[9] - (void *)&dmy_vrregset)
#define UC_MCONTEXT_VREGS_R10 ((void *)&dmy_vrregset.vrregs[10] - (void *)&dmy_vrregset)
#define UC_MCONTEXT_VREGS_R11 ((void *)&dmy_vrregset.vrregs[11] - (void *)&dmy_vrregset)
#define UC_MCONTEXT_VREGS_R12 ((void *)&dmy_vrregset.vrregs[12] - (void *)&dmy_vrregset)
#define UC_MCONTEXT_VREGS_R13 ((void *)&dmy_vrregset.vrregs[13] - (void *)&dmy_vrregset)
#define UC_MCONTEXT_VREGS_R14 ((void *)&dmy_vrregset.vrregs[14] - (void *)&dmy_vrregset)
#define UC_MCONTEXT_VREGS_R15 ((void *)&dmy_vrregset.vrregs[15] - (void *)&dmy_vrregset)
#define UC_MCONTEXT_VREGS_R16 ((void *)&dmy_vrregset.vrregs[16] - (void *)&dmy_vrregset)
#define UC_MCONTEXT_VREGS_R17 ((void *)&dmy_vrregset.vrregs[17] - (void *)&dmy_vrregset)
#define UC_MCONTEXT_VREGS_R18 ((void *)&dmy_vrregset.vrregs[18] - (void *)&dmy_vrregset)
#define UC_MCONTEXT_VREGS_R19 ((void *)&dmy_vrregset.vrregs[19] - (void *)&dmy_vrregset)
#define UC_MCONTEXT_VREGS_R20 ((void *)&dmy_vrregset.vrregs[20] - (void *)&dmy_vrregset)
#define UC_MCONTEXT_VREGS_R21 ((void *)&dmy_vrregset.vrregs[21] - (void *)&dmy_vrregset)
#define UC_MCONTEXT_VREGS_R22 ((void *)&dmy_vrregset.vrregs[22] - (void *)&dmy_vrregset)
#define UC_MCONTEXT_VREGS_R23 ((void *)&dmy_vrregset.vrregs[23] - (void *)&dmy_vrregset)
#define UC_MCONTEXT_VREGS_R24 ((void *)&dmy_vrregset.vrregs[24] - (void *)&dmy_vrregset)
#define UC_MCONTEXT_VREGS_R25 ((void *)&dmy_vrregset.vrregs[25] - (void *)&dmy_vrregset)
#define UC_MCONTEXT_VREGS_R26 ((void *)&dmy_vrregset.vrregs[26] - (void *)&dmy_vrregset)
#define UC_MCONTEXT_VREGS_R27 ((void *)&dmy_vrregset.vrregs[27] - (void *)&dmy_vrregset)
#define UC_MCONTEXT_VREGS_R28 ((void *)&dmy_vrregset.vrregs[28] - (void *)&dmy_vrregset)
#define UC_MCONTEXT_VREGS_R29 ((void *)&dmy_vrregset.vrregs[29] - (void *)&dmy_vrregset)
#define UC_MCONTEXT_VREGS_R30 ((void *)&dmy_vrregset.vrregs[30] - (void *)&dmy_vrregset)
#define UC_MCONTEXT_VREGS_R31 ((void *)&dmy_vrregset.vrregs[31] - (void *)&dmy_vrregset)
#define UC_MCONTEXT_VREGS_VSCR ((void *)&dmy_vrregset.vscr - (void *)&dmy_vrregset)
#define UC_MCONTEXT_VREGS_VRSAVE ((void *)&dmy_vrregset.vrsave - (void *)&dmy_vrregset)

#endif
