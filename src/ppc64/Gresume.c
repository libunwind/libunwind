/* libunwind - a platform-independent unwind library
   Copyright (C) 2006-2007 IBM
   Contributed by
     Corey Ashford cjashfor@us.ibm.com
     Jose Flavio Aguilar Paulino <jflavio@br.ibm.com> <joseflavio@gmail.com>

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

#include "unwind_i.h"
#include "ucontext_i.h"

#ifndef UNW_REMOTE_ONLY

HIDDEN inline int
ppc64_local_resume (unw_addr_space_t as, unw_cursor_t *cursor, void *arg)
{
  struct cursor *c = (struct cursor *) cursor;
  ucontext_t *uc = (ucontext_t *)c->dwarf.as_arg;

  if (unlikely (c->sigcontext_format != PPC_SCF_NONE))
    {
      /* The cursor has been stepped through a signal frame.  Copy the
         cursor's register state from the handler's ucontext (where
         establish_machine_state deposited it) into the signal frame's
         ucontext, then use setcontext to resume.

         We use setcontext on the signal frame's ucontext rather than
         rt_sigreturn because:
         1. The signal frame's ucontext has the correct MSR (set by the
            kernel).  The handler's ucontext from getcontext() has MSR=0
            because reading MSR is privileged.
         2. The signal frame's ucontext has the correct signal mask from
            before signal delivery.
         3. The signal frame's ucontext has valid metadata fields (regs
            pointer, v_regs pointer) that setcontext may need.

         We only copy the register fields that establish_machine_state
         set (GPRs, FPRs, NIP, LR, CTR, CR, XER), preserving the
         signal frame's MSR, ORIG_R3, and other kernel-internal fields
         in the gp_regs array.  */
      ucontext_t *sig_uc = (ucontext_t *) c->sigcontext_addr;
      int i;

      Debug (8, "resuming at ip=%llx via setcontext(%p)\n",
             (unsigned long long) c->dwarf.ip, sig_uc);

      /* Copy GPR0-GPR31 */
      for (i = 0; i < 32; i++)
        sig_uc->uc_mcontext.gp_regs[i] = uc->uc_mcontext.gp_regs[i];

      /* Copy NIP, skip MSR and ORIG_R3, copy CTR, LR, XER, CCR */
      sig_uc->uc_mcontext.gp_regs[NIP_IDX] = uc->uc_mcontext.gp_regs[NIP_IDX];
      sig_uc->uc_mcontext.gp_regs[CTR_IDX] = uc->uc_mcontext.gp_regs[CTR_IDX];
      sig_uc->uc_mcontext.gp_regs[LINK_IDX] = uc->uc_mcontext.gp_regs[LINK_IDX];
      sig_uc->uc_mcontext.gp_regs[XER_IDX] = uc->uc_mcontext.gp_regs[XER_IDX];
      sig_uc->uc_mcontext.gp_regs[CCR_IDX] = uc->uc_mcontext.gp_regs[CCR_IDX];

      /* Copy FPRs */
      memcpy (&sig_uc->uc_mcontext.fp_regs, &uc->uc_mcontext.fp_regs,
              sizeof (sig_uc->uc_mcontext.fp_regs));

      setcontext (sig_uc);
    }
  else
    {
      Debug (8, "resuming at ip=%llx via setcontext()\n",
            (unsigned long long) c->dwarf.ip);
      setcontext (uc);
    }
  return -UNW_EINVAL;
}

#endif /* !UNW_REMOTE_ONLY */

/* This routine is responsible for copying the register values in
   cursor C and establishing them as the current machine state. */

static inline int
establish_machine_state (struct cursor *c)
{
  unw_addr_space_t as = c->dwarf.as;
  void *arg = c->dwarf.as_arg;
  unw_fpreg_t fpval;
  unw_word_t val;
  int reg;

  Debug (8, "copying out cursor state\n");

  for (reg = 0; reg <= UNW_REG_LAST; ++reg)
    {
      Debug (16, "copying %s %d\n", unw_regname (reg), reg);
      if (unw_is_fpreg (reg))
        {
          if (tdep_access_fpreg (c, reg, &fpval, 0) >= 0)
            as->acc.access_fpreg (as, reg, &fpval, 1, arg);
        }
      else
        {
          if (tdep_access_reg (c, reg, &val, 0) >= 0)
            as->acc.access_reg (as, reg, &val, 1, arg);
        }
    }
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
