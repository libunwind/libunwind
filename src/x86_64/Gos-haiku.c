/* libunwind - a platform-independent unwind library
   Copyright (C) 2010 Konstantin Belousov <kib@freebsd.org>

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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <commpage_defs.h>
#include <signal.h>
#include <stddef.h>
#include "unwind_i.h"
#include "ucontext_i.h"


extern void *__gCommPageAddress;

// TODO: got no idea here, need someone who understands this code better
//       also, Haiku doesn't have a stable syscall interface, IIRC

int
unw_is_signal_frame (unw_cursor_t *cursor)
{
  struct cursor *c = (struct cursor *) cursor;
  unw_word_t ip;
  unw_word_t sighandler;

  c->sigcontext_format = X86_64_SCF_NONE;

  ip = c->dwarf.ip;
  sighandler = (((unw_word_t*)__gCommPageAddress)[COMMPAGE_ENTRY_X86_SIGNAL_HANDLER] + (unw_word_t)__gCommPageAddress);
  if (ip == sighandler + 45)
  	c->sigcontext_format = X86_64_SCF_HAIKU_SIGFRAME;

  return (c->sigcontext_format);
}

HIDDEN int
x86_64_handle_signal_frame (unw_cursor_t *cursor)
{
  struct cursor *c = (struct cursor *) cursor;
  unw_word_t ucontext = c->dwarf.cfa;

  if (c->sigcontext_format == X86_64_SCF_HAIKU_SIGFRAME)
  	return -UNW_EBADFRAME;

  c->sigcontext_addr = c->dwarf.cfa + 0x70;

  Debug(1, "signal frame cfa = %lx ucontext = %lx\n",
    (uint64_t)c->dwarf.cfa, (uint64_t)ucontext);

  struct dwarf_loc rsp_loc = DWARF_LOC (ucontext + UC_MCONTEXT_GREGS_RSP, 0);
  int ret = dwarf_get (&c->dwarf, rsp_loc, &c->dwarf.cfa);
  if (ret < 0)
   {
     Debug (2, "haiku returning %d\n", ret);
     return ret;
   }

    c->dwarf.loc[RAX] = DWARF_LOC (ucontext + UC_MCONTEXT_GREGS_RAX, 0);
    c->dwarf.loc[RDX] = DWARF_LOC (ucontext + UC_MCONTEXT_GREGS_RDX, 0);
    c->dwarf.loc[RCX] = DWARF_LOC (ucontext + UC_MCONTEXT_GREGS_RCX, 0);
    c->dwarf.loc[RBX] = DWARF_LOC (ucontext + UC_MCONTEXT_GREGS_RBX, 0);
    c->dwarf.loc[RSI] = DWARF_LOC (ucontext + UC_MCONTEXT_GREGS_RSI, 0);
    c->dwarf.loc[RDI] = DWARF_LOC (ucontext + UC_MCONTEXT_GREGS_RDI, 0);
    c->dwarf.loc[RBP] = DWARF_LOC (ucontext + UC_MCONTEXT_GREGS_RBP, 0);
    c->dwarf.loc[RSP] = DWARF_LOC (ucontext + UC_MCONTEXT_GREGS_RSP, 0);
    c->dwarf.loc[ R8] = DWARF_LOC (ucontext + UC_MCONTEXT_GREGS_R8, 0);
    c->dwarf.loc[ R9] = DWARF_LOC (ucontext + UC_MCONTEXT_GREGS_R9, 0);
    c->dwarf.loc[R10] = DWARF_LOC (ucontext + UC_MCONTEXT_GREGS_R10, 0);
    c->dwarf.loc[R11] = DWARF_LOC (ucontext + UC_MCONTEXT_GREGS_R11, 0);
    c->dwarf.loc[R12] = DWARF_LOC (ucontext + UC_MCONTEXT_GREGS_R12, 0);
    c->dwarf.loc[R13] = DWARF_LOC (ucontext + UC_MCONTEXT_GREGS_R13, 0);
    c->dwarf.loc[R14] = DWARF_LOC (ucontext + UC_MCONTEXT_GREGS_R14, 0);
    c->dwarf.loc[R15] = DWARF_LOC (ucontext + UC_MCONTEXT_GREGS_R15, 0);
    c->dwarf.loc[RIP] = DWARF_LOC (ucontext + UC_MCONTEXT_GREGS_RIP, 0);

    c->dwarf.use_prev_instr = 1;
    return 0;
}

#ifndef UNW_REMOTE_ONLY
HIDDEN void *
x86_64_r_uc_addr (ucontext_t *uc, int reg)
{
  /* NOTE: common_init() in init.h inlines these for fast path access. */
  void *addr;

  Debug (14, "haiku:x86_64_r_uc_addr: %p, %p\n", uc, &(uc->uc_mcontext));

  switch (reg)
    {
    case UNW_X86_64_R8: addr = &uc->uc_mcontext.r8; break;
    case UNW_X86_64_R9: addr = &uc->uc_mcontext.r9; break;
    case UNW_X86_64_R10: addr = &uc->uc_mcontext.r10; break;
    case UNW_X86_64_R11: addr = &uc->uc_mcontext.r11; break;
    case UNW_X86_64_R12: addr = &uc->uc_mcontext.r12; break;
    case UNW_X86_64_R13: addr = &uc->uc_mcontext.r13; break;
    case UNW_X86_64_R14: addr = &uc->uc_mcontext.r14; break;
    case UNW_X86_64_R15: addr = &uc->uc_mcontext.r15; break;
    case UNW_X86_64_RDI: addr = &uc->uc_mcontext.rdi; break;
    case UNW_X86_64_RSI: addr = &uc->uc_mcontext.rsi; break;
    case UNW_X86_64_RBP: addr = &uc->uc_mcontext.rbp; break;
    case UNW_X86_64_RBX: addr = &uc->uc_mcontext.rbx; break;
    case UNW_X86_64_RDX: addr = &uc->uc_mcontext.rdx; break;
    case UNW_X86_64_RAX: addr = &uc->uc_mcontext.rax; break;
    case UNW_X86_64_RCX: addr = &uc->uc_mcontext.rcx; break;
    case UNW_X86_64_RSP: addr = &uc->uc_mcontext.rsp; break;
    case UNW_X86_64_RIP: addr = &uc->uc_mcontext.rip; break;

    default:
      addr = NULL;
    }
  return addr;
}

HIDDEN NORETURN void
x86_64_sigreturn (unw_cursor_t *cursor)
{
  Debug (14, "haiku:x86_64_sigreturn: abort, c->uc is undefined\n");
  abort();
}
#endif
