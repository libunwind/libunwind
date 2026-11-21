/* libunwind - a platform-independent unwind library
   Copyright (C) 2002-2003 Hewlett-Packard Co
        Contributed by David Mosberger-Tang <davidm@hpl.hp.com>

   Modified for x86_64 by Max Asbock <masbock@us.ibm.com>

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

#ifdef HAVE_ASM_VSYSCALL_H
#include <asm/vsyscall.h>
#endif

#include "libunwind_i.h"
#include "unwind_i.h"
#include "ucontext_i.h"

#include <sys/syscall.h>

HIDDEN void
tdep_fetch_frame (struct dwarf_cursor *dw, unw_word_t ip UNUSED, int need_unwind_info UNUSED)
{
  struct cursor *c = (struct cursor *) dw;
  assert(! need_unwind_info || dw->pi_valid);
  assert(! need_unwind_info || dw->pi.unwind_info);
  if (dw->pi_valid
      && dw->pi.unwind_info
      && ((struct dwarf_cie_info *) dw->pi.unwind_info)->signal_frame)
    c->sigcontext_format = X86_64_SCF_LINUX_RT_SIGFRAME;
  else
    c->sigcontext_format = X86_64_SCF_NONE;

  Debug(5, "fetch frame ip=0x%lx cfa=0x%lx format=%d\n",
        dw->ip, dw->cfa, c->sigcontext_format);
}

HIDDEN int
tdep_cache_frame (struct dwarf_cursor *dw)
{
  struct cursor *c = (struct cursor *) dw;

  Debug(5, "cache frame ip=0x%lx cfa=0x%lx format=%d\n",
        dw->ip, dw->cfa, c->sigcontext_format);
  return c->sigcontext_format;
}

HIDDEN void
tdep_reuse_frame (struct dwarf_cursor *dw, int frame)
{
  struct cursor *c = (struct cursor *) dw;
  c->sigcontext_format = frame;
  if (c->sigcontext_format == X86_64_SCF_LINUX_RT_SIGFRAME)
  {
    c->frame_info.frame_type = UNW_X86_64_FRAME_SIGRETURN;
    /* Offset from cfa to ucontext_t in signal frame.  */
    c->frame_info.cfa_reg_offset = 0;
    c->sigcontext_addr = dw->cfa;
  }

  Debug(5, "reuse frame ip=0x%lx cfa=0x%lx format=%d addr=0x%lx offset=%+d\n",
        dw->ip, dw->cfa, c->sigcontext_format, c->sigcontext_addr,
        (c->sigcontext_format == X86_64_SCF_LINUX_RT_SIGFRAME
         ? c->frame_info.cfa_reg_offset : 0));
}

/* Check whether the cursor's IP points at the rt_sigreturn trampoline:

     48 c7 c0 0f 00 00 00    mov $0xf,%rax
     0f 05                   syscall

   glibc annotates its trampoline with DWARF CFI carrying the 'S'
   augmentation, which is how tdep_fetch_frame() normally detects signal
   frames.  Other C libraries, such as musl, provide no unwind info for
   the trampoline, so fall back to matching its instructions.  The IP may
   be bad, e.g. after a jump through a bad function pointer, so the reads
   are validated for the local address space, which does not validate by
   default.  */
static int
is_rt_sigreturn_trampoline (struct cursor *c)
{
  unw_accessors_t *a = unw_get_accessors_int (c->dwarf.as);
  unw_word_t w0, w1;
  int validate = 0, ret;

  if (c->dwarf.as == unw_local_addr_space)
    {
      validate = dwarf_get_validate (&c->dwarf);
      dwarf_set_validate (&c->dwarf, 1);
    }

  ret = (*a->access_mem) (c->dwarf.as, c->dwarf.ip, &w0, 0,
                          c->dwarf.as_arg) >= 0
        && (*a->access_mem) (c->dwarf.as, c->dwarf.ip + 8, &w1, 0,
                             c->dwarf.as_arg) >= 0
        && w0 == 0x0f0000000fc0c748 && (w1 & 0xff) == 0x05;

  if (c->dwarf.as == unw_local_addr_space)
    dwarf_set_validate (&c->dwarf, validate);

  return ret;
}

int
unw_is_signal_frame (unw_cursor_t *cursor)
{
  struct cursor *c = (struct cursor *) cursor;

  return c->sigcontext_format != X86_64_SCF_NONE
         || is_rt_sigreturn_trampoline (c);
}

