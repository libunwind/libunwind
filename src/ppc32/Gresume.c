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

#include <signal.h>
#include <stdlib.h>
#include <string.h>

#include "ucontext_i.h"
#include "unwind_i.h"

#ifndef UNW_REMOTE_ONLY

/* glibc and musl disagree over the layout of this struct */
#ifdef __GLIBC__
#define _UCONTEXT_UC_REGS(uc) (uc)->uc_mcontext.uc_regs
#else
#define _UCONTEXT_UC_REGS(uc) (uc)->uc_regs
#endif

/* Offset of oldmask within struct sigcontext (see Gstep.c).  */
#define SIGCONTEXT_OLDMASK  24

HIDDEN inline int
ppc32_local_resume (unw_addr_space_t as, unw_cursor_t *cursor, void *arg)
{
  struct cursor *c = (struct cursor *) cursor;
  ucontext_t *uc = (ucontext_t *) c->dwarf.as_arg;

  if (unlikely (c->sigcontext_format == PPC_SCF_LINUX_RT_SIGFRAME))
    {
      /* The cursor has been stepped through a signal frame.  Copy the
         cursor's register state from the handler's ucontext (where
         establish_machine_state deposited it) into the signal frame's
         ucontext, then use setcontext to resume.  See ppc64's Gresume.c
         for the rationale; the same reasoning applies here.  */
      ucontext_t *sig_uc = (ucontext_t *) c->sigcontext_addr;
      int i;

      Debug (8, "resuming at ip=%lx via setcontext(%p)\n",
             (unsigned long) c->dwarf.ip, sig_uc);

      /* Copy GPR0-GPR31.  */
      for (i = 0; i < 32; i++)
        _UCONTEXT_UC_REGS (sig_uc)->gregs[i]
          = _UCONTEXT_UC_REGS (uc)->gregs[i];

      /* Copy NIP, CTR, LR, XER, CCR (skip MSR, ORIG_GPR3, and other
         kernel-internal fields).  */
      _UCONTEXT_UC_REGS (sig_uc)->gregs[NIP_IDX]
        = _UCONTEXT_UC_REGS (uc)->gregs[NIP_IDX];
      _UCONTEXT_UC_REGS (sig_uc)->gregs[CTR_IDX]
        = _UCONTEXT_UC_REGS (uc)->gregs[CTR_IDX];
      _UCONTEXT_UC_REGS (sig_uc)->gregs[LINK_IDX]
        = _UCONTEXT_UC_REGS (uc)->gregs[LINK_IDX];
      _UCONTEXT_UC_REGS (sig_uc)->gregs[XER_IDX]
        = _UCONTEXT_UC_REGS (uc)->gregs[XER_IDX];
      _UCONTEXT_UC_REGS (sig_uc)->gregs[CCR_IDX]
        = _UCONTEXT_UC_REGS (uc)->gregs[CCR_IDX];

      /* Copy FPRs.  */
      memcpy (&_UCONTEXT_UC_REGS (sig_uc)->fpregs,
              &_UCONTEXT_UC_REGS (uc)->fpregs,
              sizeof (_UCONTEXT_UC_REGS (sig_uc)->fpregs));

      /* The default setcontext (@GLIBC_2.3.4) on ppc32 is a thin
         swapcontext syscall wrapper and does NOT restore the signal
         mask.  Restore it ourselves from the signal frame's ucontext
         so pending signals (e.g. those blocked by the handler) can
         be delivered after the resume.  */
      sigprocmask (SIG_SETMASK, &sig_uc->uc_sigmask, NULL);

      setcontext (sig_uc);
    }
  else if (unlikely (c->sigcontext_format == PPC_SCF_LINUX_SIGFRAME))
    {
      /* Non-RT (legacy) signal frame.  There is no ucontext_t here --
         just a struct sigcontext with an oldmask field holding the
         pre-handler signal mask (low 32 bits) and a pointer to a
         struct pt_regs with the saved register state.

         Restore the signal mask from oldmask so pending signals that
         were blocked by the handler can be delivered after resume,
         then setcontext() on the handler's ucontext (which has been
         populated by establish_machine_state with the cursor's
         register state, including the target IP).  */
      unw_word_t sigcontext = c->sigcontext_addr;
      unw_word_t oldmask = 0;
      sigset_t mask;
      unw_addr_space_t as_local = c->dwarf.as;
      unw_accessors_t *a = unw_get_accessors_int (as_local);
      void *arg_local = c->dwarf.as_arg;
      int ret;

      ret = (*a->access_mem) (as_local, sigcontext + SIGCONTEXT_OLDMASK,
                              &oldmask, 0, arg_local);

      Debug (8, "resuming at ip=%lx via setcontext(), oldmask=%lx\n",
             (unsigned long) c->dwarf.ip, (unsigned long) oldmask);

      if (ret >= 0)
        {
          sigemptyset (&mask);
          memcpy (&mask, &oldmask, sizeof (oldmask));
          sigprocmask (SIG_SETMASK, &mask, NULL);
        }
      else
        {
          /* Leave the handler's current mask in place rather than
             silently unblocking signals by applying oldmask=0.  */
          Debug (2, "failed to read oldmask from sigcontext (%d); "
                    "keeping handler's signal mask\n", ret);
        }

      setcontext (uc);
    }
  else
    {
      /* Not a signal frame: uc is the context unw_getcontext()
         captured at the start of unwinding, so uc_sigmask is whatever
         the mask was then -- typically still the program's current
         mask.  This sigprocmask is therefore defensive (usually a
         no-op); it's only load-bearing on the signal-frame paths
         above.  */
      Debug (8, "resuming at ip=%lx via setcontext()\n",
             (unsigned long) c->dwarf.ip);
      sigprocmask (SIG_SETMASK, &uc->uc_sigmask, NULL);
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
