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

#include <sys/types.h>
#include <signal.h>
#include <ucontext.h>
#include <machine/sigframe.h>

#include "unwind_i.h"
#include "offsets.h"

PROTECTED int
unw_is_signal_frame (unw_cursor_t *cursor)
{
  struct cursor *c = (struct cursor *) cursor;
  unw_word_t w0, w1, w2, w3, w4, w5, ip;
  unw_addr_space_t as;
  unw_accessors_t *a;
  void *arg;
  int ret;

  as = c->dwarf.as;
  a = unw_get_accessors (as);
  arg = c->dwarf.as_arg;

  /* Check if EIP points at sigreturn() sequence.  It can be:
sigcode+4: from amd64 freebsd32 environment
8d 44 24 20		lea    0x20(%esp),%eax
50			push   %eax
b8 a1 01 00 00		mov    $0x1a1,%ea
50			push   %eax
cd 80			int    $0x80

sigcode+4: from real i386
8d 44 24 20		lea    0x20(%esp),%eax
50			push   %eax
f7 40 54 00 02 00	testl  $0x20000,0x54(%eax)
75 03			jne    sigcode+21
8e 68 14		mov    0x14(%eax),%gs
b8 a1 01 00 00		mov    $0x1a1,%eax
50			push   %eax
cd 80			int    $0x80

freebsd4_sigcode+4:
XXX
osigcode:
XXX
  */
  ip = c->dwarf.ip;
  ret = X86_SCF_NONE;
  c->sigcontext_format = ret;
  if ((*a->access_mem) (as, ip, &w0, 0, arg) < 0 ||
      (*a->access_mem) (as, ip + 4, &w1, 0, arg) < 0 ||
      (*a->access_mem) (as, ip + 8, &w2, 0, arg) < 0 ||
      (*a->access_mem) (as, ip + 12, &w3, 0, arg) < 0)
    return ret;
  if (w0 == 0x2024448d && w1 == 0x01a1b850 && w2 == 0xcd500000 &&
      (w3 & 0xff) == 0x80)
    ret = X86_SCF_FREEBSD_SIGFRAME;
  else {
    if ((*a->access_mem) (as, ip + 16, &w4, 0, arg) < 0 ||
	(*a->access_mem) (as, ip + 20, &w5, 0, arg) < 0)
      return ret;
    if (w0 == 0x2024448d && w1 == 0x5440f750 && w2 == 0x75000200 &&
	w3 == 0x14688e03 && w4 == 0x0001a1b8 && w5 == 0x80cd5000)
      ret = X86_SCF_FREEBSD_SIGFRAME;
  }
  Debug (16, "returning %d\n", ret);
  c->sigcontext_format = ret;
  return (ret);
}

PROTECTED int
unw_handle_signal_frame (unw_cursor_t *cursor)
{
  struct cursor *c = (struct cursor *) cursor;
  int ret;

  if (c->sigcontext_format == X86_SCF_FREEBSD_SIGFRAME) {
    struct sigframe *sf;
    uintptr_t uc_addr;
    struct dwarf_loc esp_loc;

    sf = (struct sigframe *)c->dwarf.cfa;
    uc_addr = (uintptr_t)&(sf->sf_uc);

    esp_loc = DWARF_LOC (uc_addr + FREEBSD_UC_MCONTEXT_ESP_OFF, 0);
    ret = dwarf_get (&c->dwarf, esp_loc, &c->dwarf.cfa);
    if (ret < 0)
    {
	    Debug (2, "returning 0\n");
	    return 0;
    }

    c->dwarf.loc[EIP] = DWARF_LOC (uc_addr + FREEBSD_UC_MCONTEXT_EIP_OFF, 0);
    c->dwarf.loc[ESP] = DWARF_LOC (uc_addr + FREEBSD_UC_MCONTEXT_ESP_OFF, 0);
    c->dwarf.loc[EAX] = DWARF_LOC (uc_addr + FREEBSD_UC_MCONTEXT_EAX_OFF, 0);
    c->dwarf.loc[ECX] = DWARF_LOC (uc_addr + FREEBSD_UC_MCONTEXT_ECX_OFF, 0);
    c->dwarf.loc[EDX] = DWARF_LOC (uc_addr + FREEBSD_UC_MCONTEXT_EDX_OFF, 0);
    c->dwarf.loc[EBX] = DWARF_LOC (uc_addr + FREEBSD_UC_MCONTEXT_EBX_OFF, 0);
    c->dwarf.loc[EBP] = DWARF_LOC (uc_addr + FREEBSD_UC_MCONTEXT_EBP_OFF, 0);
    c->dwarf.loc[ESI] = DWARF_LOC (uc_addr + FREEBSD_UC_MCONTEXT_ESI_OFF, 0);
    c->dwarf.loc[EDI] = DWARF_LOC (uc_addr + FREEBSD_UC_MCONTEXT_EDI_OFF, 0);
    c->dwarf.loc[EFLAGS] = DWARF_LOC (uc_addr + FREEBSD_UC_MCONTEXT_EFLAGS_OFF, 0);
    c->dwarf.loc[TRAPNO] = DWARF_LOC (uc_addr + FREEBSD_UC_MCONTEXT_TRAPNO_OFF, 0);
    c->dwarf.loc[ST0] = DWARF_NULL_LOC;
  } else {
    Debug (8, "Gstep: not handling frame format %d\n", c->sigcontext_format);
    abort();
  }
  return 0;
}
