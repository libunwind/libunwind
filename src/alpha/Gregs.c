/* libunwind - a platform-independent unwind library
   Copyright (c) 2002-2004 Hewlett-Packard Development Company, L.P.
        Contributed by David Mosberger-Tang <davidm@hpl.hp.com>

   Modified for Alpha by Matt Turner <mattst88@gmail.com>

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

HIDDEN int
tdep_access_reg (struct cursor *c, unw_regnum_t reg, unw_word_t *valp,
                 int write)
{
  dwarf_loc_t loc = DWARF_NULL_LOC;

  switch (reg)
    {
    case UNW_ALPHA_CFA:
      if (write)
        return -UNW_EREADONLYREG;
      *valp = c->dwarf.cfa;
      return 0;

    case UNW_ALPHA_R0:
    case UNW_ALPHA_R1:
    case UNW_ALPHA_R2:
    case UNW_ALPHA_R3:
    case UNW_ALPHA_R4:
    case UNW_ALPHA_R5:
    case UNW_ALPHA_R6:
    case UNW_ALPHA_R7:
    case UNW_ALPHA_R8:
    case UNW_ALPHA_R9:
    case UNW_ALPHA_R10:
    case UNW_ALPHA_R11:
    case UNW_ALPHA_R12:
    case UNW_ALPHA_R13:
    case UNW_ALPHA_R14:
    case UNW_ALPHA_R15:
    case UNW_ALPHA_R16:
    case UNW_ALPHA_R17:
    case UNW_ALPHA_R18:
    case UNW_ALPHA_R19:
    case UNW_ALPHA_R20:
    case UNW_ALPHA_R21:
    case UNW_ALPHA_R22:
    case UNW_ALPHA_R23:
    case UNW_ALPHA_R24:
    case UNW_ALPHA_R25:
    case UNW_ALPHA_R26:
    case UNW_ALPHA_R27:
    case UNW_ALPHA_R28:
    case UNW_ALPHA_R29:
    case UNW_ALPHA_PC:
      loc = c->dwarf.loc[reg];
      break;

    case UNW_ALPHA_R30:
      if (write)
        return -UNW_EREADONLYREG;
      loc = c->dwarf.loc[reg];
      break;

    case UNW_ALPHA_R31:
      if (write)
        return -UNW_EREADONLYREG;
      *valp = 0;
      return 0;

    default:
      Debug (1, "bad register number %u\n", reg);
      return -UNW_EBADREG;
    }

  if (write)
    return dwarf_put (&c->dwarf, loc, *valp);
  else
    return dwarf_get (&c->dwarf, loc, valp);
}

HIDDEN int
tdep_access_fpreg (struct cursor *c, unw_regnum_t reg, unw_fpreg_t *valp,
                   int write)
{
  dwarf_loc_t loc = DWARF_NULL_LOC;

  switch (reg)
    {
    case UNW_ALPHA_F0:
    case UNW_ALPHA_F1:
    case UNW_ALPHA_F2:
    case UNW_ALPHA_F3:
    case UNW_ALPHA_F4:
    case UNW_ALPHA_F5:
    case UNW_ALPHA_F6:
    case UNW_ALPHA_F7:
    case UNW_ALPHA_F8:
    case UNW_ALPHA_F9:
    case UNW_ALPHA_F10:
    case UNW_ALPHA_F11:
    case UNW_ALPHA_F12:
    case UNW_ALPHA_F13:
    case UNW_ALPHA_F14:
    case UNW_ALPHA_F15:
    case UNW_ALPHA_F16:
    case UNW_ALPHA_F17:
    case UNW_ALPHA_F18:
    case UNW_ALPHA_F19:
    case UNW_ALPHA_F20:
    case UNW_ALPHA_F21:
    case UNW_ALPHA_F22:
    case UNW_ALPHA_F23:
    case UNW_ALPHA_F24:
    case UNW_ALPHA_F25:
    case UNW_ALPHA_F26:
    case UNW_ALPHA_F27:
    case UNW_ALPHA_F28:
    case UNW_ALPHA_F29:
    case UNW_ALPHA_F30:
      loc = c->dwarf.loc[reg];
      break;

    case UNW_ALPHA_F31:
      if (write)
        return -UNW_EREADONLYREG;
      *valp = 0;
      return 0;

    default:
      Debug (1, "bad register number %u\n", reg);
      return -UNW_EBADREG;
    }

  if (write)
    return dwarf_putfp (&c->dwarf, loc, *valp);
  else
    return dwarf_getfp (&c->dwarf, loc, valp);
}
