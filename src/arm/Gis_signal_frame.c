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

#include <stdio.h>
#include "unwind_i.h"

#ifdef __linux__
#include <sys/syscall.h>

#define ARM_SIGRETURN (0xef000000UL |(__NR_sigreturn)|(__NR_OABI_SYSCALL_BASE))
#define THUMB_SIGRETURN (0xdf00UL  << 16 | 0x2700 | (__NR_sigreturn - __NR_SYSCALL_BASE))
#define MOV_R7_SIGRETURN (0xe3a07000UL  | (__NR_sigreturn - __NR_SYSCALL_BASE))
#endif

PROTECTED int
unw_is_signal_frame (unw_cursor_t *cursor)
{
#ifdef __linux__
  struct cursor *c = (struct cursor *) cursor;
  unw_word_t w0, ip;
  unw_addr_space_t as;
  unw_accessors_t *a;
  void *arg;
  int ret;

  as = c->dwarf.as;
  a = unw_get_accessors (as);
  arg = c->dwarf.as_arg;

  ip = c->dwarf.ip;
  if ((ret = (*a->access_mem) (as, ip, &w0, 0, arg)) < 0)
    return ret;
  ret = (w0 == ARM_SIGRETURN) || (w0 == MOV_R7_SIGRETURN)
    || (w0 == THUMB_SIGRETURN);
  fprintf (stderr, "w0=%8.8x ret=%d\n", w0, ret);
  Debug (16, "returning %d\n", ret);
  return ret;
#else
  printf ("%s: implement me\n", __FUNCTION__);
  return -UNW_ENOINFO;
#endif
}
