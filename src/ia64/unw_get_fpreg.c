/* libunwind - a platform-independent unwind library
   Copyright (C) 2002 Hewlett-Packard Co
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

#include "unwind_i.h"

int
unw_get_fpreg (unw_cursor_t *cursor, int regnum, unw_fpreg_t *valp)
{
  struct ia64_cursor *c = (struct ia64_cursor *) cursor;

  return ia64_access_fpreg (c, regnum, valp, 0);
}
