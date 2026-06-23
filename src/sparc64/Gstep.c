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

#include "unwind_i.h"
#include <stddef.h>
#include <signal.h>
#include <asm/ptrace.h>

/* Linux SPARC64 rt_signal_frame layout (from arch/sparc/kernel/signal_64.c).
   UREG_I2 passed to the SA_SIGINFO handler = &sf->info.  The kernel
   (and QEMU via save_reg_win) saves the faulting window's I-registers
   into ss.ins[]/ss.fp/ss.callers_pc, NOT into pt_regs.u_regs[8..15]
   (QEMU incorrectly stores O-registers there).  */
struct sparc64_rt_signal_frame {
  struct sparc_stackf  ss;       /* 192 bytes: register save area + stack header */
  siginfo_t            info;     /* 128 bytes */
  struct pt_regs       regs;     /* 160 bytes: saved register state at time of signal */
  unsigned long        fpu_save; /* 8 bytes */
  stack_t              stack;    /* 24 bytes */
  unsigned long        mask;     /* pre-signal mask (first word of sigset_t), at offset 512 */
};

int
unw_step (unw_cursor_t * cursor)
{
  struct cursor *c = (struct cursor *) cursor;
  int validate = c->validate;
  int ret;

  Debug (1, "(cursor=%p, ip=0x%016lx)\n", c, (unsigned long) c->dwarf.ip);

  if (c->dwarf.ip == 0)
    {
      /* Unless the cursor or stack is corrupt or uninitialized,
         we've most likely hit the top of the stack */
      return 0;
    }

  /* Validate all addresses before dereferencing. */
  c->validate = 1;

  /* Try DWARF-based unwinding first.  */
  ret = dwarf_step (&c->dwarf);

  c->validate = validate;

  if (ret < 0 && ret != -UNW_ENOINFO)
    {
      Debug (2, "returning %d\n", ret);
      return ret;
    }

  if (ret > 0)
    {
      /* After dwarf_step the cursor's IP holds the saved O7 of the next
         frame, which on SPARC is the address of the CALL instruction
         itself.  If that frame is the rt_sigreturn trampoline, fall
         through to the signal-frame setup below; otherwise bump IP past
         the call + delay slot so the rest of libunwind (FDE/LSDA lookup,
         _Unwind_GetIP, unw_resume) can treat IP as the "after the call"
         address that personality routines and setcontext both expect.  */
      if (likely (!unw_is_signal_frame (cursor)))
        {
          c->dwarf.ip += 8;
          return ret;
        }
      goto signal_frame;
    }

  if (unlikely (ret == -UNW_ENOINFO))
    {
      if (likely (!unw_is_signal_frame (cursor)))
        {
          /* No DWARF info: fall back to SPARC64 register-window conventions.
             After SAVE, the old O-registers become I-registers:
               I6 = old O6 = caller's biased sp (physical sp = I6+BIAS)
               I7 = old O7 = address of CALL instruction in caller
             The caller's register-save area (RSA) is at I6+BIAS:
               L0-L7 at RSA+0..+56, I0-I7 at RSA+64..+120.  */
          unw_word_t fp, ra;
          int i;

          if (dwarf_get (&c->dwarf, c->dwarf.loc[UNW_SPARC64_I6], &fp) < 0)
            {
              Debug (2, "returning %d\n", ret);
              return ret;
            }
          if (fp == 0)
            return 0;

          if (dwarf_get (&c->dwarf, c->dwarf.loc[UNW_SPARC64_I7], &ra) < 0)
            {
              Debug (2, "returning %d\n", ret);
              return ret;
            }

          /* See note above: bump to address after CALL + delay slot.  */
          c->dwarf.ip  = ra + 8;
          c->dwarf.cfa = fp + SPARC64_STACK_BIAS;

          for (i = UNW_SPARC64_L0; i <= UNW_SPARC64_L7; i++)
            c->dwarf.loc[i] = DWARF_MEM_LOC (&c->dwarf,
                c->dwarf.cfa + (i - UNW_SPARC64_L0) * sizeof (unsigned long));

          for (i = UNW_SPARC64_I0; i <= UNW_SPARC64_I7; i++)
            c->dwarf.loc[i] = DWARF_MEM_LOC (&c->dwarf,
                c->dwarf.cfa + (i - UNW_SPARC64_L0) * sizeof (unsigned long));

          return (c->dwarf.ip != 0) ? 1 : 0;
        }

    signal_frame:
      /* Signal frame: after dwarf_step through the signal handler via
         DW_CFA_GNU_window_save, loc[I6] holds the memory address sf+112
         (= &rt_signal_frame.ss.fp).  Derive sf from that.
         c->dwarf.cfa at this point is the CFA of the interrupted frame,
         not the base of the signal frame.  */
      ;
      unw_word_t sf = c->dwarf.loc[UNW_SPARC64_I6].val
                      - offsetof (struct sparc64_rt_signal_frame, ss.fp);
      unw_word_t regs_base = sf + offsetof (struct sparc64_rt_signal_frame, regs);
      unw_word_t tpc_addr = regs_base + offsetof (struct pt_regs, tpc);
      /* O6 (UREG_FP) from pt_regs.u_regs[14]: the interrupted function's
         biased stack pointer.  CFA = O6 + STACK_BIAS.  */
      unw_word_t o6_addr = regs_base + UREG_FP * sizeof (unsigned long);
      /* I6 and I7 are in the interrupted function's register save area at
         its physical stack pointer (CFA): I6 at CFA+112, I7 at CFA+120.  */

      unw_word_t o6_val;
      struct dwarf_loc loc;
      int i;

      Debug (1, "signal frame: cfa=0x%lx sf=0x%lx tpc_addr=0x%lx o6_addr=0x%lx\n",
             (unsigned long) c->dwarf.cfa, (unsigned long) sf,
             (unsigned long) tpc_addr, (unsigned long) o6_addr);

      /* PC of interrupted code.  */
      loc = DWARF_MEM_LOC (&c->dwarf, tpc_addr);
      if ((ret = dwarf_get (&c->dwarf, loc, &c->dwarf.ip)) < 0)
        return ret;

      /* O6 (stack pointer) of the interrupted frame from pt_regs.  */
      loc = DWARF_MEM_LOC (&c->dwarf, o6_addr);
      if ((ret = dwarf_get (&c->dwarf, loc, &o6_val)) < 0)
        return ret;
      c->dwarf.cfa = o6_val + SPARC64_STACK_BIAS;

      /* Set O6 loc so the DWARF CFA rule (CFA = O6 + STACK_BIAS) for the
         interrupted function uses the correct stack pointer from pt_regs,
         not the stale O6 from the unwind context.  */
      c->dwarf.loc[UNW_SPARC64_O6] = DWARF_MEM_LOC (&c->dwarf, o6_addr);

      /* O7 (return address register) from pt_regs.u_regs[15].  Leaf functions
         use O7 as their DWARF RA column; without this, DWARF_WHERE_SAME would
         propagate the stale O7 from the previous frame (sighandler's step sets
         loc[O7] = sf+120 = ka_restorer-8), causing a false second signal-frame
         detection.  */
      c->dwarf.loc[UNW_SPARC64_O7] = DWARF_MEM_LOC (&c->dwarf,
          regs_base + UREG_RETPC * sizeof (unsigned long));

      /* G registers (G1-G7) from pt_regs.u_regs[1..7].  */
      for (i = UNW_SPARC64_G1; i <= UNW_SPARC64_G7; i++)
        c->dwarf.loc[i] = DWARF_MEM_LOC (&c->dwarf,
            regs_base + i * sizeof (unsigned long));

      /* L0-I7: read from the signal frame's ss (saved by kernel/QEMU via
         save_reg_win), not from the RSA at c->dwarf.cfa which may be
         uninitialized in QEMU user-mode emulation.  */
      for (i = UNW_SPARC64_L0; i <= UNW_SPARC64_I7; i++)
        c->dwarf.loc[i] = DWARF_MEM_LOC (&c->dwarf,
            sf + (i - UNW_SPARC64_L0) * sizeof (unsigned long));

      /* O0-O5: not captured; leave as stale from previous frame.  */

      c->sigcontext_format = SPARC64_SCF_LINUX_RT_SIGFRAME;
      c->sigcontext_addr   = sf;
      c->sigcontext_sp     = o6_val;
      c->sigcontext_pc     = c->dwarf.ip;

      ret = 1;
    }

  return ret;
}
