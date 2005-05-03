/* libunwind - a platform-independent unwind library
   Copyright (c) 2002-2004 Hewlett-Packard Development Company, L.P.
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

#include "offsets.h"
#include "unwind_i.h"

static inline dwarf_loc_t
linux_scratch_loc (struct cursor *c, unw_regnum_t reg)
{
  unw_word_t addr = c->sigcontext_addr, fpstate_addr, off;
  int ret, is_fpstate = 0;

  switch (c->sigcontext_format)
    {
    case X86_SCF_NONE:
      return DWARF_REG_LOC (&c->dwarf, reg);

    case X86_SCF_LINUX_SIGFRAME:
      break;

    case X86_SCF_LINUX_RT_SIGFRAME:
      addr += LINUX_UC_MCONTEXT_OFF;
      break;
    }

  switch (reg)
    {
    case UNW_X86_GS: off = LINUX_SC_GS_OFF; break;
    case UNW_X86_FS: off = LINUX_SC_FS_OFF; break;
    case UNW_X86_ES: off = LINUX_SC_ES_OFF; break;
    case UNW_X86_DS: off = LINUX_SC_DS_OFF; break;
    case UNW_X86_EDI: off = LINUX_SC_EDI_OFF; break;
    case UNW_X86_ESI: off = LINUX_SC_ESI_OFF; break;
    case UNW_X86_EBP: off = LINUX_SC_EBP_OFF; break;
    case UNW_X86_ESP: off = LINUX_SC_ESP_OFF; break;
    case UNW_X86_EBX: off = LINUX_SC_EBX_OFF; break;
    case UNW_X86_EDX: off = LINUX_SC_EDX_OFF; break;
    case UNW_X86_ECX: off = LINUX_SC_ECX_OFF; break;
    case UNW_X86_EAX: off = LINUX_SC_EAX_OFF; break;
    case UNW_X86_TRAPNO: off = LINUX_SC_TRAPNO_OFF; break;
    case UNW_X86_EIP: off = LINUX_SC_EIP_OFF; break;
    case UNW_X86_CS: off = LINUX_SC_CS_OFF; break;
    case UNW_X86_EFLAGS: off = LINUX_SC_EFLAGS_OFF; break;
    case UNW_X86_SS: off = LINUX_SC_SS_OFF; break;

      /* The following is probably not correct for all possible cases.
	 Somebody who understands this better should review this for
	 correctness.  */

    case UNW_X86_FCW: is_fpstate = 1; off = LINUX_FPSTATE_CW_OFF; break;
    case UNW_X86_FSW: is_fpstate = 1; off = LINUX_FPSTATE_SW_OFF; break;
    case UNW_X86_FTW: is_fpstate = 1; off = LINUX_FPSTATE_TAG_OFF; break;
    case UNW_X86_FCS: is_fpstate = 1; off = LINUX_FPSTATE_CSSEL_OFF; break;
    case UNW_X86_FIP: is_fpstate = 1; off = LINUX_FPSTATE_IPOFF_OFF; break;
    case UNW_X86_FEA: is_fpstate = 1; off = LINUX_FPSTATE_DATAOFF_OFF; break;
    case UNW_X86_FDS: is_fpstate = 1; off = LINUX_FPSTATE_DATASEL_OFF; break;
    case UNW_X86_MXCSR: is_fpstate = 1; off = LINUX_FPSTATE_MXCSR_OFF; break;

      /* stacked fp registers */
    case UNW_X86_ST0: case UNW_X86_ST1: case UNW_X86_ST2: case UNW_X86_ST3:
    case UNW_X86_ST4: case UNW_X86_ST5: case UNW_X86_ST6: case UNW_X86_ST7:
      is_fpstate = 1;
      off = LINUX_FPSTATE_ST0_OFF + 10*(reg - UNW_X86_ST0);
      break;

     /* SSE fp registers */
    case UNW_X86_XMM0_lo: case UNW_X86_XMM0_hi:
    case UNW_X86_XMM1_lo: case UNW_X86_XMM1_hi:
    case UNW_X86_XMM2_lo: case UNW_X86_XMM2_hi:
    case UNW_X86_XMM3_lo: case UNW_X86_XMM3_hi:
    case UNW_X86_XMM4_lo: case UNW_X86_XMM4_hi:
    case UNW_X86_XMM5_lo: case UNW_X86_XMM5_hi:
    case UNW_X86_XMM6_lo: case UNW_X86_XMM6_hi:
    case UNW_X86_XMM7_lo: case UNW_X86_XMM7_hi:
      is_fpstate = 1;
      off = LINUX_FPSTATE_XMM0_OFF + 8*(reg - UNW_X86_XMM0_lo);
      break;

    case UNW_X86_FOP:
    case UNW_X86_TSS:
    case UNW_X86_LDT:
    default:
      return DWARF_REG_LOC (&c->dwarf, reg);
    }

  if (is_fpstate)
    {
      if ((ret = dwarf_get (&c->dwarf,
			    DWARF_MEM_LOC (&c->dwarf,
					   addr + LINUX_SC_FPSTATE_OFF),
			    &fpstate_addr)) < 0)
	return DWARF_NULL_LOC;

      if (!fpstate_addr)
	return DWARF_NULL_LOC;

      return DWARF_MEM_LOC (c, fpstate_addr + off);
    }
  else
    return DWARF_MEM_LOC (c, addr + off);
}

