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

#include <string.h>

#include "dwarf_i.h"
#include "tdep.h"

/* Note: we don't need to keep track of more than the first four
   characters of the augmentation string, because we (a) ignore any
   augmentation string contents once we find an unrecognized character
   and (b) those characters that we do recognize, can't be
   repeated.  */
static inline int
parse_cie (unw_addr_space_t as, unw_accessors_t *a, unw_word_t *addr,
	   unw_proc_info_t *pi, unw_dyn_dwarf_fde_info_t *dfi,
	   int *lsda_encodingp, void *arg)
{
  uint8_t version, ch, augstr[5], fde_encoding, handler_encoding;
  unw_word_t len, cie_end_addr, aug_size, handler = 0;
  uint32_t u32val;
  uint64_t u64val;
  size_t i;
  int ret;
# define STR2(x)	#x
# define STR(x)		STR2(x)

  /* Pick appropriate default for FDE-encoding.  DWARF spec says it
     has to be the size of the addressing unit of the architecture,
     unless specfied otherwise in the augmentation string.  */
  switch (sizeof (unw_word_t))
    {
    case 4:	fde_encoding = DW_EH_PE_udata4; break;
    case 8:	fde_encoding = DW_EH_PE_udata8; break;
    default:	fde_encoding = DW_EH_PE_omit; break;
    }

  *lsda_encodingp = DW_EH_PE_omit;
  dfi->flags = 0;

  if ((ret = dwarf_readu32 (as, a, addr, &u32val, arg)) < 0)
    return ret;

  if (u32val != 0xffffffff)
    {
      /* the CIE is in the 32-bit DWARF format */
      uint32_t cie_id;

      len = u32val;
      cie_end_addr = *addr + len;
      if ((ret = dwarf_readu32 (as, a, addr, &cie_id, arg)) < 0)
	return ret;
      /* DWARF says CIE id should be 0xffffffff, but in .eh_frame, it's 0 */
      if (cie_id != 0)
	{
	  Debug (1, "Unexpected CIE id %x\n", cie_id);
	  return -UNW_EINVAL;
	}
    }
  else
    {
      /* the CIE is in the 64-bit DWARF format */
      uint64_t cie_id;

      if ((ret = dwarf_readu64 (as, a, addr, &u64val, arg)) < 0)
	return ret;
      len = u64val;
      cie_end_addr = *addr + len;
      if ((ret = dwarf_readu64 (as, a, addr, &cie_id, arg)) < 0)
	return ret;
      /* DWARF says CIE id should be 0xffffffffffffffff, but in
	 .eh_frame, it's 0 */
      if (cie_id != 0)
	{
	  Debug (1, "Unexpected CIE id %llx\n", (long long) cie_id);
	  return -UNW_EINVAL;
	}
    }
  dfi->cie_instr_end = cie_end_addr;

  if ((ret = dwarf_readu8 (as, a, addr, &version, arg)) < 0)
    return ret;

  if (version != 1 && version != DWARF_CIE_VERSION)
    {
      Debug (1, "Got CIE version %u, expected version 1 or "
	     STR (DWARF_CIE_VERSION) "\n", version);
      return -UNW_EBADVERSION;
    }

  /* read and parse the augmentation string: */
  memset (augstr, 0, sizeof (augstr));
  for (i = 0;;)
    {
      if ((ret = dwarf_readu8 (as, a, addr, &ch, arg)) < 0)
	return ret;

      if (!ch)
	break;	/* end of augmentation string */

      if (i < sizeof (augstr) - 1)
	augstr[i++] = ch;
    }

  if ((ret = dwarf_read_uleb128 (as, a, addr, &dfi->code_align, arg)) < 0
      || (ret = dwarf_read_sleb128 (as, a, addr, &dfi->data_align, arg)) < 0)
    return ret;

  /* Read the return-address column either as a u8 or as a uleb128.  */
  if (version == 1)
    {
      if ((ret = dwarf_readu8 (as, a, addr, &ch, arg)) < 0)
	return ret;
      dfi->ret_addr_column = ch;
    }
  else if ((ret = dwarf_read_uleb128 (as, a, addr, &dfi->ret_addr_column, arg))
	   < 0)
    return ret;

  if (augstr[0] == 'z')
    {
      dfi->flags |= UNW_DYN_DFI_FLAG_AUGMENTATION_HAS_SIZE;
      if ((ret = dwarf_read_uleb128 (as, a, addr, &aug_size, arg))
	  < 0)
	return ret;
    }

  for (i = 1; i < sizeof (augstr) && augstr[i]; ++i)
    switch (augstr[i])
      {
      case 'L':
	/* read the LSDA pointer-encoding format.  */
	if ((ret = dwarf_readu8 (as, a, addr, &ch, arg)) < 0)
	  return ret;
	*lsda_encodingp = ch;
	break;

      case 'R':
	/* read the FDE pointer-encoding format.  */
	if ((ret = dwarf_readu8 (as, a, addr, &fde_encoding, arg)) < 0)
	  return ret;
	break;

      case 'P':
	/* read the personality-routine pointer-encoding format.  */
	if ((ret = dwarf_readu8 (as, a, addr, &handler_encoding, arg)) < 0)
	  return ret;
	if ((ret = dwarf_read_encoded_pointer (as, a, addr, handler_encoding,
					       pi, &handler, arg)) < 0)
	  return ret;
	break;

      default:
	if (dfi->flags & UNW_DYN_DFI_FLAG_AUGMENTATION_HAS_SIZE)
	  /* If we have the size of the augmentation body, we can skip
	     over the parts that we don't understand, so we're OK. */
	  return 0;
	else
	  {
	    Debug (1, "Unexpected augmentation string `%s'\n", augstr);
	    return -UNW_EINVAL;
	  }
      }
  dfi->flags |= fde_encoding & UNW_DYN_DFI_FLAG_FDE_PE_MASK;
  pi->handler = handler;
  Debug (15, "CIE parsed OK, augmentation = \"%s\", handler=0x%lx\n",
	 augstr, (long) handler);
  dfi->cie_instr_start = *addr;
  return 0;
}

