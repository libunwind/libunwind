/* libunwind - a platform-independent unwind library
   Copyright (C) 2008 CodeSourcery
   Copyright 2011 Linaro Limited

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
#include "offsets.h"
#include "ex_tables.h"

static inline int
arm_exidx_step (struct cursor *c)
{
  struct arm_exidx_table *table = arm_exidx_table_find (c->frame.pc);
  if (NULL == table)
    return -UNW_ENOINFO;

  struct arm_exidx_entry *entry = arm_exidx_table_lookup (table, c->frame.pc);
  if (NULL == entry)
    return -UNW_ENOINFO;

  struct arm_exidx_vrs s;
  arm_exidx_frame_to_vrs (&c->frame, &s);

  uint8_t buf[32];
  int ret = arm_exidx_extract (entry, buf);
  if (ret < 0)
    return ret;

  ret = arm_exidx_decode (buf, ret, &arm_exidx_vrs_callback, &s);
  if (ret < 0)
    return -ret;

  return arm_exidx_vrs_to_frame (&s, &c->frame);
}

PROTECTED int
unw_step (unw_cursor_t *cursor)
{
  struct cursor *c = (struct cursor *) cursor;
  int ret = -UNW_EUNSPEC;

  Debug (1, "(cursor=%p)\n", c);

  if (UNW_TRY_METHOD (UNW_ARM_METHOD_EXIDX))
    {
      ret = arm_exidx_step (c);
      if (ret >= 0)
        return ret;
      if (ret < 0 && ret != -UNW_ENOINFO)
	return ret;
    }

  /* Next, try DWARF-based unwinding. */
  if (UNW_TRY_METHOD(UNW_ARM_METHOD_DWARF))
    {
      ret = dwarf_step (&c->dwarf);
      Debug(1, "dwarf_step()=%d\n", ret);

      if (unlikely (ret == -UNW_ESTOPUNWIND))
        return ret;

    if (ret < 0 && ret != -UNW_ENOINFO)
      {
        Debug (2, "returning %d\n", ret);
        return ret;
      }
    }

  if (unlikely (ret < 0))
    {
      if (UNW_TRY_METHOD(UNW_ARM_METHOD_FRAME))
        {
          ret = UNW_ESUCCESS;
          /* DWARF unwinding failed, try to follow APCS/optimized APCS frame chain */
          unw_word_t instr, i;
          Debug (13, "dwarf_step() failed (ret=%d), trying frame-chain\n", ret);
          dwarf_loc_t ip_loc, fp_loc;
          unw_word_t frame;
          /* Mark all registers unsaved, since we don't know where
             they are saved (if at all), except for the EBP and
             EIP.  */
          if (dwarf_get(&c->dwarf, c->dwarf.loc[UNW_ARM_R11], &frame) < 0)
            {
              return 0;
            }
          for (i = 0; i < DWARF_NUM_PRESERVED_REGS; ++i) {
            c->dwarf.loc[i] = DWARF_NULL_LOC;
          }
          if (frame)
            {
              if (dwarf_get(&c->dwarf, DWARF_LOC(frame, 0), &instr) < 0)
                {
                  return 0;
                }
              instr -= 8;
              if (dwarf_get(&c->dwarf, DWARF_LOC(instr, 0), &instr) < 0)
                {
                  return 0;
                }
              if ((instr & 0xFFFFD800) == 0xE92DD800)
                {
                  /* Standard APCS frame. */
                  ip_loc = DWARF_LOC(frame - 4, 0);
                  fp_loc = DWARF_LOC(frame - 12, 0);
                }
              else
                {
                  /* Codesourcery optimized normal frame. */
                  ip_loc = DWARF_LOC(frame, 0);
                  fp_loc = DWARF_LOC(frame - 4, 0);
                }
              if (dwarf_get(&c->dwarf, ip_loc, &c->dwarf.ip) < 0)
                {
                  return 0;
                }
              c->dwarf.loc[UNW_ARM_R12] = ip_loc;
              c->dwarf.loc[UNW_ARM_R11] = fp_loc;
              Debug(15, "ip=%lx\n", c->dwarf.ip);
            }
          else
            {
              ret = -UNW_ENOINFO;
            }
        }
    }
  return ret == -UNW_ENOINFO ? 0 : 1;
}
