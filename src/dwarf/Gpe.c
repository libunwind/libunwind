/* libunwind - a platform-independent unwind library
   Copyright (c) 2003 Hewlett-Packard Development Company, L.P.
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

#include "dwarf_i.h"
#include "tdep.h"

HIDDEN int
dwarf_read_encoded_pointer (unw_addr_space_t as, unw_accessors_t *a,
			    unw_word_t *addr, unsigned char encoding,
			    unw_proc_info_t *pi,
			    unw_word_t *valp, void *arg)
{
  unw_word_t val, initial_addr = *addr;
  uint16_t uval16;
  uint32_t uval32;
  uint64_t uval64;
  int16_t sval16;
  int32_t sval32;
  int64_t sval64;
  int ret;

  /* DW_EH_PE_omit and DW_EH_PE_aligned don't follow the normal
     format/application encoding.  Handle them first.  */
  if (encoding == DW_EH_PE_omit)
    {
      *valp = 0;
      return 0;
    }
  else if (encoding == DW_EH_PE_aligned)
    {
      *addr = (initial_addr + sizeof (unw_word_t) - 1) & -sizeof (unw_word_t);
      return dwarf_readw (as, a, addr, valp, arg);
    }

  switch (encoding & DW_EH_PE_FORMAT_MASK)
    {
    case DW_EH_PE_ptr:
      if ((ret = dwarf_readw (as, a, addr, &val, arg)) < 0)
	return ret;
      break;

    case DW_EH_PE_uleb128:
      if ((ret = dwarf_read_uleb128 (as, a, addr, &val, arg)) < 0)
	return ret;
      break;

    case DW_EH_PE_udata2:
      if ((ret = dwarf_readu16 (as, a, addr, &uval16, arg)) < 0)
	return ret;
      val = uval16;
      break;

    case DW_EH_PE_udata4:
      if ((ret = dwarf_readu32 (as, a, addr, &uval32, arg)) < 0)
	return ret;
      val = uval32;
      break;

    case DW_EH_PE_udata8:
      if ((ret = dwarf_readu64 (as, a, addr, &uval64, arg)) < 0)
	return ret;
      val = uval64;
      break;

    case DW_EH_PE_sleb128:
      if ((ret = dwarf_read_uleb128 (as, a, addr, &val, arg)) < 0)
	return ret;
      break;

    case DW_EH_PE_sdata2:
      if ((ret = dwarf_reads16 (as, a, addr, &sval16, arg)) < 0)
	return ret;
      val = sval16;
      break;

    case DW_EH_PE_sdata4:
      if ((ret = dwarf_reads32 (as, a, addr, &sval32, arg)) < 0)
	return ret;
      val = sval32;
      break;

    case DW_EH_PE_sdata8:
      if ((ret = dwarf_reads64 (as, a, addr, &sval64, arg)) < 0)
	return ret;
      val = sval64;
      break;

    default:
      Debug (1, "unexpected encoding format 0x%x\n",
	     encoding & DW_EH_PE_FORMAT_MASK);
      return -UNW_EINVAL;
    }

  if (val == 0)
    {
      /* 0 is a special value and always absolute.  */
      *valp = 0;
      return 0;
    }

  switch (encoding & DW_EH_PE_APPL_MASK)
    {
    case DW_EH_PE_absptr:
      break;

    case DW_EH_PE_pcrel:
      val += initial_addr;
      break;

    case DW_EH_PE_datarel:
      /* XXX For now, assume that data-relative addresses are relative
         to the global pointer.  */
      val += pi->gp;
      break;

    case DW_EH_PE_funcrel:
      val += pi->start_ip;
      break;

    case DW_EH_PE_textrel:
      /* XXX For now we don't support text-rel values.  If there is a
         platform which needs this, we probably would have to add a
         "segbase" member to unw_proc_info_t.  */
    default:
      Debug (1, "unexpected application type 0x%x\n",
	     encoding & DW_EH_PE_APPL_MASK);
      return -UNW_EINVAL;
    }

  if (encoding & DW_EH_PE_indirect)
    {
      unw_word_t indirect_addr = val;

      if ((ret = dwarf_readw (as, a, &indirect_addr, &val, arg)) < 0)
	return ret;
    }

  *valp = val;
  return 0;
}
