/* libunwind - a platform-independent unwind library
   Copyright (c) 2002-2003 Hewlett-Packard Development Company, L.P.
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

#include "unwind_i.h"

HIDDEN int
x86_access_reg (struct cursor *c, unw_regnum_t reg, unw_word_t *valp,
		int write)
{
  struct dwarf_loc loc = DWARF_LOC (0, 0);

  switch (reg)
    {

    case UNW_X86_EIP:
      if (write)
	c->dwarf.ip = *valp;		/* also update the EIP cache */
      loc = c->dwarf.loc[EIP];
      break;

    case UNW_X86_CFA:
      if (write)
	return -UNW_EREADONLYREG;
      *valp = c->dwarf.cfa;
      return 0;

    case UNW_X86_EAX: loc = c->dwarf.loc[EAX]; break;
    case UNW_X86_EBX: loc = c->dwarf.loc[EBX]; break;
    case UNW_X86_ECX: loc = c->dwarf.loc[ECX]; break;
    case UNW_X86_EDX: loc = c->dwarf.loc[EDX]; break;
    case UNW_X86_ESI: loc = c->dwarf.loc[ESI]; break;
    case UNW_X86_EDI: loc = c->dwarf.loc[EDI]; break;
    case UNW_X86_EBP: loc = c->dwarf.loc[EBP]; break;
    case UNW_X86_ESP: loc = c->dwarf.loc[ESP]; break;

#if 0
    UNW_X86_EFLAGS
    UNW_X86_TRAPNO

    UNW_X86_FCW
    UNW_X86_FSW
    UNW_X86_FTW
    UNW_X86_FOP
    UNW_X86_FCS
    UNW_X86_FIP
    UNW_X86_FEA
    UNW_X86_FDS

    UNW_X86_MXCSR

    UNW_X86_GS
    UNW_X86_FS
    UNW_X86_ES
    UNW_X86_DS
    UNW_X86_SS
    UNW_X86_CS
    UNW_X86_TSS
    UNW_X86_LDT
#endif

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
x86_access_fpreg (struct cursor *c, unw_regnum_t reg, unw_fpreg_t *valp,
		  int write)
{
#if 0
    /* MMX/stacked-fp registers */
    UNW_X86_ST0,	/* fp return value */
    UNW_X86_ST1,	/* scratch */
    UNW_X86_ST2,	/* scratch */
    UNW_X86_ST3,	/* scratch */
    UNW_X86_ST4,	/* scratch */
    UNW_X86_ST5,	/* scratch */
    UNW_X86_ST6,	/* scratch */
    UNW_X86_ST7,	/* scratch */
    UNW_X86_XMM0_lo,	/* scratch */
    UNW_X86_XMM0_hi,	/* scratch */
    UNW_X86_XMM1_lo,	/* scratch */
    UNW_X86_XMM1_hi,	/* scratch */
    UNW_X86_XMM2_lo,	/* scratch */
    UNW_X86_XMM2_hi,	/* scratch */
    UNW_X86_XMM3_lo,	/* scratch */
    UNW_X86_XMM3_hi,	/* scratch */
    UNW_X86_XMM4_lo,	/* scratch */
    UNW_X86_XMM4_hi,	/* scratch */
    UNW_X86_XMM5_lo,	/* scratch */
    UNW_X86_XMM5_hi,	/* scratch */
    UNW_X86_XMM6_lo,	/* scratch */
    UNW_X86_XMM6_hi,	/* scratch */
    UNW_X86_XMM7_lo,	/* scratch */
    UNW_X86_XMM7_hi,	/* scratch */
#endif
  return -UNW_EINVAL;
}
