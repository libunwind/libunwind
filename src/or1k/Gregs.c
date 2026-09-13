/* libunwind - a platform-independent unwind library
   Copyright (C) 2008 CodeSourcery
   Copyright (C) 2012 Tommi Rantala <tt.rantala@gmail.com>
   Copyright (C) 2026 Ali Ahmet Memiş <aliamemis@disroot.org>

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

  if ((int) reg < UNW_OR1K_R0 || reg > UNW_OR1K_PC)
    {
      Debug (1, "bad register number %u\n", reg);
      return -UNW_EBADREG;
    }

  switch (reg)
    {
    case UNW_OR1K_R0:
      /* r0 reads as zero and cannot be written.  */
      if (write)
        return -UNW_EREADONLYREG;
      *valp = 0;
      return 0;

    case UNW_OR1K_R1:
      /* The stack pointer is the CFA managed by the DWARF unwinder.  */
      if (write)
        return -UNW_EREADONLYREG;
      *valp = c->dwarf.cfa;
      return 0;

    case UNW_OR1K_R25:
    case UNW_OR1K_R27:
      /* The exception-data registers are call-clobbered and are not tracked
         by DWARF CFI.  _Unwind_SetGR stores them in eh_args[] instead.  */
      {
        unsigned int idx = (reg - UNW_TDEP_EH) / 2;
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
      break;

    case UNW_OR1K_PC:
      /* Ordinary frames return through r9, so DWARF CFI never assigns the
         PC column: its location still refers to the context the cursor was
         initialised from.  The cursor's IP is the PC of the current frame.  */
      if (write)
        c->dwarf.ip = *valp;
      else
        *valp = c->dwarf.ip;
      return 0;

    default:
      break;
    }

  loc = c->dwarf.loc[reg];

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