HIDDEN dwarf_loc_t
x86_scratch_loc (struct cursor *c, unw_regnum_t reg)
{
  if (c->sigcontext_addr)
    return linux_scratch_loc (c, reg);
  else
    return DWARF_REG_LOC (&c->dwarf, reg);
}

HIDDEN int
tdep_access_reg (struct cursor *c, unw_regnum_t reg, unw_word_t *valp,
		 int write)
{
  dwarf_loc_t loc = DWARF_NULL_LOC;
  unsigned int mask;
  int arg_num;

  switch (reg)
    {

    case UNW_X86_EIP:
      if (write)
	c->dwarf.ip = *valp;		/* also update the EIP cache */
      loc = c->dwarf.loc[EIP];
      break;

    case UNW_X86_CFA:
    case UNW_X86_ESP:
      if (write)
	return -UNW_EREADONLYREG;
      *valp = c->dwarf.cfa;
      return 0;

    case UNW_X86_EAX:
    case UNW_X86_EDX:
      arg_num = reg - UNW_X86_EAX;
      mask = (1 << arg_num);
      if (write)
	{
	  c->dwarf.eh_args[arg_num] = *valp;
	  c->dwarf.eh_valid_mask |= mask;
	  return 0;
	}
      else if ((c->dwarf.eh_valid_mask & mask) != 0)
	{
	  *valp = c->dwarf.eh_args[arg_num];
	  return 0;
	}
      else
	loc = c->dwarf.loc[(reg == UNW_X86_EAX) ? EAX : EDX];
      break;

    case UNW_X86_ECX: loc = c->dwarf.loc[ECX]; break;
    case UNW_X86_EBX: loc = c->dwarf.loc[EBX]; break;

    case UNW_X86_EBP: loc = c->dwarf.loc[EBP]; break;
    case UNW_X86_ESI: loc = c->dwarf.loc[ESI]; break;
    case UNW_X86_EDI: loc = c->dwarf.loc[EDI]; break;
    case UNW_X86_EFLAGS: loc = c->dwarf.loc[EFLAGS]; break;
    case UNW_X86_TRAPNO: loc = c->dwarf.loc[TRAPNO]; break;

    case UNW_X86_FCW:
    case UNW_X86_FSW:
    case UNW_X86_FTW:
    case UNW_X86_FOP:
    case UNW_X86_FCS:
    case UNW_X86_FIP:
    case UNW_X86_FEA:
    case UNW_X86_FDS:
    case UNW_X86_MXCSR:
    case UNW_X86_GS:
    case UNW_X86_FS:
    case UNW_X86_ES:
    case UNW_X86_DS:
    case UNW_X86_SS:
    case UNW_X86_CS:
    case UNW_X86_TSS:
    case UNW_X86_LDT:
      loc = x86_scratch_loc (c, reg);
      break;

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
  struct dwarf_loc loc = DWARF_NULL_LOC;

  switch (reg)
    {
    case UNW_X86_ST0:
      loc = c->dwarf.loc[ST0];
      break;

      /* stacked fp registers */
    case UNW_X86_ST1:
    case UNW_X86_ST2:
    case UNW_X86_ST3:
    case UNW_X86_ST4:
    case UNW_X86_ST5:
    case UNW_X86_ST6:
    case UNW_X86_ST7:
      /* SSE fp registers */
    case UNW_X86_XMM0_lo:
    case UNW_X86_XMM0_hi:
    case UNW_X86_XMM1_lo:
    case UNW_X86_XMM1_hi:
    case UNW_X86_XMM2_lo:
    case UNW_X86_XMM2_hi:
    case UNW_X86_XMM3_lo:
    case UNW_X86_XMM3_hi:
    case UNW_X86_XMM4_lo:
    case UNW_X86_XMM4_hi:
    case UNW_X86_XMM5_lo:
    case UNW_X86_XMM5_hi:
    case UNW_X86_XMM6_lo:
    case UNW_X86_XMM6_hi:
    case UNW_X86_XMM7_lo:
    case UNW_X86_XMM7_hi:
      loc = x86_scratch_loc (c, reg);
      break;

    default:
      Debug (1, "bad register number %u\n", reg);
      return -UNW_EBADREG;
    }

  if (write)
    return dwarf_putfp (&c->dwarf, loc, *valp);
  else
    return dwarf_getfp (&c->dwarf, loc, valp);
}