/* Step through a signal trampoline that has no DWARF unwind info.  */
HIDDEN int
x86_64_handle_signal_frame (unw_cursor_t *cursor)
{
  struct cursor *c = (struct cursor *) cursor;
  unw_word_t uc;
  int i, ret;

  if (c->sigcontext_format == X86_64_SCF_NONE
      && is_rt_sigreturn_trampoline (c))
    {
      c->sigcontext_format = X86_64_SCF_LINUX_RT_SIGFRAME;
      /* On entry to the trampoline the return address has been popped,
         so the stack pointer (the CFA of the frame above) points at the
         ucontext_t of the rt_sigframe.  */
      c->sigcontext_addr = c->dwarf.cfa;
    }

  if (c->sigcontext_format != X86_64_SCF_LINUX_RT_SIGFRAME)
    return -UNW_EBADFRAME;

  uc = c->sigcontext_addr;
  Debug (1, "signal frame without unwind info, ucontext at 0x%lx\n",
         (long) uc);

  ret = dwarf_get (&c->dwarf, DWARF_LOC (uc + UC_MCONTEXT_GREGS_RSP, 0),
                   &c->dwarf.cfa);
  if (ret < 0)
    {
      Debug (2, "returning %d\n", ret);
      return ret;
    }

  for (i = 0; i < DWARF_NUM_PRESERVED_REGS; ++i)
    c->dwarf.loc[i] = DWARF_NULL_LOC;

  c->dwarf.loc[RAX] = DWARF_LOC (uc + UC_MCONTEXT_GREGS_RAX, 0);
  c->dwarf.loc[RDX] = DWARF_LOC (uc + UC_MCONTEXT_GREGS_RDX, 0);
  c->dwarf.loc[RCX] = DWARF_LOC (uc + UC_MCONTEXT_GREGS_RCX, 0);
  c->dwarf.loc[RBX] = DWARF_LOC (uc + UC_MCONTEXT_GREGS_RBX, 0);
  c->dwarf.loc[RSI] = DWARF_LOC (uc + UC_MCONTEXT_GREGS_RSI, 0);
  c->dwarf.loc[RDI] = DWARF_LOC (uc + UC_MCONTEXT_GREGS_RDI, 0);
  c->dwarf.loc[RBP] = DWARF_LOC (uc + UC_MCONTEXT_GREGS_RBP, 0);
  c->dwarf.loc[RSP] = DWARF_LOC (uc + UC_MCONTEXT_GREGS_RSP, 0);
  c->dwarf.loc[ R8] = DWARF_LOC (uc + UC_MCONTEXT_GREGS_R8, 0);
  c->dwarf.loc[ R9] = DWARF_LOC (uc + UC_MCONTEXT_GREGS_R9, 0);
  c->dwarf.loc[R10] = DWARF_LOC (uc + UC_MCONTEXT_GREGS_R10, 0);
  c->dwarf.loc[R11] = DWARF_LOC (uc + UC_MCONTEXT_GREGS_R11, 0);
  c->dwarf.loc[R12] = DWARF_LOC (uc + UC_MCONTEXT_GREGS_R12, 0);
  c->dwarf.loc[R13] = DWARF_LOC (uc + UC_MCONTEXT_GREGS_R13, 0);
  c->dwarf.loc[R14] = DWARF_LOC (uc + UC_MCONTEXT_GREGS_R14, 0);
  c->dwarf.loc[R15] = DWARF_LOC (uc + UC_MCONTEXT_GREGS_R15, 0);
  c->dwarf.loc[RIP] = DWARF_LOC (uc + UC_MCONTEXT_GREGS_RIP, 0);

  /* Let unw_backtrace() cache this frame like a DWARF-described one.  */
  c->frame_info.frame_type = UNW_X86_64_FRAME_SIGRETURN;
  c->frame_info.cfa_reg_offset = 0;

  return 0;
}

