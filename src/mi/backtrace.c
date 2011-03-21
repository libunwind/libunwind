/* libunwind - a platform-independent unwind library
   Copyright (C) 2001-2002 Hewlett-Packard Co
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

#ifndef UNW_REMOTE_ONLY

#define UNW_LOCAL_ONLY
#include <libunwind.h>
#include <string.h>

/* See glibc manual for a description of this function.  */

int
unw_backtrace (void **buffer, int size)
{
  unw_cursor_t cursor;
  unw_context_t uc, ucsave;
  int n = size;

  unw_getcontext (&uc);
  memcpy (&ucsave, &uc, sizeof(uc));

  if (unw_init_local (&cursor, &uc) < 0)
    return 0;

  if (unw_tdep_trace (&cursor, buffer, &n) < 0)
    {
      n = 0;
      unw_init_local (&cursor, &ucsave);
      while (n < size)
        {
          unw_word_t ip;
          if (unw_get_reg (&cursor, UNW_REG_IP, &ip) < 0)
	    return n;
          buffer[n++] = (void *) (uintptr_t) ip;
	  if (unw_step (&cursor) <= 0)
	    break;
        }
    }

  return n;
}

#endif /* !UNW_REMOTE_ONLY */
