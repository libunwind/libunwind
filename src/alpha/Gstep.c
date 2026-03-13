/* libunwind - a platform-independent unwind library
   Copyright (C) 2002-2004 Hewlett-Packard Co
        Contributed by David Mosberger-Tang <davidm@hpl.hp.com>

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

#include "unwind_i.h"
#include <signal.h>

/* Offsets within struct sigcontext */
#define SC_PC           16
#define SC_REGS         32
#define SC_FPREGS       296

static int
alpha_handle_signal_frame (unw_cursor_t *cursor)
{
  struct cursor *c = (struct cursor *) cursor;
  int ret, i;
  unw_word_t sc_addr, sp;

  ret = dwarf_get (&c->dwarf, c->dwarf.loc[UNW_ALPHA_R30], &sp);
  if (ret < 0)
    return ret;

  /* Save the SP and PC to be able to return execution at this point
     later in time (unw_resume).  */
  c->sigcontext_sp = sp;
  c->sigcontext_pc = c->dwarf.ip;

  switch (c->sigcontext_format)
    {
    case ALPHA_SCF_LINUX_SIGFRAME:
      /* struct sigframe { struct sigcontext sc; unsigned int retcode[3]; }
         The sigcontext starts at SP.  */
      sc_addr = sp;
      break;
    case ALPHA_SCF_LINUX_RT_SIGFRAME:
      /* struct rt_sigframe { struct siginfo info; struct ucontext uc; ... }
         The kernel asserts: offsetof(struct rt_sigframe, uc.uc_mcontext) == 176
         The uc_mcontext IS a struct sigcontext.  */
      sc_addr = sp + 176;
      break;
    default:
      return -UNW_EUNSPEC;
    }

  c->sigcontext_addr = sc_addr;

  for (i = 0; i < DWARF_NUM_PRESERVED_REGS; ++i)
    c->dwarf.loc[i] = DWARF_NULL_LOC;

  /* Update the dwarf cursor.
     Set the location of the registers to the corresponding addresses of the
     sigcontext structure contents.  */
  for (i = UNW_ALPHA_R0; i <= UNW_ALPHA_R31; ++i)
    c->dwarf.loc[i] = DWARF_MEM_LOC (c, sc_addr + SC_REGS + (i - UNW_ALPHA_R0) * 8);
  for (i = UNW_ALPHA_F0; i <= UNW_ALPHA_F31; ++i)
    c->dwarf.loc[i] = DWARF_MEM_LOC (c, sc_addr + SC_FPREGS + (i - UNW_ALPHA_F0) * 8);

  c->dwarf.loc[UNW_ALPHA_PC] = DWARF_MEM_LOC (c, sc_addr + SC_PC);

  /* Set SP/CFA and PC/IP.  */
  dwarf_get (&c->dwarf, c->dwarf.loc[UNW_ALPHA_R30], &c->dwarf.cfa);
  dwarf_get (&c->dwarf, c->dwarf.loc[UNW_ALPHA_PC], &c->dwarf.ip);

  c->dwarf.pi_valid = 0;
  c->dwarf.use_prev_instr = 0;

  return 1;
}

int
unw_step (unw_cursor_t *cursor)
{
  struct cursor *c = (struct cursor *) cursor;
  int ret = 0, val = c->validate, sig;

#if CONSERVATIVE_CHECKS
  c->validate = 1;
#endif

  Debug (1, "(cursor=%p, ip=0x%016lx, cfa=0x%016lx)\n",
         c, c->dwarf.ip, c->dwarf.cfa);

  /* Try DWARF-based unwinding... */
  c->sigcontext_format = ALPHA_SCF_NONE;
  ret = dwarf_step (&c->dwarf);

#if CONSERVATIVE_CHECKS
  c->validate = val;
#endif

  if (unlikely (ret == -UNW_ENOINFO))
    {
      /* Memory accesses here are quite likely to be unsafe. */
      c->validate = 1;

      /* Check if this is a signal frame. */
      sig = unw_is_signal_frame (cursor);
      if (sig > 0)
        {
          c->sigcontext_format = sig;
          ret = alpha_handle_signal_frame (cursor);
        }
      else
        {
          c->dwarf.ip = 0;
          ret = 0;
        }

      c->validate = val;
      return ret;
    }

  if (unlikely (ret > 0 && c->dwarf.ip == 0))
    return 0;

  return ret;
}
