/* libunwind - a platform-independent unwind library
   Copyright (C) 2002 Hewlett-Packard Co
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

static inline int
common_init (struct cursor *c)
{
  int ret;

  c->dwarf.loc[RAX] = DWARF_REG_LOC (&c->dwarf, UNW_X86_64_RAX);
  c->dwarf.loc[RDX] = DWARF_REG_LOC (&c->dwarf, UNW_X86_64_RDX);
  c->dwarf.loc[RCX] = DWARF_REG_LOC (&c->dwarf, UNW_X86_64_RCX);
  c->dwarf.loc[RBX] = DWARF_REG_LOC (&c->dwarf, UNW_X86_64_RBX);
  c->dwarf.loc[RSI] = DWARF_REG_LOC (&c->dwarf, UNW_X86_64_RSI);
  c->dwarf.loc[RDI] = DWARF_REG_LOC (&c->dwarf, UNW_X86_64_RDI);
  c->dwarf.loc[RBP] = DWARF_REG_LOC (&c->dwarf, UNW_X86_64_RBP);
  c->dwarf.loc[RSP] = DWARF_REG_LOC (&c->dwarf, UNW_X86_64_RSP);
  c->dwarf.loc[R8]  = DWARF_REG_LOC (&c->dwarf, UNW_X86_64_R8);
  c->dwarf.loc[R9]  = DWARF_REG_LOC (&c->dwarf, UNW_X86_64_R9);
  c->dwarf.loc[R10] = DWARF_REG_LOC (&c->dwarf, UNW_X86_64_R10);
  c->dwarf.loc[R11] = DWARF_REG_LOC (&c->dwarf, UNW_X86_64_R11);
  c->dwarf.loc[R12] = DWARF_REG_LOC (&c->dwarf, UNW_X86_64_R12);
  c->dwarf.loc[R13] = DWARF_REG_LOC (&c->dwarf, UNW_X86_64_R13);
  c->dwarf.loc[R14] = DWARF_REG_LOC (&c->dwarf, UNW_X86_64_R14);
  c->dwarf.loc[R15] = DWARF_REG_LOC (&c->dwarf, UNW_X86_64_R15);
  c->dwarf.loc[RIP] = DWARF_REG_LOC (&c->dwarf, UNW_X86_64_RIP);

  ret = dwarf_get (&c->dwarf, c->dwarf.loc[RIP], &c->dwarf.ip);
  if (ret < 0)
    return ret;

  ret = dwarf_get (&c->dwarf, DWARF_REG_LOC (&c->dwarf, UNW_X86_64_RSP),
		   &c->dwarf.cfa);
  if (ret < 0)
    return ret;

  c->sigcontext_format = X86_64_SCF_NONE;
  c->sigcontext_addr = 0;

  c->dwarf.args_size = 0;
  c->dwarf.ret_addr_column = RIP;
  c->dwarf.pi_valid = 0;
  c->dwarf.pi_is_dynamic = 0;
  c->dwarf.hint = 0;
  c->dwarf.prev_rs = 0;

  return 0;
}
