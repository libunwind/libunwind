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

#include <signal.h>
#include <stddef.h>
#include "unwind_i.h"
#include "ucontext_i.h"

// TODO: got no idea here, need someone who understands this code better
//       also, Haiku doesn't have a stable syscall interface, IIRC

int
unw_is_signal_frame (unw_cursor_t *cursor)
{
  /* XXXKIB */
  struct cursor *c = (struct cursor *) cursor;
  unw_word_t w0, w1, b0, b1, ip;
  unw_addr_space_t as;
  unw_accessors_t *a;
  void *arg;
  int ret;

  as = c->dwarf.as;
  a = unw_get_accessors_int (as);
  arg = c->dwarf.as_arg;
  ip = c->dwarf.ip;
  c->sigcontext_format = X86_64_SCF_NONE;

  /* Check if RIP points at sigreturn sequence.
48 c7 c0 3f 00 00 00    mov     $SYSCALL_RESTORE_SIGNAL_FRAME, %rax
4c 89 e7                mov     %r12, %rdi
0f 05                   syscall
  */
  if ((ret = (*a->access_mem) (as, ip, &w0, 0, arg)) < 0
      || (ret = (*a->access_mem) (as, ip + 8, &w1, 0, arg)) < 0)
    return 0;
  w1 &= 0xffffffff;
  if (w0 == 0x4c0000003fc0c748 &&
      w1 == 0x00000000050fe789)
   {
   	 Debug (12, "haiku sigframe\n");
     c->sigcontext_format = X86_64_SCF_HAIKU_SIGFRAME;
     return (c->sigcontext_format);
   }

  /* Now these checks never occur... interesting */

  /* Check if RIP points at standard syscall sequence with RCX
49 89 ca             mov    %rcx,%r10
48 c7 c0 .. .. 00 00 mov    $N,%rax
0f 05                syscall
  */
  if ((ret = (*a->access_mem) (as, ip - 12, &b0, 0, arg)) < 0
      || (ret = (*a->access_mem) (as, ip - 12 + 8, &b1, 0, arg)) < 0)
    return (0);
  Debug (12, "b0.0 0x%08lx\n", b0);
  Debug (12, "b1.0 0x%08lx\n", b1);
  b0 &= 0x0000ffffffffffff;
  b1 &= 0x000000ffffffffff;
  if (b0 == 0x0000c0c748ca8949 &&
      b1 == 0x000000c3050f0000)
   {
    c->sigcontext_format = X86_64_SCF_HAIKU_SYSCALL;
    return (c->sigcontext_format);
   }
  /* Check if RIP points at standard syscall sequence without RCX
48 c7 c0 .. .. 00 00 mov    $N,%rax
0f 05                syscall
  */
  if ((ret = (*a->access_mem) (as, ip - 10, &b0, 0, arg)) < 0
    || (ret = (*a->access_mem) (as, ip - 10 + 8, &b1, 0, arg)) < 0)
    return (0);
  Debug (12, "b0.1 0x%08lx\n", b0);
  Debug (12, "b1.1 0x%08lx\n", b1);
  b0 &= 0xffff0000ffffff;
  b1 &= 0x0000000000ffff;
  if (b0 == 0x0f00000000c0c748 &&
      b1 == 0x00000000000005c3)
   {
    c->sigcontext_format = X86_64_SCF_HAIKU_SYSCALL;
    return (c->sigcontext_format);
   }
  return (X86_64_SCF_NONE);
}

HIDDEN int
x86_64_handle_signal_frame (unw_cursor_t *cursor)
{
	#if 0
  struct cursor *c = (struct cursor *) cursor;
  unw_word_t ucontext = c->dwarf.cfa + sizeof (struct sigframe);
  
  if (c->sigcontext_format == X86_64_SCF_HAIKU_SIGFRAME)
   {
    c->sigcontext_addr = c->dwarf.cfa;
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
  else if (c->sigcontext_format == X86_64_SCF_HAIKU_SYSCALL)
   {
   	Debug(1, "sigcontext_format == X86_64_SCF_HAIKU_SYSCALL");
   	// some syscalls don't save RCX...
   	c->dwarf.loc[RCX] = c->dwarf.loc[R10];
    /*  rsp_loc = DWARF_LOC(c->dwarf.cfa - 8, 0);       */
    /*  rbp_loc = c->dwarf.loc[RBP];                    */
    c->dwarf.loc[RIP] = DWARF_LOC (c->dwarf.cfa, 0);
    int ret = dwarf_get (&c->dwarf, c->dwarf.loc[RIP], &c->dwarf.ip);
    Debug (1, "Frame Chain [RIP=0x%Lx] = 0x%Lx\n",
           (unsigned long long) DWARF_GET_LOC (c->dwarf.loc[RIP]),
           (unsigned long long) c->dwarf.ip);
    if (ret < 0)
     {
       Debug (2, "returning %d\n", ret);
       return ret;
     }
    c->dwarf.cfa += 8;
    c->dwarf.use_prev_instr = 1;
    return 1;
   }
  else
    return -UNW_EBADFRAME;
#endif
  Debug (14, "haiku handle signal frame\n");
  return -UNW_EBADFRAME;
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
	#if 0
  struct cursor *c = (struct cursor *) cursor;
  ucontext_t *uc = (ucontext_t *)(c->sigcontext_addr +
    sizeof(struct sigframe));

  uc->uc_mcontext.r8 = c->uc_mcontext.r8;
  uc->uc_mcontext.r9 = c->uc->uc_mcontext.r9;
  uc->uc_mcontext.r10 = c->uc->uc_mcontext.r10;
  uc->uc_mcontext.r11 = c->uc->uc_mcontext.r11;
  uc->uc_mcontext.r12 = c->uc->uc_mcontext.r12;
  uc->uc_mcontext.r13 = c->uc->uc_mcontext.r13;
  uc->uc_mcontext.r14 = c->uc->uc_mcontext.r14;
  uc->uc_mcontext.r15 = c->uc->uc_mcontext.r15;
  uc->uc_mcontext.rdi = c->uc->uc_mcontext.rdi;
  uc->uc_mcontext.rsi = c->uc->uc_mcontext.rsi;
  uc->uc_mcontext.rbp = c->uc->uc_mcontext.rbp;
  uc->uc_mcontext.rbx = c->uc->uc_mcontext.rbx;
  uc->uc_mcontext.rdx = c->uc->uc_mcontext.rdx;
  uc->uc_mcontext.rax = c->uc->uc_mcontext.rax;
  uc->uc_mcontext.rcx = c->uc->uc_mcontext.rcx;
  uc->uc_mcontext.rsp = c->uc->uc_mcontext.rsp;
  uc->uc_mcontext.rip = c->uc->uc_mcontext.rip;

  Debug (8, "resuming at ip=%llx via sigreturn(%p)\n",
             (unsigned long long) c->dwarf.ip, uc);
  sigreturn(uc);
  #endif
  Debug (14, "haiku:x86_64_sigreturn: abort, c->uc is undefined\n");
  abort();
}
#endif
