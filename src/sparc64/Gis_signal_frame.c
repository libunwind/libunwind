/* libunwind - a platform-independent unwind library
   Copyright (C) 2014 Oracle Inc
   Contributed by
     Jose E. Marchesi <jose.marchesi@oracle.com>

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

#include <libunwind_i.h>

int
unw_is_signal_frame (unw_cursor_t * cursor)
{
  struct cursor *c = (struct cursor *) cursor;
  unw_word_t w, i0, i1, ip;
  unw_addr_space_t as;
  unw_accessors_t *a;
  void *arg;
  int ret;

  as = c->dwarf.as;
  as->validate = 1;		/* Don't trust the ip */
  arg = c->dwarf.as_arg;

  /* Check if return address points at sigreturn sequence.
     on sparc64 Linux that is (see libc.so):
     0x82102065  mov __NR_rt_sigreturn, %g1
     0x91d0206d  ta 0x6d
   */

  ip = c->dwarf.ip;
  if (ip == 0)
    return 0;

  /* Read up one 8-byte words at the IP.  */

  a = unw_get_accessors (as);
  if ((ret = (*a->access_mem) (as, ip, &w, 0, arg)) < 0)
    return 0;

  i0 = w >> 32;
  i1 = w & 0xffffffffUL;

  return (i0 == 0x82102065 && i1 == 0x91d0206d);
}