HIDDEN int
dwarf_parse_fde (unw_addr_space_t as, unw_accessors_t *a, unw_word_t *addr,
		 unw_proc_info_t *pi, unw_dyn_dwarf_fde_info_t *dfi, void *arg)
{
  unw_word_t fde_end_addr, cie_addr, cie_offset_addr, aug_end_addr = 0;
  int ret, fde_encoding, ip_range_encoding, lsda_encoding;
  unw_word_t start_ip, ip_range, aug_size;
  uint64_t u64val;
  uint32_t u32val;

  Debug (12, "FDE @ 0x%lx\n", (long) *addr);

  /* Parse enough of the FDE to get the procedure info (LSDA and handler).  */

  if ((ret = dwarf_readu32 (as, a, addr, &u32val, arg)) < 0)
    return ret;

  if (u32val != 0xffffffff)
    {
      uint32_t cie_offset;

      /* the FDE is in the 32-bit DWARF format */

      fde_end_addr = *addr + u32val;
      cie_offset_addr = *addr;

      if ((ret = dwarf_reads32 (as, a, addr, &cie_offset, arg)) < 0)
	return ret;

      /* DWARF says that the CIE_pointer in the FDE is a
	 .debug_frame-relative offset, but the GCC-generated .eh_frame
	 sections instead store a "pcrelative" offset, which is just
	 as fine as it's self-contained.  */
      cie_addr = cie_offset_addr - cie_offset;
    }
  else
    {
      uint64_t cie_offset;

      /* the FDE is in the 64-bit DWARF format */

      if ((ret = dwarf_readu64 (as, a, addr, &u64val, arg)) < 0)
	return ret;

      fde_end_addr = *addr + u64val;
      cie_offset_addr = *addr;

      if ((ret = dwarf_reads64 (as, a, addr, &cie_offset, arg)) < 0)
	return ret;

      /* DWARF says that the CIE_pointer in the FDE is a
	 .debug_frame-relative offset, but the GCC-generated .eh_frame
	 sections instead store a "pcrelative" offset, which is just
	 as fine as it's self-contained.  */
      cie_addr = (unw_word_t) ((uint64_t) cie_offset_addr - cie_offset);
    }
  pi->extra.dwarf_info.fde_instr_end = fde_end_addr;

  if ((ret = parse_cie (as, a, &cie_addr, pi, &pi->extra.dwarf_info,
			&lsda_encoding, arg)) < 0)
    return ret;

  fde_encoding = pi->extra.dwarf_info.flags & UNW_DYN_DFI_FLAG_FDE_PE_MASK;
  /* IP-range has same encoding as FDE pointers, except that it's
     always an absolute value: */
  ip_range_encoding = fde_encoding & DW_EH_PE_FORMAT_MASK;

  if ((ret = dwarf_read_encoded_pointer (as, a, addr, fde_encoding,
					 pi, &start_ip, arg)) < 0
      || (ret = dwarf_read_encoded_pointer (as, a, addr, ip_range_encoding,
					    pi, &ip_range, arg)) < 0)
    return ret;
  pi->start_ip = start_ip;
  pi->end_ip = start_ip + ip_range;

  if (pi->extra.dwarf_info.flags & UNW_DYN_DFI_FLAG_AUGMENTATION_HAS_SIZE)
    {
      if ((ret = dwarf_read_uleb128 (as, a, addr, &aug_size, arg))
	  < 0)
	return ret;
      aug_end_addr =  *addr + aug_size;
    }

  if ((ret = dwarf_read_encoded_pointer (as, a, addr, lsda_encoding,
					 pi, &pi->lsda, arg)) < 0)
    return ret;

  if (pi->extra.dwarf_info.flags & UNW_DYN_DFI_FLAG_AUGMENTATION_HAS_SIZE)
    pi->extra.dwarf_info.fde_instr_start = aug_end_addr;
  else
    pi->extra.dwarf_info.fde_instr_start = *addr;

  /* Always set the unwind info, whether or not need_unwind_info is
     set.  We had to do all the work anyhow, so there is no point in
     not doing so.  */
  pi->format = UNW_INFO_FORMAT_DWARF_FDE;
  pi->unwind_info_size = sizeof (pi->extra.dwarf_info) / sizeof (unw_word_t);
  pi->unwind_info = &pi->extra.dwarf_info;

  Debug (15, "FDE covers IP 0x%lx-0x%lx, LSDA=0x%lx\n",
	 (long) pi->start_ip, (long) pi->end_ip, (long) pi->lsda);
  return 0;
}
