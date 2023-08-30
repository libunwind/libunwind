/* libunwind - a platform-independent unwind library
   Copyright (C) 2008 CodeSourcery
   Copyright (C) 2011-2013 Linaro Limited
   Copyright (C) 2012 Tommi Rantala <tt.rantala@gmail.com>
   Copyright 2022 Blackberry Limited.

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
#include "ucontext_i.h"
#include "unwind_i.h"

/* Recognise PLT entries such as:
  40ddf0:       b0000570        adrp    x16, 4ba000 <_GLOBAL_OFFSET_TABLE_+0x2a8>
  40ddf4:       f9433611        ldr     x17, [x16,#1640]
  40ddf8:       9119a210        add     x16, x16, #0x668
  40ddfc:       d61f0220        br      x17

  \note The current implementation only supports little endian modes.
*/
static int
is_plt_entry (struct dwarf_cursor *c)
{
  unw_word_t w0, w1;
  unw_accessors_t *a;

  if (c->as->big_endian)
    {
      return 0;
    }

  /*
    A PLT (Procedure Linkage Table) is used by the dynamic linker to map the
    relative address of a position independent function call onto the real
    address of the function. If we attempt to unwind from any instruction
    inside the PLT, and the PLT is missing DWARF unwind information, then
    conventional unwinding will fail because although the function has been
    "called" we have not yet entered the prologue and set-up the stack frame.

    This code looks to see if the instruction is anywhere within a "recognised"
    PLT entry (note that the IP could be anywhere within the PLT, so we have to
    examine nearby instructions).
  */

  struct instruction_entry
    {
      uint32_t pattern;
      uint32_t mask;
    } instructions[4] =
    {
      // aarch64
      {0x90000010,0x9f00001f}, // adrp
      {0xf9400211,0xffc003ff}, // ldr
      {0x91000210,0xff8003ff}, // add
      {0xd61f0220,0xffffffff}, // br
    };

  a = unw_get_accessors (c->as);
  if ((*a->access_mem) (c->as, c->ip, &w0, 0, c->as_arg) < 0)
    {
      return 0;
    }

  /*
    NB: the following code is endian sensitive!

    The current implementation is for little-endian modes, big-endian modes
    will see the first instruction in the high bits of w0, and the second
    instruction in the low bits of w0. Some tweaks will be needed to read from
    the correct part of the word to support big endian modes.
  */
  if ((w0      & instructions[0].mask) == instructions[0].pattern &&
     ((w0>>32) & instructions[1].mask) == instructions[1].pattern)
  {
    if ((*a->access_mem) (c->as, c->ip+8, &w1, 0, c->as_arg) >= 0 &&
       (w1      & instructions[2].mask) == instructions[2].pattern &&
      ((w1>>32) & instructions[3].mask) == instructions[3].pattern)
      {
        return 1;
      }
    else
      {
        return 0;
      }
  }
  else if ((w0 & instructions[2].mask) == instructions[2].pattern &&
     ((w0>>32) & instructions[3].mask) == instructions[3].pattern)
  {
    w1 = w0;
    if ((*a->access_mem) (c->as, c->ip-8, &w0, 0, c->as_arg) >= 0 &&
       (w0      & instructions[0].mask) == instructions[0].pattern &&
      ((w0>>32) & instructions[1].mask) == instructions[1].pattern)
      {
        return 1;
      }
    else
      {
        return 0;
      }
  }
  else if ((w0 & instructions[1].mask) == instructions[1].pattern &&
     ((w0>>32) & instructions[2].mask) == instructions[2].pattern)
  {
    if ((*a->access_mem) (c->as, c->ip-4, &w0, 0, c->as_arg) < 0 ||
        (*a->access_mem) (c->as, c->ip+4, &w1, 0, c->as_arg) < 0)
    {
      return 0;
    }
  }
  else if ((w0 & instructions[3].mask) == instructions[3].pattern)
  {
    if ((*a->access_mem) (c->as, c->ip-12, &w0, 0, c->as_arg) < 0 ||
        (*a->access_mem) (c->as, c->ip-4, &w1, 0, c->as_arg) < 0)
    {
      return 0;
    }
  }

  if ((w0      & instructions[0].mask) == instructions[0].pattern &&
     ((w0>>32) & instructions[1].mask) == instructions[1].pattern &&
      (w1      & instructions[2].mask) == instructions[2].pattern &&
     ((w1>>32) & instructions[3].mask) == instructions[3].pattern)
    {
      return 1;
    }
  else
    {
      return 0;
    }
}

