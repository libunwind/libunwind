/* libunwind - a platform-independent unwind library
   Copyright (C) 2008 CodeSourcery
   Copyright (C) 2012 Tommi Rantala <tt.rantala@gmail.com>

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
    case UNW_SH_PC:
      if (write)
        c->dwarf.ip = *valp;            /* update the IP cache */
    case UNW_SH_R0:
    case UNW_SH_R1:
    case UNW_SH_R2:
    case UNW_SH_R3:
    case UNW_SH_R6:
    case UNW_SH_R7:
    case UNW_SH_R8:
    case UNW_SH_R9:
    case UNW_SH_R10:
    case UNW_SH_R11:
    case UNW_SH_R12:
    case UNW_SH_R13:
    case UNW_SH_R14:
    case UNW_SH_PR:
      loc = c->dwarf.loc[reg];
      break;

    case UNW_SH_R4:
    case UNW_SH_R5:
      /* R4 and R5 are the exception data registers (UNW_TDEP_EH).
         They are scratch registers never tracked by DWARF CFI, so
         _Unwind_SetGR stores values in eh_args[] for establish_machine_state
         to forward to uc_mcontext at resume time.  */
      {
        unsigned int idx = reg - UNW_TDEP_EH;
        unsigned int mask = 1u << idx;
        if (write)
          {
            c->dwarf.eh_args[idx] = *valp;
            c->dwarf.eh_valid_mask |= mask;
            return 0;
          }
        if (c->dwarf.eh_valid_mask & mask)
          {
            *valp = c->dwarf.eh_args[idx];
            return 0;
          }
      }
      loc = c->dwarf.loc[reg];
      break;

    case UNW_SH_R15:
      if (write)
        return -UNW_EREADONLYREG;
      *valp = c->dwarf.cfa;
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
  Debug (1, "bad register number %u\n", reg);
  return -UNW_EBADREG;
}
