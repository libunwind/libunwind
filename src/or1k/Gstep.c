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

#include <signal.h>

#include "unwind_i.h"
#include "offsets.h"

static int
or1k_handle_signal_frame (unw_cursor_t *cursor)
{
  struct cursor *c = (struct cursor *) cursor;
  unw_word_t sc_addr;
  int i, ret;

  /* Save the SP and PC to be able to return execution at this point
     later in time (unw_resume).  */
  c->sigcontext_sp = c->dwarf.cfa;
  c->sigcontext_pc = c->dwarf.ip;

  /* The trampoline frame's CFA is the base of the rt_sigframe.  Skip the
     siginfo_t to reach the ucontext.  */
  c->sigcontext_format = OR1K_SCF_LINUX_RT_SIGFRAME;
  sc_addr = c->dwarf.cfa + sizeof (siginfo_t) + LINUX_UC_MCONTEXT_OFF;
  c->sigcontext_addr = sc_addr;

  for (i = 0; i < DWARF_NUM_PRESERVED_REGS; ++i)
    c->dwarf.loc[i] = DWARF_NULL_LOC;

  /* Point every register at its slot in the sigcontext.  */
  for (i = UNW_OR1K_R0; i <= UNW_OR1K_R31; ++i)
    c->dwarf.loc[i] = DWARF_LOC (sc_addr + LINUX_SC_GPR_OFF
                                 + (i - UNW_OR1K_R0) * sizeof (unw_word_t), 0);

  c->dwarf.loc[UNW_OR1K_PC] = DWARF_LOC (sc_addr + LINUX_SC_PC_OFF, 0);

  /* Set SP/CFA and PC/IP.  */
  ret = dwarf_get (&c->dwarf, c->dwarf.loc[UNW_OR1K_R1], &c->dwarf.cfa);
  if (ret < 0)
    return ret;

  ret = dwarf_get (&c->dwarf, c->dwarf.loc[UNW_OR1K_PC], &c->dwarf.ip);
  if (ret < 0)
    return ret;

  c->dwarf.pi_valid = 0;

  /* The IP is the interrupted PC, not a return address, so the next frame
     must be looked up at the IP itself rather than at IP - 1.  */
  c->dwarf.use_prev_instr = 0;

  return 1;
}

int
unw_step (unw_cursor_t *cursor)
{
  struct cursor *c = (struct cursor *) cursor;
  int ret;

  Debug (1, "(cursor=%p)\n", c);

  if (unw_is_signal_frame (cursor) > 0)
    return or1k_handle_signal_frame (cursor);

  c->sigcontext_format = OR1K_SCF_NONE;
  ret = dwarf_step (&c->dwarf);

  if (unlikely (ret == -UNW_ESTOPUNWIND))
    return ret;

  if (unlikely (ret < 0))
    return 0;

  return (c->dwarf.ip == 0) ? 0 : 1;
}