#if defined __QNX__
/*
 * QNX kernel call have neither CFI nor save frame pointer,
 * 00000000000549e8 <MsgSendsv_r>:
 *  549e8:       cb0203e2        neg     x2, x2
 *  549ec:       52800168        mov     w8, #0xb  // #11
 *  549f0:       d4000a21        svc     #0x51
 *  549f4:       d65f03c0        ret
 *  549f8:       cb0003e0        neg     x0, x0
 *  549fc:       d65f03c0        ret
 *
 * From disassemble, QNX kernel call have mov followed by svc, and may with some
 * neg instructions at beginning.
 * 1. find procedure's ip start,end start
 * 2. search mov,svc from begin, skip any neg instructions
 */
static bool is_neg_instr(uint32_t instr)
{
  /* 64bit register Xd */
  return ((instr & 0xffe003e0) == 0xcb0003e0);
}

/* QNX use w8 to pass kernel call number */
static bool is_mov_w8_instr(uint32_t instr)
{
  /* movz 32bit register Wd */
  return ((instr & 0xffe00008) == 0x52800008);
}

static bool is_svc_instr(uint32_t instr)
{
  return instr == 0xd4000a21;
}

static bool
is_qnx_kercall(struct dwarf_cursor *c)
{
  unw_word_t w0;
  unw_accessors_t *a;
  int ret;
  unw_word_t proc_start_ip;
  unw_word_t proc_end_ip;

  a = unw_get_accessors_int (c->as);
  if (c->as->big_endian || !a->get_proc_ip_range)
    {
      return false;
    }

  ret = (*a->get_proc_ip_range) (c->as, c->ip, &proc_start_ip, &proc_end_ip, c->as_arg);
  if (ret < 0)
    {
      Debug (2, "ip=0x%lx get proc ip range fail, ret = %d\n", c->ip, ret);
      return false;
    }

  unw_word_t ip = proc_start_ip;
  while ((ip < proc_end_ip) && (ip + 8 < proc_end_ip))
    {
      if ((*a->access_mem) (c->as, ip, &w0, 0, c->as_arg) < 0)
        {
          Debug (14, "access_mem ip=0x%lx fail\n", ip);
          return false;
        }

      uint32_t low32 = w0 & 0xffffffff;
      uint32_t high32 = w0 >> 32;

      if (is_mov_w8_instr(low32) && is_svc_instr(high32))
        {
          return true;
        }
      if (is_neg_instr(low32) && is_neg_instr(high32))
        {
          ip += 8;
        }
      else if (is_neg_instr(low32) && is_mov_w8_instr(high32))
        {
          ip += 4;
        }
      else
        {
          return false;
        }
    }

  return false;
}
#endif

/*
 * Save the location of VL (vector length) from the signal frame to the VG (vector
 * granule) register if it exists, otherwise do nothing. If there is an error,
 * the location is also not modified.
 */
