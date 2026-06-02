/* libunwind - a platform-independent unwind library

   Coyright (c) 2014 Oracle Inc.
   Contributed by:
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

#include "unwind_i.h"
#include "init.h"
#include <signal.h>
#include <asm/ptrace.h>

#ifdef UNW_REMOTE_ONLY

int
unw_init_local (unw_cursor_t *cursor, ucontext_t *uc)
{
  /* XXX: empty stub.  */
  return -UNW_EINVAL;
}

#else /* !UNW_REMOTE_ONLY */

static int
unw_init_local_common (unw_cursor_t *cursor, unw_context_t *uc, unsigned use_prev_instr)
{
  struct cursor *c = (struct cursor *) cursor;

  /* Flush all register windows to their register save areas on the stack so
     that DWARF-based unwinding can read I-registers from memory.  Without
     this, unspilled windows contain stale/garbage data and the unwind chain
     never terminates.  */
  asm volatile ("flushw");

  if (!atomic_load(&tdep_init_done))
    tdep_init ();

  Debug (1, "(cursor=%p)\n", c);

  c->dwarf.as = unw_local_addr_space;
  c->dwarf.as_arg = c;
  c->uc = uc;
  c->validate = 0;

  return common_init (c, use_prev_instr);
}

int
unw_init_local (unw_cursor_t *cursor, unw_context_t *uc)
{
  return unw_init_local_common(cursor, uc, 0);
}

int
unw_init_local2 (unw_cursor_t *cursor, unw_context_t *uc, int flag)
{
  if (flag == 0)
    return unw_init_local_common (cursor, uc, 0);

  if (flag != UNW_INIT_SIGNAL_FRAME)
    return -UNW_EINVAL;

  /* On Linux SPARC64, the 3rd argument to a SA_SIGINFO handler is
     &sf->info (UREG_I2 = &sf->info per signal_64.c:setup_rt_frame).
     The signal frame layout is: struct sparc_stackf ss; siginfo_t info; ...
     so ss = (struct sparc_stackf*)uc - 1.
     The kernel (and QEMU via save_reg_win) saves the faulting window's
     I-registers into ss.ins[]/ss.fp/ss.callers_pc.  Use these directly
     instead of pt_regs.u_regs[8..15], which QEMU incorrectly fills with
     O-registers rather than I-registers.  */
  struct cursor *c = (struct cursor *) cursor;
  struct pt_regs *regs = (struct pt_regs *) ((siginfo_t *) uc + 1);
  struct sparc_stackf *ss = (struct sparc_stackf *) uc - 1;
  int i;

  if (!atomic_load (&tdep_init_done))
    tdep_init ();

  Debug (1, "(signal frame cursor=%p)\n", c);

  c->dwarf.as = unw_local_addr_space;
  c->dwarf.as_arg = c;
  c->uc = uc;
  c->validate = 0;

  for (i = 0; i < DWARF_NUM_PRESERVED_REGS; i++)
    c->dwarf.loc[i] = DWARF_NULL_LOC;

  for (i = UNW_SPARC64_FIRST_FPREG; i <= UNW_SPARC64_LAST_FPREG; i++)
    c->dwarf.loc[i] = DWARF_FPREG_LOC (&c->dwarf, i);

  /* G0-G7: pt_regs.u_regs[0..7] (consistent between kernel and QEMU).  */
  for (i = UNW_SPARC64_G0; i <= UNW_SPARC64_G7; i++)
    c->dwarf.loc[i] = DWARF_MEM_LOC (&c->dwarf, (unw_word_t) &regs->u_regs[i]);

  /* O6: not captured; leave as NULL (apply_reg_state special-cases NULL O6). */
  /* O0-O5: not captured; leave as NULL.  */
  /* L0-L7: not captured; leave as NULL.  */

  /* O7 (return address register): from pt_regs.u_regs[UREG_RETPC].
     Leaf functions (e.g. kill()) use O7 as their DWARF RA column.  Without
     this, apply_reg_state sees NULL loc for the RA, sets ip=0, and step
     returns 0 (false end-of-stack) on the very first step.  */
  c->dwarf.loc[UNW_SPARC64_O7] = DWARF_MEM_LOC (&c->dwarf,
      (unw_word_t) &regs->u_regs[UREG_RETPC]);

  /* I0-I5: from ss->ins[0..5] (saved by kernel raw_copy_in_user / QEMU save_reg_win).  */
  for (i = UNW_SPARC64_I0; i < UNW_SPARC64_I6; i++)
    c->dwarf.loc[i] = DWARF_MEM_LOC (&c->dwarf,
        (unw_word_t) &ss->ins[i - UNW_SPARC64_I0]);

  /* I6 (fp) and I7 (callers_pc) stored explicitly in sparc_stackf.  */
  c->dwarf.loc[UNW_SPARC64_I6] = DWARF_MEM_LOC (&c->dwarf, (unw_word_t) &ss->fp);
  c->dwarf.loc[UNW_SPARC64_I7] = DWARF_MEM_LOC (&c->dwarf, (unw_word_t) &ss->callers_pc);

  c->dwarf.ip = regs->tpc;
  c->dwarf.cfa = (unw_word_t) ss->fp + SPARC64_STACK_BIAS;

  c->sigcontext_format = SPARC64_SCF_NONE;
  c->sigcontext_addr = 0;
  c->dwarf.args_size = 0;
  c->dwarf.stash_frames = 0;
  c->dwarf.use_prev_instr = 0;
  c->dwarf.pi_valid = 0;
  c->dwarf.pi_is_dynamic = 0;
  c->dwarf.hint = 0;
  c->dwarf.prev_rs = 0;

  return 0;
}

#endif /* !UNW_REMOTE_ONLY */
