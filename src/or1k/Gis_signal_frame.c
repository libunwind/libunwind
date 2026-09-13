/* libunwind - a platform-independent unwind library
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

/* OpenRISC has no vDSO or non-RT signal frame.  The rt signal frame
   contains a two-instruction rt_sigreturn trampoline on the user stack.  */

#define OR1K_RT_SIGRETURN_INSN0 0xa960008b      /* l.ori r11, r0, __NR_rt_sigreturn */
#define OR1K_RT_SIGRETURN_INSN1 0x20000001      /* l.sys 1 */

int
unw_is_signal_frame (unw_cursor_t *cursor)
{
#ifdef __linux__
  struct cursor *c = (struct cursor *) cursor;
  unw_word_t w0, w1, ip;
  unw_addr_space_t as;
  unw_accessors_t *a;
  void *arg;
  int ret;

  as = c->dwarf.as;
  a = unw_get_accessors_int (as);
  arg = c->dwarf.as_arg;
  ip = c->dwarf.ip;

  ret = (*a->access_mem) (as, ip, &w0, 0, arg);
  if (ret < 0)
    return ret;

  ret = (*a->access_mem) (as, ip + 4, &w1, 0, arg);
  if (ret < 0)
    return ret;

  if (w0 == OR1K_RT_SIGRETURN_INSN0 && w1 == OR1K_RT_SIGRETURN_INSN1)
    return 1;

  return 0;
#else
  return -UNW_ENOINFO;
#endif
}
