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

#include <stdlib.h>

#include "internal.h"
#include "tdep.h"

HIDDEN sigset_t unwi_full_sigmask;

static const char rcsid[] UNUSED =
  "$Id: " PACKAGE_STRING " --- report bugs to " PACKAGE_BUGREPORT " $";

HIDDEN void
mi_init (void)
{
  extern void unw_cursor_t_is_too_small (void);
#if UNW_DEBUG
  const char *str = getenv ("UNW_DEBUG_LEVEL");

  if (str)
    tdep_debug_level = atoi (str);

  if (tdep_debug_level > 0)
    {
      setbuf (stdout, NULL);
      setbuf (stderr, NULL);
    }
#endif

  if (sizeof (struct cursor) > sizeof (unw_cursor_t))
    unw_cursor_t_is_too_small ();
}
