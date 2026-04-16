/* libunwind - a platform-independent unwind library
   Copyright (C) 2006-2007 IBM
   Contributed by
     Corey Ashford <cjashfor@us.ibm.com>
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

#include "unwind_i.h"
#include <signal.h>

/* The kernel's newsp = frame - (__SIGNAL_FRAMESIZE + 16), where
   __SIGNAL_FRAMESIZE is 64 on ppc32.  The rt_sigframe starts at
   'frame', with siginfo first, then ucontext:
     struct rt_sigframe {
       struct siginfo info;   // 128 bytes
       struct ucontext uc;
       ...
     };
   So: ucontext = sp + (__SIGNAL_FRAMESIZE + 16) + sizeof(siginfo_t)
                 = sp + 80 + 128 = sp + 208  */
#define RT_SIGFRAME_UCONTEXT_OFFSET  208

/* On ppc32 glibc, ucontext_t.uc_mcontext.uc_regs is a pointer to the
   mcontext_t that contains the actual register data.  The kernel sets
   this pointer when building the signal frame.  The uc_regs pointer
   is at offset 0x30 (48) within ucontext_t.  */
#define UC_MCONTEXT_UC_REGS_PTR  0x30

/* Offsets within mcontext_t.gregs[] (each entry is 4 bytes) */
#define GREGS_R0    (0 * 4)
#define GREGS_R1    (1 * 4)
#define GREGS_NIP   (32 * 4)
#define GREGS_LNK   (36 * 4)
#define GREGS_CTR   (35 * 4)
#define GREGS_XER   (37 * 4)
#define GREGS_CCR   (38 * 4)

/* Offset from mcontext_t base to fpregs (48 gregs * 4 = 192 bytes for gregs) */
#define FREGS_R0    (48 * 4)

typedef struct
{
  long unsigned back_chain;
  long unsigned lr_save;
  /* many more fields here, but they are unused by this code */
} stack_frame_t;


int
unw_step (unw_cursor_t * cursor)
{
  struct cursor *c = (struct cursor *) cursor;
  stack_frame_t dummy;
  unw_word_t back_chain_offset, lr_save_offset;
  struct dwarf_loc back_chain_loc, lr_save_loc, sp_loc, ip_loc;
  int ret;
  int validate = c->dwarf.as->validate;

  Debug (1, "(cursor=%p, ip=0x%016lx)\n", c, (unsigned long) c->dwarf.ip);

  if (c->dwarf.ip == 0)
    {
      /* Unless the cursor or stack is corrupt or uninitialized,
         we've most likely hit the top of the stack */
      return 0;
    }

  /* Check for signal frame before trying DWARF-based unwinding.
     This ensures sigcontext_format and sigcontext_addr are always set
     when stepping through a signal trampoline, even when DWARF CFI
     covers the trampoline (e.g. from the VDSO).  */
  if (unlikely (unw_is_signal_frame (cursor) > 0))
    goto signal_frame;

  /* Try DWARF-based unwinding... */

  c->dwarf.as->validate = 1;
  ret = dwarf_step (&c->dwarf);
  c->dwarf.as->validate = validate;

  if (ret < 0 && ret != -UNW_ENOINFO)
    {
      Debug (2, "returning %d\n", ret);
      return ret;
    }

  if (unlikely (ret < 0))
    {
      /* DWARF unwinding failed.  Attempt to unwind the frame by using
         the back chain.  This is very crude and won't be able to unwind
         any registers besides the IP, SP, and LR. */

      back_chain_offset = ((void *) &dummy.back_chain - (void *) &dummy);
      lr_save_offset = ((void *) &dummy.lr_save - (void *) &dummy);

      back_chain_loc = DWARF_LOC (c->dwarf.cfa + back_chain_offset, 0);

      if ((ret =
           dwarf_get (&c->dwarf, back_chain_loc, &c->dwarf.cfa)) < 0)
        {
          Debug (2,
             "Unable to retrieve CFA from back chain in stack frame - %d\n",
             ret);
          return ret;
        }
      if (c->dwarf.cfa == 0)
        /* Unless the cursor or stack is corrupt or uninitialized we've most
           likely hit the top of the stack */
        return 0;

      lr_save_loc = DWARF_LOC (c->dwarf.cfa + lr_save_offset, 0);

      if ((ret = dwarf_get (&c->dwarf, lr_save_loc, &c->dwarf.ip)) < 0)
        {
          Debug (2,
             "Unable to retrieve IP from lr save in stack frame - %d\n",
             ret);
          return ret;
        }
      ret = 1;
    }

  return ret;

signal_frame:
    {
      unw_word_t ucontext = c->dwarf.cfa + RT_SIGFRAME_UCONTEXT_OFFSET;
      unw_word_t uc_regs;
      int i;

      Debug (1, "signal frame, skip over trampoline\n");

      c->sigcontext_format = PPC_SCF_LINUX_RT_SIGFRAME;
      c->sigcontext_addr = ucontext;

      /* On ppc32, ucontext_t.uc_mcontext.uc_regs is a pointer to the
         mcontext data.  Read it to find the gregs base address.  */
      ret = dwarf_get (&c->dwarf,
                       DWARF_LOC (ucontext + UC_MCONTEXT_UC_REGS_PTR, 0),
                       &uc_regs);
      if (ret < 0)
        {
          Debug (2, "failed to read uc_regs pointer: %d\n", ret);
          return ret;
        }

      Debug (2, "ucontext=%lx, uc_regs=%lx\n",
             (unsigned long) ucontext, (unsigned long) uc_regs);

      sp_loc = DWARF_LOC (uc_regs + GREGS_R1, 0);
      ip_loc = DWARF_LOC (uc_regs + GREGS_NIP, 0);

      ret = dwarf_get (&c->dwarf, sp_loc, &c->dwarf.cfa);
      if (ret < 0)
        {
          Debug (2, "returning %d\n", ret);
          return ret;
        }
      ret = dwarf_get (&c->dwarf, ip_loc, &c->dwarf.ip);
      if (ret < 0)
        {
          Debug (2, "returning %d\n", ret);
          return ret;
        }

      /* Restore all GPRs from the signal context.  */
      for (i = UNW_PPC32_R0; i <= UNW_PPC32_R31; i++)
        c->dwarf.loc[i] = DWARF_LOC (uc_regs + GREGS_R0 + (i * 4), 0);

      c->dwarf.loc[UNW_PPC32_LR] =
        DWARF_LOC (uc_regs + GREGS_LNK, 0);
      c->dwarf.loc[UNW_PPC32_CTR] =
        DWARF_LOC (uc_regs + GREGS_CTR, 0);
      c->dwarf.loc[UNW_PPC32_CCR] =
        DWARF_LOC (uc_regs + GREGS_CCR, 0);
      c->dwarf.loc[UNW_PPC32_XER] =
        DWARF_LOC (uc_regs + GREGS_XER, 0);
      c->dwarf.loc[UNW_PPC32_NIP] =
        DWARF_LOC (uc_regs + GREGS_NIP, 0);

      /* Restore FPRs from the signal context.  FP regs are 8 bytes each. */
      for (i = 0; i < 32; i++)
        c->dwarf.loc[UNW_PPC32_F0 + i] =
          DWARF_FPREG_LOC (&c->dwarf, uc_regs + FREGS_R0 + (i * 8));

      ret = 1;
    }
  return ret;
}
