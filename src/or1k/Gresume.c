/* libunwind - a platform-independent unwind library
   Copyright (C) 2008 CodeSourcery
   Copyright 2011 Linaro Limited
   Copyright (C) 2012 Tommi Rantala <tt.rantala@gmail.com>
   Copyright (C) 2026 Ali Ahmet Memiş <aliamemis@disroot.org>

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
#include "offsets.h"

#ifndef UNW_REMOTE_ONLY

HIDDEN inline int
or1k_local_resume (unw_addr_space_t as, unw_cursor_t *cursor, void *arg)
{
#ifdef __linux__
  struct cursor *c = (struct cursor *) cursor;
  unw_tdep_context_t *uc = c->dwarf.as_arg;
  unw_word_t *mc = (unw_word_t *) &uc->uc_mcontext;

  if (c->sigcontext_format == OR1K_SCF_NONE)
    {
      /* Restore the call-saved registers, exception-data registers, PC and
         SP.  Restore SP last since nothing may touch the stack afterwards.  */
      unsigned long regs[14];

      regs[0]  = mc[2];
      regs[1]  = mc[14];
      regs[2]  = mc[16];
      regs[3]  = mc[18];
      regs[4]  = mc[20];
      regs[5]  = mc[22];
      regs[6]  = mc[24];
      regs[7]  = mc[26];
      regs[8]  = mc[28];
      regs[9]  = mc[30];
      regs[10] = mc[25];
      regs[11] = mc[27];
      regs[12] = mc[32];
      regs[13] = mc[1];

      struct regs_overlay {
        char x[sizeof(regs)];
      };

      /* The asm never returns, so it needs no clobber list; r2 could not
         appear in one anyway when it is in use as the frame pointer.  Pin
         the array pointer to a register that is not restored, so that it
         survives the loads.  */
      register unsigned long *ptr __asm__ ("r11") = regs;

      __asm__ __volatile__ (
        "l.lwz  r2,   0(r11)\n"
        "l.lwz  r14,  4(r11)\n"
        "l.lwz  r16,  8(r11)\n"
        "l.lwz  r18, 12(r11)\n"
        "l.lwz  r20, 16(r11)\n"
        "l.lwz  r22, 20(r11)\n"
        "l.lwz  r24, 24(r11)\n"
        "l.lwz  r26, 28(r11)\n"
        "l.lwz  r28, 32(r11)\n"
        "l.lwz  r30, 36(r11)\n"
        "l.lwz  r25, 40(r11)\n"
        "l.lwz  r27, 44(r11)\n"
        "l.lwz  r9,  48(r11)\n"
        "l.lwz  r1,  52(r11)\n"
        "l.jr   r9\n"
        " l.nop\n"
        :
        : "r" (ptr),
          "m" (*(struct regs_overlay *)regs)
      );
    }
  else
    {
      /* Resume a signal frame through its rt_sigreturn trampoline.  */
      unw_word_t *sc = (unw_word_t *) c->sigcontext_addr;
      int i;

      for (i = 0; i < 32; ++i)
        sc[i] = mc[i];
      sc[LINUX_SC_PC_OFF / sizeof (unw_word_t)] = mc[32];

      __asm__ __volatile__ (
        "l.ori  r1, %0, 0\n"
        "l.jr   %1\n"
        " l.nop\n"
        :
        : "r" (c->sigcontext_sp),
          "r" (c->sigcontext_pc)
        : "memory"
      );
    }
  unreachable();
#endif
  return -UNW_EINVAL;
}

#endif /* !UNW_REMOTE_ONLY */

static inline void
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
}

int
unw_resume (unw_cursor_t *cursor)
{
  struct cursor *c = (struct cursor *) cursor;

  Debug (1, "(cursor=%p)\n", c);

  if (!c->dwarf.ip)
    {
      Debug (1, "refusing to resume execution at address 0\n");
      return -UNW_EINVAL;
    }

  establish_machine_state (c);

  return (*c->dwarf.as->acc.resume) (c->dwarf.as, (unw_cursor_t *) c,
                                     c->dwarf.as_arg);
}
