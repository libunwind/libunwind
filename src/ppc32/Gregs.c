/* libunwind - a platform-independent unwind library
   Copyright (C) 2006-2007 IBM
   Contributed by
     Corey Ashford <cjashfor@us.ibm.com>
     Jose Flavio Aguilar Paulino <jflavio@br.ibm.com> <joseflavio@gmail.com>

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
  struct dwarf_loc loc;

  switch (reg)
    {
    case UNW_PPC32_R0:
    case UNW_PPC32_R2:
    case UNW_PPC32_R3:
    case UNW_PPC32_R4:
    case UNW_PPC32_R5:
    case UNW_PPC32_R6:
    case UNW_PPC32_R7:
    case UNW_PPC32_R8:
    case UNW_PPC32_R9:
    case UNW_PPC32_R10:
    case UNW_PPC32_R11:
    case UNW_PPC32_R12:
    case UNW_PPC32_R13:
    case UNW_PPC32_R14:
    case UNW_PPC32_R15:
    case UNW_PPC32_R16:
    case UNW_PPC32_R17:
    case UNW_PPC32_R18:
    case UNW_PPC32_R19:
    case UNW_PPC32_R20:
    case UNW_PPC32_R21:
    case UNW_PPC32_R22:
    case UNW_PPC32_R23:
    case UNW_PPC32_R24:
    case UNW_PPC32_R25:
    case UNW_PPC32_R26:
    case UNW_PPC32_R27:
    case UNW_PPC32_R28:
    case UNW_PPC32_R29:
    case UNW_PPC32_R30:
    case UNW_PPC32_R31:
    case UNW_PPC32_LR:
    case UNW_PPC32_CTR:
    case UNW_PPC32_CCR:
    case UNW_PPC32_XER:
      loc = c->dwarf.loc[reg];
      break;

    case UNW_TDEP_IP: /* UNW_PPC32_NIP */
      if (write)
        {
          c->dwarf.ip = *valp;  /* update the IP cache */
          if (c->dwarf.pi_valid && (*valp < c->dwarf.pi.start_ip
                                    || *valp >= c->dwarf.pi.end_ip))
            c->dwarf.pi_valid = 0;      /* new IP outside of current proc */
        }
      else
        *valp = c->dwarf.ip;
      return 0;

    case UNW_TDEP_SP: /* UNW_PPC32_R1 */
      if (write)
        return -UNW_EREADONLYREG;
      *valp = c->dwarf.cfa;
      return 0;

    default:
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
  struct dwarf_loc loc;

  if ((unsigned) (reg - UNW_PPC32_F0) < 32)
  {
    loc = c->dwarf.loc[reg];
    if (write)
      return dwarf_putfp (&c->dwarf, loc, *valp);
    else
      return dwarf_getfp (&c->dwarf, loc, valp);
  }

  return -UNW_EBADREG;
}