#ifndef UNW_REMOTE_ONLY
HIDDEN void *
x86_64_r_uc_addr (ucontext_t *uc, int reg)
{
  /* NOTE: common_init() in init.h inlines these for fast path access. */
  void *addr;

  switch (reg)
    {
    case UNW_X86_64_R8: addr = &uc->uc_mcontext.gregs[REG_R8]; break;
    case UNW_X86_64_R9: addr = &uc->uc_mcontext.gregs[REG_R9]; break;
    case UNW_X86_64_R10: addr = &uc->uc_mcontext.gregs[REG_R10]; break;
    case UNW_X86_64_R11: addr = &uc->uc_mcontext.gregs[REG_R11]; break;
    case UNW_X86_64_R12: addr = &uc->uc_mcontext.gregs[REG_R12]; break;
    case UNW_X86_64_R13: addr = &uc->uc_mcontext.gregs[REG_R13]; break;
    case UNW_X86_64_R14: addr = &uc->uc_mcontext.gregs[REG_R14]; break;
    case UNW_X86_64_R15: addr = &uc->uc_mcontext.gregs[REG_R15]; break;
    case UNW_X86_64_RDI: addr = &uc->uc_mcontext.gregs[REG_RDI]; break;
    case UNW_X86_64_RSI: addr = &uc->uc_mcontext.gregs[REG_RSI]; break;
    case UNW_X86_64_RBP: addr = &uc->uc_mcontext.gregs[REG_RBP]; break;
    case UNW_X86_64_RBX: addr = &uc->uc_mcontext.gregs[REG_RBX]; break;
    case UNW_X86_64_RDX: addr = &uc->uc_mcontext.gregs[REG_RDX]; break;
    case UNW_X86_64_RAX: addr = &uc->uc_mcontext.gregs[REG_RAX]; break;
    case UNW_X86_64_RCX: addr = &uc->uc_mcontext.gregs[REG_RCX]; break;
    case UNW_X86_64_RSP: addr = &uc->uc_mcontext.gregs[REG_RSP]; break;
    case UNW_X86_64_RIP: addr = &uc->uc_mcontext.gregs[REG_RIP]; break;

    default:
      addr = NULL;
    }
  return addr;
}

/* sigreturn() is a no-op on x86_64 glibc.  */
HIDDEN NORETURN void
x86_64_sigreturn (unw_cursor_t *cursor)
{
  struct cursor *c = (struct cursor *) cursor;
  struct sigcontext *sc = (struct sigcontext *) c->sigcontext_addr;
  mcontext_t *sc_mcontext = &((ucontext_t*)sc)->uc_mcontext;
  /* Copy in saved uc - all preserved regs are at the start of sigcontext */
  memcpy(sc_mcontext, &dwarf_get_uc(&c->dwarf)->uc_mcontext,
         DWARF_NUM_PRESERVED_REGS * sizeof(unw_word_t));

  Debug (8, "resuming at ip=%llx via sigreturn(%p)\n",
             (unsigned long long) c->dwarf.ip, sc);

  /* Pop 4 shadow stack frames:

#0  _Ux86_64_sigreturn (cursor=0x7fffffffc990) at x86_64/Gos-linux.c:144
#1  0x00007ffff7f9d23f in _Ux86_64_local_resume (
    as=0x7ffff7fb63c0 <local_addr_space>, cursor=0x7fffffffc990, arg=0x2)
    at x86_64/Gresume.c:50
#2  0x00007ffff7f9d44b in _Ux86_64_resume (cursor=0x7fffffffc990)
    at x86_64/Gresume.c:123
#3  0x000000000040094b in handler (sig=10) at Gtest-resume-sig.c:127
#4  <signal handler called>
#5  0x00007ffff7d80e7b in kill () from /lib64/libc.so.6

   */
  POP_SHADOW_STACK_FRAMES (4);

  __asm__ __volatile__ ("mov %0, %%rsp;"
                        "mov %1, %%rax;"
                        "syscall"
                        :: "r"((uint64_t)sc), "i"(SYS_rt_sigreturn)
                        : "memory");
  abort();
}

#endif

static int
is_vsyscall (struct dwarf_cursor *c UNUSED)
{
#if defined(VSYSCALL_START) && defined(VSYSCALL_END)
  return c->ip >= VSYSCALL_START && c->ip < VSYSCALL_END;
#elif defined(VSYSCALL_ADDR)
  /* Linux 3.16 removes `VSYSCALL_START` and `VSYSCALL_END`.  Assume
     a single page is mapped for vsyscalls.  */
  return c->ip >= VSYSCALL_ADDR && c->ip < VSYSCALL_ADDR + unw_page_size;
#else
  return 0;
#endif
}

HIDDEN int
x86_64_os_step(struct cursor *c)
{
  if (is_vsyscall (&c->dwarf))
    {
      Debug (2, "in vsyscall region\n");
      c->frame_info.cfa_reg_offset = 8;
      c->frame_info.cfa_reg_rsp = -1;
      c->frame_info.frame_type = UNW_X86_64_FRAME_GUESSED;
      c->dwarf.loc[RIP] = DWARF_LOC (c->dwarf.cfa, 0);
      c->dwarf.cfa += 8;
      return (1);
    }
  return (0);
}