#if defined __linux__
static int
get_sve_vl_signal_loc (struct dwarf_cursor* dwarf, unw_word_t sc_addr)
{
  uint32_t size;
  unw_addr_space_t as = dwarf->as;
  unw_accessors_t *a = unw_get_accessors_int (as);
  void *arg = dwarf->as_arg;
  unw_word_t res_addr = sc_addr + LINUX_SC_RESERVED_OFF;

  /*
   * Max possible size of reserved section is 4096. Jump by size of each record
   * until the SVE signal context section is found.
   */
  for(unw_word_t section_off = 0; section_off < 4096; section_off += size)
    {
      uint32_t magic;
      int ret;
      unw_word_t item_addr = res_addr + section_off + LINUX_SC_RESERVED_MAGIC_OFF;
      if ((ret = dwarf_readu32(as, a, &item_addr, &magic, arg)) < 0)
        return ret;
      item_addr = res_addr + section_off + LINUX_SC_RESERVED_SIZE_OFF;
      if ((ret = dwarf_readu32(as, a, &item_addr, &size, arg)) < 0)
        return ret;

      switch(magic)
        {
          case 0:
            /* End marker, size must be 0 */
            return size ? -UNW_EUNSPEC : 1;

          /* Magic number marking the SVE context section */
          case 0x53564501:
            /* Must be big enough so that the 16 bit VL value can be accessed */
            if (size < LINUX_SC_RESERVED_SVE_VL_OFF + 2)
              return -UNW_EUNSPEC;
            dwarf->loc[UNW_AARCH64_VG] = DWARF_MEM_LOC(c->dwarf, res_addr + section_off +
                                                       LINUX_SC_RESERVED_SVE_VL_OFF);
            return 1;

          default:
            /* Don't care about any of the other sections */
            break;
        }
   }
   return 1;
}
#else
static int
get_sve_vl_signal_loc (struct dwarf_cursor* dwarf, unw_word_t sc_addr)
{
   return 1;
}
#endif

