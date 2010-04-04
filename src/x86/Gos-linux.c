/* libunwind - a platform-independent unwind library
   Copyright (C) 2002-2004 Hewlett-Packard Co
	Contributed by David Mosberger-Tang <davidm@hpl.hp.com>

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

PROTECTED int
unw_is_signal_frame (unw_cursor_t *cursor)
{
  struct cursor *c = (struct cursor *) cursor;
  unw_word_t w0, w1, ip;
  unw_addr_space_t as;
  unw_accessors_t *a;
  void *arg;
  int ret;

  as = c->dwarf.as;
  a = unw_get_accessors (as);
  arg = c->dwarf.as_arg;

  /* Check if EIP points at sigreturn() sequence.  On Linux, this is:

    __restore:
	0x58				pop %eax
	0xb8 0x77 0x00 0x00 0x00	movl 0x77,%eax
	0xcd 0x80			int 0x80

     without SA_SIGINFO, and

    __restore_rt:
       0xb8 0xad 0x00 0x00 0x00        movl 0xad,%eax
       0xcd 0x80                       int 0x80
       0x00                            

     if SA_SIGINFO is specified.
  */
  ip = c->dwarf.ip;
  if ((ret = (*a->access_mem) (as, ip, &w0, 0, arg)) < 0
      || (ret = (*a->access_mem) (as, ip + 4, &w1, 0, arg)) < 0)
    return ret;
  ret = ((w0 == 0x0077b858 && w1 == 0x80cd0000)
	 || (w0 == 0x0000adb8 && (w1 & 0xffffff) == 0x80cd00));
  Debug (16, "returning %d\n", ret);
  return ret;
}

PROTECTED int
unw_handle_signal_frame (unw_cursor_t *cursor)
{
  struct cursor *c = (struct cursor *) cursor;
  int ret;

  /* c->esp points at the arguments to the handler.  Without
     SA_SIGINFO, the arguments consist of a signal number
     followed by a struct sigcontext.  With SA_SIGINFO, the
     arguments consist a signal number, a siginfo *, and a
     ucontext *. */
  unw_word_t sc_addr;
  unw_word_t siginfo_ptr_addr = c->dwarf.cfa + 4;
  unw_word_t sigcontext_ptr_addr = c->dwarf.cfa + 8;
  unw_word_t siginfo_ptr, sigcontext_ptr;
  struct dwarf_loc esp_loc, siginfo_ptr_loc, sigcontext_ptr_loc;

  siginfo_ptr_loc = DWARF_LOC (siginfo_ptr_addr, 0);
  sigcontext_ptr_loc = DWARF_LOC (sigcontext_ptr_addr, 0);
  ret = (dwarf_get (&c->dwarf, siginfo_ptr_loc, &siginfo_ptr)
	 | dwarf_get (&c->dwarf, sigcontext_ptr_loc, &sigcontext_ptr));
  if (ret < 0)
    {
      Debug (2, "returning 0\n");
      return 0;
    }
  if (siginfo_ptr < c->dwarf.cfa
      || siginfo_ptr > c->dwarf.cfa + 256
      || sigcontext_ptr < c->dwarf.cfa
      || sigcontext_ptr > c->dwarf.cfa + 256)
    {
      /* Not plausible for SA_SIGINFO signal */
      c->sigcontext_format = X86_SCF_LINUX_SIGFRAME;
      c->sigcontext_addr = sc_addr = c->dwarf.cfa + 4;
    }
  else
    {
      /* If SA_SIGINFO were not specified, we actually read
	 various segment pointers instead.  We believe that at
	 least fs and _fsh are always zero for linux, so it is
	 not just unlikely, but impossible that we would end
	 up here. */
      c->sigcontext_format = X86_SCF_LINUX_RT_SIGFRAME;
      c->sigcontext_addr = sigcontext_ptr;
      sc_addr = sigcontext_ptr + LINUX_UC_MCONTEXT_OFF;
    }
  esp_loc = DWARF_LOC (sc_addr + LINUX_SC_ESP_OFF, 0);
  ret = dwarf_get (&c->dwarf, esp_loc, &c->dwarf.cfa);
  if (ret < 0)
    {
      Debug (2, "returning 0\n");
      return 0;
    }

  c->dwarf.loc[EAX] = DWARF_LOC (sc_addr + LINUX_SC_EAX_OFF, 0);
  c->dwarf.loc[ECX] = DWARF_LOC (sc_addr + LINUX_SC_ECX_OFF, 0);
  c->dwarf.loc[EDX] = DWARF_LOC (sc_addr + LINUX_SC_EDX_OFF, 0);
  c->dwarf.loc[EBX] = DWARF_LOC (sc_addr + LINUX_SC_EBX_OFF, 0);
  c->dwarf.loc[EBP] = DWARF_LOC (sc_addr + LINUX_SC_EBP_OFF, 0);
  c->dwarf.loc[ESI] = DWARF_LOC (sc_addr + LINUX_SC_ESI_OFF, 0);
  c->dwarf.loc[EDI] = DWARF_LOC (sc_addr + LINUX_SC_EDI_OFF, 0);
  c->dwarf.loc[EFLAGS] = DWARF_NULL_LOC;
  c->dwarf.loc[TRAPNO] = DWARF_NULL_LOC;
  c->dwarf.loc[ST0] = DWARF_NULL_LOC;
  c->dwarf.loc[EIP] = DWARF_LOC (sc_addr + LINUX_SC_EIP_OFF, 0);
  c->dwarf.loc[ESP] = DWARF_LOC (sc_addr + LINUX_SC_ESP_OFF, 0);

  return 0;
}
