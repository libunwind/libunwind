/* libunwind - a platform-independent unwind library
   Copyright (C) 2015 Imagination Technologies Limited
   Copyright (C) 2008 CodeSourcery

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
#include <stdio.h>

/* Read a 32-bit MIPS instruction from addr via access_mem.
 *
 * On N64, unw_word_t is 64 bits but MIPS instructions are 32 bits.
 * access_mem does a 64-bit load, so addr must be 8-byte aligned.  Since MIPS
 * instructions are only guaranteed 4-byte aligned, use the same trick as
 * read_s32(): align addr down to 8 bytes, do one aligned load, then extract
 * the correct 32-bit half based on endianness and the original offset.
 *
 * On O32 and N32, unw_word_t is 32 bits, so access_mem reads exactly one
 * instruction and any MIPS-instruction-aligned address is fine.  */
static int
read_mips_instr (unw_addr_space_t as, unw_accessors_t *a,
                 unw_word_t addr, unw_word_t *val, void *arg)
{
#if _MIPS_SIM == _ABI64
  int offset = addr & 4;
  unw_word_t word;
  int ret;

  if ((ret = (*a->access_mem) (as, addr - offset, &word, 0, arg)) < 0)
    return ret;

  if ((offset != 0) == tdep_big_endian (as))
    *val = word & 0xffffffff;
  else
    *val = (word >> 32) & 0xffffffff;

  return 0;
#else
  return (*a->access_mem) (as, addr, val, 0, arg);
#endif
}

int
unw_is_signal_frame (unw_cursor_t *cursor)
{
  struct cursor *c = (struct cursor *) cursor;
  unw_word_t w0, w1, ip;
  unw_addr_space_t as;
  unw_accessors_t *a;
  void *arg;
  int ret;

  as = c->dwarf.as;
  a = unw_get_accessors_int (as);
  arg = c->dwarf.as_arg;

  ip = c->dwarf.ip;

  /* syscall */
  if ((ret = read_mips_instr (as, a, ip + 4, &w1, arg)) < 0)
    return 0;
  if ((w1 & 0xffffffff) != 0x0c)
    return 0;

  /* li v0, 0x1061 (rt) or li v0, 0x1017 */
  if ((ret = read_mips_instr (as, a, ip, &w0, arg)) < 0)
    return 0;

  switch (c->dwarf.as->abi)
    {
    case UNW_MIPS_ABI_O32:
      switch (w0 & 0xffffffff)
        {
        case 0x24021061:
          return 1;
        case 0x24021017:
          return 2;
        default:
          return 0;
        }
    case UNW_MIPS_ABI_N32:
      switch (w0 & 0xffffffff)
        {
        case 0x24021843:
          return 1;
        default:
          return 0;
        }
    case UNW_MIPS_ABI_N64:
      switch (w0 & 0xffffffff)
        {
        case 0x2402145b:
          return 1;
        default:
          return 0;
        }
    default:
      return 0;
    }
}
