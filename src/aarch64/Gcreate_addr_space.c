/* libunwind - a platform-independent unwind library
   Copyright (C) 2012 Tommi Rantala <tt.rantala@gmail.com>
   Copyright (C) 2013 Linaro Limited

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

#include <string.h>
#include <stdlib.h>

#include "unwind_i.h"

unw_addr_space_t
unw_create_addr_space (unw_accessors_t *a, int byte_order)
{
#ifdef UNW_LOCAL_ONLY
  // to suppress warning (unused)
  (void)a;
  (void)byte_order;
  return NULL;
#else
  unw_addr_space_t as;

  /* AArch64 supports little-endian and big-endian. */
  if (byte_order != 0 && byte_order_is_valid(byte_order) == 0)
    return NULL;

  as = malloc (sizeof (*as));
  if (!as)
    return NULL;

  memset (as, 0, sizeof (*as));

  as->acc = *a;

  /* Default to little-endian for AArch64. */
  if (byte_order == 0 || byte_order == UNW_LITTLE_ENDIAN)
    as->big_endian = 0;
  else
    as->big_endian = 1;

  return as;
#endif
}