static int
aarch64_handle_signal_frame (unw_cursor_t *cursor)
{
  struct cursor *c = (struct cursor *) cursor;
  int i, ret;
  unw_word_t sc_addr, sp, sp_addr = c->dwarf.cfa;
  struct dwarf_loc sp_loc = DWARF_LOC (sp_addr, 0);

  if ((ret = dwarf_get (&c->dwarf, sp_loc, &sp)) < 0)
    return -UNW_EUNSPEC;

  ret = unw_is_signal_frame (cursor);
  Debug(1, "unw_is_signal_frame()=%d\n", ret);
  if (ret > 0)
    {
      c->sigcontext_format = SCF_FORMAT;
      sc_addr = sp_addr + sizeof (siginfo_t) + UC_MCONTEXT_OFF;
    }
  else
    return -UNW_EUNSPEC;

  /* Save the SP and PC to be able to return execution at this point
     later in time (unw_resume).  */
  c->sigcontext_sp = c->dwarf.cfa;
  c->sigcontext_pc = c->dwarf.ip;

  c->sigcontext_addr = sc_addr;
  c->frame_info.frame_type = UNW_AARCH64_FRAME_SIGRETURN;
  c->frame_info.cfa_reg_offset = sc_addr - sp_addr;

  for (i = 0; i < DWARF_NUM_PRESERVED_REGS; ++i)
    c->dwarf.loc[i] = DWARF_NULL_LOC;

  /* Update the dwarf cursor.
     Set the location of the registers to the corresponding addresses of the
     uc_mcontext / sigcontext structure contents.  */
  c->dwarf.loc[UNW_AARCH64_X0]  = DWARF_MEM_LOC (c->dwarf, sc_addr + SC_GPR_OFF);
  c->dwarf.loc[UNW_AARCH64_X1]  = DWARF_MEM_LOC (c->dwarf, sc_addr + SC_GPR_OFF + 8 * 1);
  c->dwarf.loc[UNW_AARCH64_X2]  = DWARF_MEM_LOC (c->dwarf, sc_addr + SC_GPR_OFF + 8 * 2);
  c->dwarf.loc[UNW_AARCH64_X3]  = DWARF_MEM_LOC (c->dwarf, sc_addr + SC_GPR_OFF + 8 * 3);
  c->dwarf.loc[UNW_AARCH64_X4]  = DWARF_MEM_LOC (c->dwarf, sc_addr + SC_GPR_OFF + 8 * 4);
  c->dwarf.loc[UNW_AARCH64_X5]  = DWARF_MEM_LOC (c->dwarf, sc_addr + SC_GPR_OFF + 8 * 5);
  c->dwarf.loc[UNW_AARCH64_X6]  = DWARF_MEM_LOC (c->dwarf, sc_addr + SC_GPR_OFF + 8 * 6);
  c->dwarf.loc[UNW_AARCH64_X7]  = DWARF_MEM_LOC (c->dwarf, sc_addr + SC_GPR_OFF + 8 * 7);
  c->dwarf.loc[UNW_AARCH64_X8]  = DWARF_MEM_LOC (c->dwarf, sc_addr + SC_GPR_OFF + 8 * 8);
  c->dwarf.loc[UNW_AARCH64_X9]  = DWARF_MEM_LOC (c->dwarf, sc_addr + SC_GPR_OFF + 8 * 9);
  c->dwarf.loc[UNW_AARCH64_X10] = DWARF_MEM_LOC (c->dwarf, sc_addr + SC_GPR_OFF + 8 * 10);
  c->dwarf.loc[UNW_AARCH64_X11] = DWARF_MEM_LOC (c->dwarf, sc_addr + SC_GPR_OFF + 8 * 11);
  c->dwarf.loc[UNW_AARCH64_X12] = DWARF_MEM_LOC (c->dwarf, sc_addr + SC_GPR_OFF + 8 * 12);
  c->dwarf.loc[UNW_AARCH64_X13] = DWARF_MEM_LOC (c->dwarf, sc_addr + SC_GPR_OFF + 8 * 13);
  c->dwarf.loc[UNW_AARCH64_X14] = DWARF_MEM_LOC (c->dwarf, sc_addr + SC_GPR_OFF + 8 * 14);
  c->dwarf.loc[UNW_AARCH64_X15] = DWARF_MEM_LOC (c->dwarf, sc_addr + SC_GPR_OFF + 8 * 15);
  c->dwarf.loc[UNW_AARCH64_X16] = DWARF_MEM_LOC (c->dwarf, sc_addr + SC_GPR_OFF + 8 * 16);
  c->dwarf.loc[UNW_AARCH64_X17] = DWARF_MEM_LOC (c->dwarf, sc_addr + SC_GPR_OFF + 8 * 17);
  c->dwarf.loc[UNW_AARCH64_X18] = DWARF_MEM_LOC (c->dwarf, sc_addr + SC_GPR_OFF + 8 * 18);
  c->dwarf.loc[UNW_AARCH64_X19] = DWARF_MEM_LOC (c->dwarf, sc_addr + SC_GPR_OFF + 8 * 19);
  c->dwarf.loc[UNW_AARCH64_X20] = DWARF_MEM_LOC (c->dwarf, sc_addr + SC_GPR_OFF + 8 * 20);
  c->dwarf.loc[UNW_AARCH64_X21] = DWARF_MEM_LOC (c->dwarf, sc_addr + SC_GPR_OFF + 8 * 21);
  c->dwarf.loc[UNW_AARCH64_X22] = DWARF_MEM_LOC (c->dwarf, sc_addr + SC_GPR_OFF + 8 * 22);
  c->dwarf.loc[UNW_AARCH64_X23] = DWARF_MEM_LOC (c->dwarf, sc_addr + SC_GPR_OFF + 8 * 23);
  c->dwarf.loc[UNW_AARCH64_X24] = DWARF_MEM_LOC (c->dwarf, sc_addr + SC_GPR_OFF + 8 * 24);
  c->dwarf.loc[UNW_AARCH64_X25] = DWARF_MEM_LOC (c->dwarf, sc_addr + SC_GPR_OFF + 8 * 25);
  c->dwarf.loc[UNW_AARCH64_X26] = DWARF_MEM_LOC (c->dwarf, sc_addr + SC_GPR_OFF + 8 * 26);
  c->dwarf.loc[UNW_AARCH64_X27] = DWARF_MEM_LOC (c->dwarf, sc_addr + SC_GPR_OFF + 8 * 27);
  c->dwarf.loc[UNW_AARCH64_X28] = DWARF_MEM_LOC (c->dwarf, sc_addr + SC_GPR_OFF + 8 * 28);
  c->dwarf.loc[UNW_AARCH64_X29] = DWARF_MEM_LOC (c->dwarf, sc_addr + SC_X29_OFF);
  c->dwarf.loc[UNW_AARCH64_X30] = DWARF_MEM_LOC (c->dwarf, sc_addr + SC_X30_OFF);
  c->dwarf.loc[UNW_AARCH64_SP]  = DWARF_MEM_LOC (c->dwarf, sc_addr + SC_SP_OFF);
  c->dwarf.loc[UNW_AARCH64_PC]  = DWARF_MEM_LOC (c->dwarf, sc_addr + SC_PC_OFF);
  c->dwarf.loc[UNW_AARCH64_PSTATE]  = DWARF_MEM_LOC (c->dwarf, sc_addr + SC_PSTATE_OFF);
  c->dwarf.loc[UNW_AARCH64_VG]  = DWARF_NULL_LOC;

  /* Set SP/CFA and PC/IP.  */
  dwarf_get (&c->dwarf, c->dwarf.loc[UNW_AARCH64_SP], &c->dwarf.cfa);
  dwarf_get (&c->dwarf, c->dwarf.loc[UNW_AARCH64_PC], &c->dwarf.ip);

  c->dwarf.pi_valid = 0;
  c->dwarf.use_prev_instr = 0;

  return get_sve_vl_signal_loc (&c->dwarf, sc_addr);
}

