/* libunwind - a platform-independent unwind library
   Copyright (C) 2014 Oracle Inc
   Contributed by
     Jose E. Marchesi <jose.marchesi@oracle.com>

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

#include <stdlib.h>
#include <ucontext.h>
#include <asm/ptrace.h>

#include "unwind_i.h"

#ifndef UNW_REMOTE_ONLY

HIDDEN inline int
sparc64_local_resume (unw_addr_space_t as, unw_cursor_t *cursor, void *arg)
{
  struct cursor *c = (struct cursor *) cursor;
  ucontext_t *uc = c->uc;

  if (unlikely (c->sigcontext_format != SPARC64_SCF_NONE))
    {
      /* QEMU's sparc64_set_context (ta 0x6f) loads L0-L7 and I0-I5
         from the register-save area (RSA) at O6+STACK_BIAS.  Unlike
         the real kernel which flushes all register windows to their
         RSAs before signal delivery, QEMU user mode only saves the
         interrupted window to sf->ss (via save_reg_win), leaving the
         RSA at the interrupted frame's stack pointer uninitialised.
         Copy the saved window to the RSA so setcontext finds correct
         L/I register values.  I6 and I7 come from mc_fp and mc_i7. */
      const struct sparc_stackf *ss =
        (const struct sparc_stackf *) c->sigcontext_addr;
      unsigned long *rsa =
        (unsigned long *) (c->sigcontext_sp + SPARC64_STACK_BIAS);
      unsigned i;

      for (i = 0; i < 8; i++)
        rsa[i] = ss->locals[i];        /* L0-L7 at RSA+0..+56   */
      for (i = 0; i < 6; i++)
        rsa[8 + i] = ss->ins[i];       /* I0-I5 at RSA+64..+104 */

      Debug (8, "signal frame: populated RSA at %p from sf->ss\n", rsa);

      /* Restore the pre-signal mask.  The rt_signal_frame layout places
         the pre-signal sigset_t at offset 512 from the frame base:
           ss(192) + info(128) + regs(160) + fpu_save(8) + stack(24) = 512.
         glibc's setcontext() copies uc->uc_sigmask (offset 536, named
         UC_SIGMASK in glibc's ucontext_i.h) into the compact __uc_sigmask
         slot (offset 16) immediately before the `ta 0x6f` trap; the kernel
         (and QEMU's sparc64_set_context) reads the mask from that compact
         slot.  Writing offset 16 directly would be clobbered by glibc's
         copy, so write to uc_sigmask instead.  Without this, setcontext
         restores the mask that was active inside the signal handler (with
         SIGUSR1/SIGUSR2 blocked), preventing delivery of the pending
         signal after resumption.  */
      uc->uc_sigmask.__val[0] =
        *(const unsigned long *)
          ((const char *)(uintptr_t)c->sigcontext_addr + 512);
    }

  Debug (8, "resuming at ip=%llx via setcontext()\n",
         (unsigned long long) c->dwarf.ip);
  setcontext (uc);
  return -UNW_EINVAL;
}

#endif /* !UNW_REMOTE_ONLY */

/* Copy cursor's register state into the ucontext so that setcontext()
   will resume execution at the landing pad.  */

static inline int
establish_machine_state (struct cursor *c)
{
  unw_addr_space_t as = c->dwarf.as;
  void *arg = c->dwarf.as_arg;
  ucontext_t *uc = c->uc;
  unw_word_t val;
  int reg;

  Debug (8, "copying out cursor state\n");

  for (reg = 0; reg <= UNW_SPARC64_LAST_GPREG; ++reg)
    {
      Debug (16, "copying %s %d\n", unw_regname (reg), reg);
      if (tdep_access_reg (c, reg, &val, 0) >= 0)
        as->acc.access_reg (as, reg, &val, 1, arg);
    }

  /* MC_O6 (SP) must hold the biased stack pointer (physical_SP - 2047).
     tdep_access_reg for UNW_TDEP_SP already subtracts the bias, and
     access_reg for O6 writes directly to mc_gregs[MC_O6], so the loop
     above handles this correctly.  We still need to write mc_fp and mc_i7
     to the ucontext (they are placed into the register-save area by the
     kernel's setcontext handler).

     c->dwarf.cfa is the physical SP of the landing-pad frame (= CFA of
     the callee that was stepped through).  The save area for the landing-
     pad frame's I registers begins at [c->dwarf.cfa + 0].  I6 is at
     offset 112 and I7 at offset 120 within the register-window save area
     (8 locals × 8 bytes = 64, then I0..I5 = 48 bytes, so I6 starts at
     64 + 6×8 = 112).  */
  if (dwarf_get (&c->dwarf, c->dwarf.loc[UNW_SPARC64_I6], &val) >= 0)
    uc->uc_mcontext.mc_fp = val;

  if (dwarf_get (&c->dwarf, c->dwarf.loc[UNW_SPARC64_I7], &val) >= 0)
    uc->uc_mcontext.mc_i7 = val;

  /* c->dwarf.ip is normalized to the "after the call" address by unw_step()
     (see comment in src/sparc64/Gstep.c) and to the interrupted PC by the
     signal-frame step path; either way setcontext should resume at exactly
     this address.  */
  uc->uc_mcontext.mc_gregs[MC_PC] = c->dwarf.ip;

  if (c->sigcontext_format != SPARC64_SCF_NONE)
    {
      /* For signal frames, tdep_access_reg(UNW_TDEP_IP) returns c->dwarf.ip
         (the interrupted PC), not the real O7 (return address).  The loop
         above therefore wrote tpc into MC_O7, which is wrong: leaf functions
         use RETL (jmpl %o7+8) to return, so a wrong O7 causes an infinite
         loop in the callee's error-handling path.  Override MC_O7 with the
         actual O7 saved in pt_regs (stored in c->dwarf.loc[UNW_SPARC64_O7]
         by the signal-frame step).  */
      if (dwarf_get (&c->dwarf, c->dwarf.loc[UNW_SPARC64_O7], &val) >= 0)
        uc->uc_mcontext.mc_gregs[MC_O7] = val;
    }

  /* Set NPC = PC + 4, required by the kernel's setcontext handler.  */
  uc->uc_mcontext.mc_gregs[MC_NPC] =
    uc->uc_mcontext.mc_gregs[MC_PC] + 4;

  return 0;
}

int
unw_resume (unw_cursor_t *cursor)
{
  struct cursor *c = (struct cursor *) cursor;
  int ret;

  Debug (1, "(cursor=%p)\n", c);

  if ((ret = establish_machine_state (c)) < 0)
    return ret;

  return (*c->dwarf.as->acc.resume) (c->dwarf.as, (unw_cursor_t *) c,
                                     c->dwarf.as_arg);
}
