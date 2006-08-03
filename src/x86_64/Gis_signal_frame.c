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

#include "unwind_i.h"

#ifdef __linux__
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
  as->validate = 1;	/* Don't trust the ip */
  arg = c->dwarf.as_arg;

  /* Check if RIP points at sigreturn sequence.
     on x86_64 Linux that is (see libc.so):
     48 c7 c0 0f 00 00 00 mov $0xf,%rax
     0f 05                syscall
     66                   data16
  */

  ip = c->dwarf.ip;
  if ((ret = (*a->access_mem) (as, ip, &w0, 0, arg)) < 0
      || (ret = (*a->access_mem) (as, ip + 8, &w1, 0, arg)) < 0)
    return 0;
  w1 &= 0xffffffff;
  return (w0 == 0x0f0000000fc0c748 && w1 == 0x66666605);
}

#else /* __linux__ */

PROTECTED int
unw_is_signal_frame (unw_cursor_t *cursor)
{
  printf ("%s: implement me\n", __FUNCTION__);
  return -UNW_ENOINFO;
}
#endif /* __linux__ */
