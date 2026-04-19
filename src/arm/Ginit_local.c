/* libunwind - a platform-independent unwind library
   Copyright (C) 2008 CodeSourcery
   Copyright 2011 Linaro Limited

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
#include "offsets.h"

#ifdef UNW_REMOTE_ONLY

int
unw_init_local (unw_cursor_t *cursor, unw_context_t *uc)
{
  return -UNW_EINVAL;
}

#else /* !UNW_REMOTE_ONLY */

static int
unw_init_local_common (unw_cursor_t *cursor, unw_context_t *uc, unsigned use_prev_instr)
{
  struct cursor *c = (struct cursor *) cursor;

  if (!atomic_load(&tdep_init_done))
    tdep_init ();

  Debug (1, "(cursor=%p)\n", c);

  c->dwarf.as = unw_local_addr_space;
  c->dwarf.as_arg = c;
  c->uc = uc;

  return common_init (c, use_prev_instr);
}

#ifdef __linux__
static int
unw_init_local_signal (unw_cursor_t *cursor, unw_context_t *uc)
{
  struct cursor *c = (struct cursor *) cursor;
  int ret, i;

  if (!atomic_load(&tdep_init_done))
    tdep_init ();

  Debug (1, "(cursor=%p)\n", c);

  c->dwarf.as = unw_local_addr_space;
  c->dwarf.as_arg = c;
  c->uc = uc;

  /* uc is a ucontext_t* from the signal handler; its layout differs from
     unw_tdep_context_t, so we cannot use common_init/uc_addr here.
     Compute the sigcontext base and set each register location directly,
     mirroring arm_handle_signal_frame in Gos-linux.c.  */
  unw_word_t sc_addr = (unw_word_t) uc + LINUX_UC_MCONTEXT_OFF;

  for (i = 0; i < DWARF_NUM_PRESERVED_REGS; ++i)
    c->dwarf.loc[i] = DWARF_NULL_LOC;

  c->dwarf.loc[UNW_ARM_R0]  = DWARF_LOC (sc_addr + LINUX_SC_R0_OFF,  0);
  c->dwarf.loc[UNW_ARM_R1]  = DWARF_LOC (sc_addr + LINUX_SC_R1_OFF,  0);
  c->dwarf.loc[UNW_ARM_R2]  = DWARF_LOC (sc_addr + LINUX_SC_R2_OFF,  0);
  c->dwarf.loc[UNW_ARM_R3]  = DWARF_LOC (sc_addr + LINUX_SC_R3_OFF,  0);
  c->dwarf.loc[UNW_ARM_R4]  = DWARF_LOC (sc_addr + LINUX_SC_R4_OFF,  0);
  c->dwarf.loc[UNW_ARM_R5]  = DWARF_LOC (sc_addr + LINUX_SC_R5_OFF,  0);
  c->dwarf.loc[UNW_ARM_R6]  = DWARF_LOC (sc_addr + LINUX_SC_R6_OFF,  0);
  c->dwarf.loc[UNW_ARM_R7]  = DWARF_LOC (sc_addr + LINUX_SC_R7_OFF,  0);
  c->dwarf.loc[UNW_ARM_R8]  = DWARF_LOC (sc_addr + LINUX_SC_R8_OFF,  0);
  c->dwarf.loc[UNW_ARM_R9]  = DWARF_LOC (sc_addr + LINUX_SC_R9_OFF,  0);
  c->dwarf.loc[UNW_ARM_R10] = DWARF_LOC (sc_addr + LINUX_SC_R10_OFF, 0);
  c->dwarf.loc[UNW_ARM_R11] = DWARF_LOC (sc_addr + LINUX_SC_FP_OFF,  0);
  c->dwarf.loc[UNW_ARM_R12] = DWARF_LOC (sc_addr + LINUX_SC_IP_OFF,  0);
  c->dwarf.loc[UNW_ARM_R13] = DWARF_LOC (sc_addr + LINUX_SC_SP_OFF,  0);
  c->dwarf.loc[UNW_ARM_R14] = DWARF_LOC (sc_addr + LINUX_SC_LR_OFF,  0);
  c->dwarf.loc[UNW_ARM_R15] = DWARF_LOC (sc_addr + LINUX_SC_PC_OFF,  0);

  ret = dwarf_get (&c->dwarf, c->dwarf.loc[UNW_ARM_R15], &c->dwarf.ip);
  if (ret < 0)
    return ret;

  ret = dwarf_get (&c->dwarf, c->dwarf.loc[UNW_ARM_R13], &c->dwarf.cfa);
  if (ret < 0)
    return ret;

  c->sigcontext_format = ARM_SCF_NONE;
  c->sigcontext_addr = 0;
  c->sigcontext_sp = 0;
  c->sigcontext_pc = 0;

  c->dwarf.args_size = 0;
  c->dwarf.stash_frames = 0;
  c->dwarf.use_prev_instr = 0;
  c->dwarf.pi_valid = 0;
  c->dwarf.pi_is_dynamic = 0;
  c->dwarf.hint = 0;
  c->dwarf.prev_rs = 0;

  return 0;
}
#endif /* __linux__ */

int
unw_init_local (unw_cursor_t *cursor, unw_context_t *uc)
{
  return unw_init_local_common(cursor, uc, 1);
}

int
unw_init_local2 (unw_cursor_t *cursor, unw_context_t *uc, int flag)
{
  if (!flag)
    {
      return unw_init_local_common(cursor, uc, 1);
    }
  else if (flag == UNW_INIT_SIGNAL_FRAME)
    {
#ifdef __linux__
      return unw_init_local_signal (cursor, uc);
#else
      return unw_init_local_common(cursor, uc, 0);
#endif
    }
  else
    {
      return -UNW_EINVAL;
    }
}

#endif /* !UNW_REMOTE_ONLY */