int
unw_step (unw_cursor_t *cursor)
{
  struct cursor *c = (struct cursor *) cursor;
  int validate = c->validate;
  unw_word_t fp;
  int ret;

  Debug (1, "(cursor=%p, ip=0x%016lx, cfa=0x%016lx))\n",
         c, c->dwarf.ip, c->dwarf.cfa);

  /* Validate all addresses before dereferencing. */
  c->validate = 1;

  /* Check if this is a signal frame. */
  ret = unw_is_signal_frame (cursor);
  if (ret > 0)
    return aarch64_handle_signal_frame (cursor);
  else if (unlikely (ret < 0))
    {
      /* IP points to non-mapped memory. */
      /* This is probably SIGBUS. */
      /* Try to load LR in IP to recover. */
      Debug(1, "Invalid address found in the call stack: 0x%lx\n", c->dwarf.ip);
      dwarf_get (&c->dwarf, c->dwarf.loc[UNW_AARCH64_X30], &c->dwarf.ip);
    }

  /* Try DWARF-based unwinding... */
  c->sigcontext_format = AARCH64_SCF_NONE;
  ret = dwarf_step (&c->dwarf);
  Debug(1, "dwarf_step()=%d\n", ret);

  /* Restore default memory validation state */
  c->validate = validate;

  if (unlikely (ret == -UNW_ESTOPUNWIND))
    return ret;

  if (likely (ret > 0))
    {
      ret = dwarf_get (&c->dwarf, c->dwarf.loc[UNW_AARCH64_X29], &fp);
      if (ret == 0 && fp == 0)
        {
	  /* Procedure Call Standard for the ARM 64-bit Architecture (AArch64)
	   * specifies that the end of the frame record chain is indicated by
	   * the address zero in the address for the previous frame.
	   */
	  c->dwarf.ip = 0;
	  Debug (2, "NULL frame pointer X29 loc, returning 0\n");
	  return 0;
        }
    }

  if (unlikely (ret < 0))
    {
      /* DWARF failed. */

      /*
       * We could get here because of missing/bad unwind information.
       * Validate all addresses before dereferencing.
       */
      if (c->dwarf.as == unw_local_addr_space)
        {
          c->validate = 1;
        }

      if (is_plt_entry (&c->dwarf))
        {
          Debug (2, "found plt entry\n");
          c->frame_info.frame_type = UNW_AARCH64_FRAME_STANDARD;
        }
#if defined __QNX__
      else if (is_qnx_kercall(&c->dwarf))
        {
          Debug (2, "found qnx kernel call, fallback to use link register\n");
          c->frame_info.frame_type = UNW_AARCH64_FRAME_GUESSED;
        }
#endif
      else
        {
	  /* Try use frame pointer (X29). */
          c->frame_info.frame_type = UNW_AARCH64_FRAME_GUESSED;

          ret = dwarf_get (&c->dwarf, c->dwarf.loc[UNW_AARCH64_X29], &fp);
	  if (likely (ret == 0))
	    {
	      if (fp == 0)
	        {
		  /* Procedure Call Standard for the ARM 64-bit Architecture (AArch64)
		   * specifies that the end of the frame record chain is indicated by
		   * the address zero in the address for the previous frame.
		   */
		  c->dwarf.ip = 0;
		  Debug (2, "NULL frame pointer X29 loc, returning 0\n");
		  return 0;
	        }

	      Debug (2, "fallback, x29 = 0x%016lx\n", fp);
	      for (int i = 0; i < DWARF_NUM_PRESERVED_REGS; ++i)
		c->dwarf.loc[i] = DWARF_NULL_LOC;

	      c->dwarf.loc[UNW_AARCH64_SP]  = DWARF_MEM_LOC (c->dwarf, fp);
	      c->dwarf.loc[UNW_AARCH64_X30] = DWARF_MEM_LOC (c->dwarf, fp + 8);

	      /* Set SP/CFA and PC/IP.  */
	      ret = dwarf_get (&c->dwarf, c->dwarf.loc[UNW_AARCH64_SP], &c->dwarf.cfa);
	      if (ret < 0)
	        return ret;
	      ret = dwarf_get (&c->dwarf, c->dwarf.loc[UNW_AARCH64_X30], &c->dwarf.ip);
	      if (ret == 0)
	        {
	          ret = 1;
	          c->dwarf.loc[UNW_AARCH64_X29] = c->dwarf.loc[UNW_AARCH64_SP];
	          c->dwarf.loc[UNW_AARCH64_PC] = c->dwarf.loc[UNW_AARCH64_X30];
		}
	      Debug (2, "fallback, CFA = 0x%016lx, IP = 0x%016lx returning %d\n",
	        c->dwarf.cfa, c->dwarf.ip, ret);
	      return ret;
	    }
        }
      /* Use link register (X30). */
      c->frame_info.cfa_reg_offset = 0;
      c->frame_info.cfa_reg_sp = 0;
      c->frame_info.fp_cfa_offset = -1;
      c->frame_info.lr_cfa_offset = -1;
      c->frame_info.sp_cfa_offset = -1;
      c->dwarf.loc[UNW_AARCH64_PC] = c->dwarf.loc[UNW_AARCH64_X30];
      c->dwarf.loc[UNW_AARCH64_X30] = DWARF_NULL_LOC;
      if (!DWARF_IS_NULL_LOC (c->dwarf.loc[UNW_AARCH64_PC]))
        {
          ret = dwarf_get (&c->dwarf, c->dwarf.loc[UNW_AARCH64_PC], &c->dwarf.ip);
          if (ret < 0)
            {
              Debug (2, "failed to get pc from link register: %d\n", ret);
              return ret;
            }
          Debug (2, "link register (x30) = 0x%016lx\n", c->dwarf.ip);
          ret = 1;
        }
      else
        c->dwarf.ip = 0;
    }

  return (c->dwarf.ip == 0) ? 0 : 1;
}
