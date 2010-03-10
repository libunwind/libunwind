/* libunwind - a platform-independent unwind library
   Copyright (C) 2002-2003 Hewlett-Packard Co
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

PROTECTED int
unw_is_signal_frame (unw_cursor_t *cursor)
{
#if defined __linux__
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
       0xb8 0xad 0x00 0x00 0x00        movl 0x80,%eax
       0xcd 0x80                       int 0x80
       0x90                            nop

     if SA_SIGINFO is specified.
  */
  ip = c->dwarf.ip;
  if ((ret = (*a->access_mem) (as, ip, &w0, 0, arg)) < 0
      || (ret = (*a->access_mem) (as, ip + 4, &w1, 0, arg)) < 0)
    return ret;
  ret = ((w0 == 0x0077b858 && w1 == 0x80cd0000)
	 || (w0 == 0x0000adb8 && w1 == 0x9080cd00));
  Debug (16, "returning %d\n", ret);
  return ret;
#elif defined __FreeBSD__
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
sigcode+4:
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
  if ((ret = (*a->access_mem) (as, ip, &w0, 0, arg)) < 0 ||
      (ret = (*a->access_mem) (as, ip + 4, &w1, 0, arg)) < 0 ||
      (ret = (*a->access_mem) (as, ip + 8, &w2, 0, arg)) < 0 ||
      (ret = (*a->access_mem) (as, ip + 16, &w3, 0, arg)) < 0 ||
      (ret = (*a->access_mem) (as, ip + 20, &w4, 0, arg)) < 0 ||
      (ret = (*a->access_mem) (as, ip + 24, &w5, 0, arg)) < 0)
    return ret;
  ret = (w0 == 0x2024448d && w1 == 0x5440f750 && w2 == 0x75000200 &&
	  w3 == 0x14688e03 && w4 == 0x0001a1b8 && w5 == 0x80cd5000);
  if (ret != 0)
	  return (1);
  Debug (16, "returning %d\n", ret);
  return (0);
#else
#error Port me
#endif
  return -UNW_ENOINFO;
}
