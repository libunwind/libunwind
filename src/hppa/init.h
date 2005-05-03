/* libunwind - a platform-independent unwind library
   Copyright (C) 2003 Hewlett-Packard Co
	Contributed by ...

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

static inline int
common_init (struct cursor *c)
{
  int ret, i;

  for (i = 0; i < 32; ++i)
    {
      c->dwarf.loc[UNW_HPPA_GR + i]
	= DWARF_REG_LOC (&c->dwarf, UNW_HPPA_GR + i);
      c->dwarf.loc[UNW_HPPA_FR + i]
	= DWARF_REG_LOC (&c->dwarf, UNW_HPPA_FR + i);
    }

  c->dwarf.loc[UNW_HPPA_IP] = c->dwarf.loc[UNW_HPPA_RP];

  ret = dwarf_get (&c->dwarf, c->dwarf.loc[UNW_HPPA_IP], &c->dwarf.ip);
  if (ret < 0)
    return ret;

  ret = dwarf_get (&c->dwarf, DWARF_REG_LOC (&c->dwarf, UNW_HPPA_SP),
		   &c->dwarf.cfa);
  if (ret < 0)
    return ret;

  c->sigcontext_format = HPPA_SCF_NONE;
  c->sigcontext_addr = 0;

  c->dwarf.args_size = 0;
  c->dwarf.ret_addr_column = 0;
  c->dwarf.pi_valid = 0;
  c->dwarf.pi_is_dynamic = 0;
  return 0;
}
