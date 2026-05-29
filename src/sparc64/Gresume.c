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

#include "unwind_i.h"

/* SPARC V9 ABI: the stack pointer O6 stores (physical_SP - 2047). */
#define SPARC64_STACK_BIAS 2047

#ifndef UNW_REMOTE_ONLY

HIDDEN inline int
sparc64_local_resume (unw_addr_space_t as, unw_cursor_t *cursor, void *arg)
{
  struct cursor *c = (struct cursor *) cursor;
  ucontext_t *uc = c->uc;

  if (unlikely (c->sigcontext_format != SPARC64_SCF_NONE))
    {
      /* Signal frame resume is not yet implemented for SPARC64.
         Fall through to the non-signal path, which may not restore
         the signal mask correctly, but will at least reach the landing
         pad for the common C++ exception handling case.  */
      Debug (1, "WARNING: signal frame resume not fully implemented\n");
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

  /* UNW_TDEP_IP (O7) stores the CALL instruction address (SPARC convention).
     For normal DWARF frames, ip = the CALL instruction; setcontext must
     resume at ip+8, which is where execution resumes after the callee
     returns (RETL restores PC to O7+8, past CALL and its delay slot).
     For signal frames, ip = the actual interrupted PC (tpc from pt_regs),
     so no adjustment is needed.  */
  if (c->sigcontext_format == SPARC64_SCF_NONE)
    uc->uc_mcontext.mc_gregs[MC_PC] = c->dwarf.ip + 8;
  else
    uc->uc_mcontext.mc_gregs[MC_PC] = c->dwarf.ip;

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
