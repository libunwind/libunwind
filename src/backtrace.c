/* libunwind - a platform-independent unwind library
   Copyright (C) 2001-2002 Hewlett-Packard Co
	Contributed by David Mosberger-Tang <davidm@hpl.hp.com>

This file is part of libunwind.

libunwind is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2, or (at your option)
any later version.

libunwind is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

As a special exception, if you link this library with other files to
produce an executable, this library does not by itself cause the
resulting executable to be covered by the GNU General Public License.
This exception does not however invalidate any other reasons why the
executable file might be covered by the GNU General Public
License.  */

#define UNW_LOCAL_ONLY
#include <libunwind.h>

/* See glibc manual for a description of this function.  */

int
backtrace (void **buffer, int size)
{
  unw_cursor_t cursor;
  unw_context_t uc;
  unw_word_t ip;
  int n = 0;

  unw_getcontext (&uc);
  if (unw_init_local (&cursor, &uc) < 0)
    return 0;

  while (unw_step (&cursor) > 0)
    {
      if (n >= size)
	return n;

      if (unw_get_reg (&cursor, UNW_REG_IP, &ip) < 0)
	return n;
      buffer[n++] = (void *) ip;
    }
  return n;
}
