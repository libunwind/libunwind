/**
 * @file src/x86_64/Gstep.c
 * @brief unw_step() for x86_64
 */
/*
 * This file is part of libunwind - a platform-independent unwind library.
 *   Copyright (C) 2002-2004 Hewlett-Packard Co
 *       Contributed by David Mosberger-Tang <davidm@hpl.hp.com>
 *       Modified for x86_64 by Max Asbock <masbock@us.ibm.com>
 *   Copyright 2026 Blackberry Limited.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */
#include "libunwind_i.h"
#include "unwind_i.h"

/**
 * @brief Detect if the current instruction pointer is in a Procedure Linkage Table entry.
 * @param[in] c Pointer to the DWARF cursor containing the current instruction pointer
 *
 * Recognizes x86_64 PLT stub pattern:
 *    3bdf0: ff 25 e2 49 13 00 jmpq   *0x1349e2(%rip)
 *    3bdf6: 68 ae 03 00 00    pushq  $0x3ae
 *    3bdfb: e9 00 c5 ff ff    jmpq   38300 <_init+0x18>
 * This pattern is used by the dynamic linker to resolve function addresses at runtime.
 *
 * This is a static internal function to reduce the indirect call overhead when used
 * within `unw_step()` and is publicly called through `unw_is_plt_entry()`.
 *
 * Reads 16 bytes at the current IP and checks for:
 * - Bytes 0-1: 0x25ff (jmpq *offset(%rip))
 * - Bytes 6-7: 0x68   (pushq immediate)
 * - Bytes 8-11: 0xe9  (jmpq offset)
 *
 * @returns 1 if current location matches PLT entry pattern, 0 otherwise
 * @returns 0 immediately if memory access fails.
 */
static int
_is_plt_entry (struct dwarf_cursor *c)
{
  unw_word_t w0, w1;
  unw_accessors_t *a;
  int ret;

  a = unw_get_accessors_int (c->as);
  if ((ret = (*a->access_mem) (c->as, c->ip, &w0, 0, c->as_arg)) < 0
      || (ret = (*a->access_mem) (c->as, c->ip + 8, &w1, 0, c->as_arg)) < 0)
    return 0;

  ret = (((w0 & 0xffff) == 0x25ff)
         && (((w0 >> 48) & 0xff) == 0x68)
         && (((w1 >> 24) & 0xff) == 0xe9));

  Debug (14, "ip=%#016lx => %#016lx %#016lx, ret = %d\n", c->ip, w0, w1, ret);
  return ret;
}

/**
 * @brief Detect cursor points to a PLT entry.
 * @param[in] uc Pointer to the unwind cursor
 *
 * This is the exported public API for indicating the current frame is a PLT
 * entry.
 *
 * @returns 1 if at a PLT entry, 0 otherwise
 */
int
unw_is_plt_entry (unw_cursor_t *uc)
{
  return _is_plt_entry (&((struct cursor *)uc)->dwarf);
}

/**
 * @brief Attempt to fixup invalid return address assuming a naked CALL
 * @param[in]  c        Pointer to the unwind cursor
 *
 * Sometimes a function may not have executed any kind of prolog. For example,
 * a bad function pointer (which generally raises a SIGSEGV) or a function written
 * in assembly not following the SYS V ABI spec.
 *
 * This function assumes the stack pointer (%rsp) is still valid.
 *
 * A special case of a NULL value for the return address is one way to indicate
 * the call frame termination. The retrieved value is otherwise validated by
 * attempting to read from that address: an unreadable address is not going to
 * contain a valid executable instruction.
 *
 * @returns UNW_ESUCCESS on successful fixup or valid chain termination
 * @returns otherwise on failure
 */
static int
_try_rip_fixup(struct cursor *c)
{
  /* Get the stack pointer from the current frame. */
  unw_word_t rsp;
  int ret = dwarf_get (&c->dwarf, c->dwarf.loc[RSP], &rsp);
  if (ret != UNW_ESUCCESS)
    {
      Debug (1, "dwarf_get(current RSP) returned %d\n", ret);
      return ret;
    }

  /* Get the word at the stack pointer, assume it's the previous %rip value. */
  unw_word_t new_ip;
  ret = dwarf_get (&c->dwarf, DWARF_MEM_LOC(c->dwarf, rsp), &new_ip);
  if (ret != UNW_ESUCCESS)
    {
      Debug (1, "dwarf_get(new RIP) returned %d\n", ret);
      return ret;
    }

  /* If the (assumed) %rip is NULL, it's the end of the call chain. */
  if (new_ip == 0)
    {
      Debug (2, "new RIP was NULL, end of call chain assumed\n");
      c->dwarf.loc[RBP] = DWARF_NULL_LOC;
      c->dwarf.loc[RSP] = DWARF_NULL_LOC;
      c->dwarf.loc[RIP] = DWARF_NULL_LOC;
      c->dwarf.ip = 0;
      return UNW_ESUCCESS;
    }

  /*
   * Try to read the %rip address from memory: if it's not a valid memory
   * location, it's obviously not a valid IP.
   */
  unw_word_t not_used;
  if (dwarf_get (&c->dwarf, DWARF_MEM_LOC(c->dwarf, new_ip), &not_used) != UNW_ESUCCESS)
    {
      Debug (2, "new %%rip %#010lx failed validation\n", new_ip);
      return -UNW_EBADFRAME;
    }

  /*
   * Update the cursor frame info.
   */
  Debug (2, "new %%rip %#010lx looks valid\n", new_ip);
  c->frame_info.cfa_reg_offset = 8;
  c->frame_info.cfa_reg_rsp = -1;
  c->frame_info.rbp_cfa_offset = -1;
  c->frame_info.rsp_cfa_offset = -1;
  c->frame_info.frame_type = UNW_X86_64_FRAME_OTHER;

  /*
   * Update the cursor DWARF info.
   *
   * The call should have pushed %rip to the stack and since there was no
   * preamble %rsp hasn't been touched so %rip should be at [%rsp].
   */
  c->dwarf.loc[RIP] = DWARF_VAL_LOC (c->dwarf, new_ip);
  c->dwarf.loc[RSP] = DWARF_VAL_LOC (c->dwarf, rsp + 8);

  dwarf_get (&c->dwarf, c->dwarf.loc[RSP], &c->dwarf.cfa);
  dwarf_get (&c->dwarf, c->dwarf.loc[RIP], &c->dwarf.ip);
  c->dwarf.use_prev_instr = 1;

  return UNW_ESUCCESS;
}

/**
 * @brief Fallback frame unwinding using %rbp-based frame pointer chain.
 * @param[in]  c        Pointer to the unwind cursor
 *
 * When DWARF info and %rip fixup fail, attempt to walk the call stack using %rbp
 * (frame pointer) as a chain. Each frame stores the previous %rbp at [%rbp] and
 * return address at [%rbp+8].
 *
 * Assumes x86_64 System V ABI frame layout:
 * - [%rbp]    = previous %rbp (frame pointer chain)
 * - [%rbp+8]  = return address (%rip)
 * - [%rbp+16] = %rsp value for parent frame
 *
 * Validates %rbp chain sanity:
 * - New %rbp must be above current CFA (stack grows down, so new %rbp must have
 *   an address greater than the current %rbp).
 * - %rbp increase must be reasonable (< 0x4000 bytes).
 *
 * On validation failure, marks %rip/%rbp locations as NULL to stop unwinding.
 * Sets frame type to GUESSED since this is heuristic-based fallback.
 *
 * @returns UNW_ESUCCESS on success
 * @returns otherwise on failure
 */
static int
_try_rbp_frame_walk(struct cursor *c)
{
  /* Get the current frame's %rbp. */
  unw_word_t cur_rbp = 0;
  int ret = dwarf_get (&c->dwarf, c->dwarf.loc[RBP], &cur_rbp);
  if (ret != UNW_ESUCCESS)
    {
      Debug (1, "dwarf_get(current RBP) returned %d\n", ret);
      return ret;
    }

  /* Get the calling frame's %rbp. */
  unw_word_t new_rbp = 0;
  ret = dwarf_get (&c->dwarf, DWARF_MEM_LOC (c, cur_rbp), &new_rbp);
  if (ret != UNW_ESUCCESS)
    {
      Debug (1, "dwarf_get(caller RBP) returned %d\n", ret);
      return ret;
    }

  /* Heuristic to determine incorrect guess. For %rbp to be a
     valid frame it needs to be above current CFA, but don't
     let it go more than a little. */
  if (new_rbp < c->dwarf.cfa || (new_rbp - c->dwarf.cfa) > 0x4000)
    {
      Debug (1, "new %%rbp failed sanity check: %%rbp=%#010lx < CFA %#010lx or > %#010lx\n",
             new_rbp, c->dwarf.cfa, c->dwarf.cfa + 0x4000);
      c->dwarf.loc[RBP] = DWARF_NULL_LOC;
      c->dwarf.loc[RIP] = DWARF_NULL_LOC;
      c->dwarf.loc[RSP] = DWARF_NULL_LOC;
      c->dwarf.ip = 0;
      return -UNW_EBADFRAME;
    }

  /*
   * Update the cursor frame info.
   */
  c->frame_info.frame_type = UNW_X86_64_FRAME_GUESSED;
  c->frame_info.cfa_reg_rsp = 0;
  c->frame_info.cfa_reg_offset = 16;
  c->frame_info.rbp_cfa_offset = -16;

  /*
   * Update the cursor DWARF info.
   */
  c->dwarf.loc[RBP] = DWARF_VAL_LOC (c, new_rbp);
  c->dwarf.loc[RIP] = DWARF_MEM_LOC (c, cur_rbp + 8);
  c->dwarf.loc[RSP] = DWARF_MEM_LOC (c, cur_rbp + 16);

  dwarf_get (&c->dwarf, c->dwarf.loc[RSP], &c->dwarf.cfa);
  dwarf_get (&c->dwarf, c->dwarf.loc[RIP], &c->dwarf.ip);
  c->dwarf.use_prev_instr = 1;

  return UNW_ESUCCESS;
}

/**
 * @brief Fallback unwinding strategies when DWARF debug info is unavailable.
 * @param[in] c       Pointer to the unwind cursor
 * @param[in] cursor  Original unwind cursor for signal frame detection
 *
 * Attempts multiple methods to unwind the stack when DWARF-based unwinding fails:
 * 1. OS-specific stepping (platform-dependent unwind info).
 * 2. Signal trampoline detection and handling.
 * 3. PLT (shared library call stubs) entry detection and setup.
 * 5. %rip fixup for code without callee frames (probably a bad function
 *    pointer).
 * 4. %rbp-based frame pointer chain walking (like back in the 32-bit days).
 *
 * Also does infinite loop detection if %rip and CFA don't change and
 * end-of-call-stack detection if %rip and %rbp are zero.
 *
 * @returns 1         - success, continue unwinding
 * @returns 0         - success, end of unwinding detected
 * @returns otherwise - error
 */
static int
_unw_step_fallback(struct cursor *c, unw_cursor_t  *cursor)
{
  /* We could get here because of missing/bad unwind information.
     Validate all addresses before dereferencing. */

  /* Try OS-specific stepping */
  Debug (2, ".. OS-specific check ..\n");
  int ret = x86_64_os_step (c);
  if (ret != 0)
    {
      Debug (2, "returning %d\n", ret);
      return ret;
    }

  /* Try signal frame handling */
  Debug (2, ".. checking for signal trampoline ..\n");
  if (unw_is_signal_frame (cursor) > 0)
    {
      ret = x86_64_handle_signal_frame(cursor);
      Debug (2, "returning %d\n", ret);
      return ret == 0 ? 1 : ret;
    }

  /* Try PLT entry handling */
  Debug (2, ".. checking for PLT ..\n");
  if (_is_plt_entry (&c->dwarf))
    {
      /*
       * Update the cursor frame info.
       * Like regular frame, CFA = %rsp+8, RA = [CFA-8], no regs saved.
       */
      c->frame_info.cfa_reg_offset = 8;
      c->frame_info.cfa_reg_rsp = -1;
      c->frame_info.frame_type = UNW_X86_64_FRAME_STANDARD;

      /*
       * Update the cursor DWARF info.
       */
      c->dwarf.loc[RIP] = DWARF_MEM_LOC (c, c->dwarf.cfa);
      c->dwarf.loc[RSP] = DWARF_VAL_LOC (c->dwarf, c->dwarf.cfa + 8);

      dwarf_get (&c->dwarf, c->dwarf.loc[RSP], &c->dwarf.cfa);
      dwarf_get (&c->dwarf, c->dwarf.loc[RIP], &c->dwarf.ip);

      Debug (2, "returning %d\n", 1);
      return 1;
    }

  /* Try %rip fixup if address looks invalid */
  unw_word_t not_used;
  int invalid_prev_rip = dwarf_get (&c->dwarf, DWARF_MEM_LOC (c->dwarf, c->dwarf.ip), &not_used);
  if (invalid_prev_rip != 0)
    {
      Debug (2, ".. trying %%rip fixup ..\n");
      if (_try_rip_fixup (c) == UNW_ESUCCESS)
        {
          ret = 1;
        }
    }
  else
    {
      Debug (2, ".. trying %%rbp frame walk ..\n");
      if (_try_rbp_frame_walk (c) == UNW_ESUCCESS)
        {
          ret = 1;
        }
    }

  /* Check for end of call chain */
  Debug (2, ".. checking NULL %%rip and %%rbp indicating end of call stack ..\n");
  if (ret >= 0 && c->dwarf.ip == 0 && DWARF_IS_NULL_LOC (c->dwarf.loc[RBP]))
    {
      for (int i = 0; i < DWARF_NUM_PRESERVED_REGS; ++i)
        c->dwarf.loc[i] = DWARF_NULL_LOC;
      ret = 0;
    }

  Debug (2, "returning %d\n", ret);
  return ret;
}

int
unw_step (unw_cursor_t *cursor)
{
  struct cursor *c = (struct cursor *) cursor;
  int val = 0;
#if CONSERVATIVE_CHECKS
  if (c->dwarf.as == unw_local_addr_space) {
    val = dwarf_get_validate(&c->dwarf);
    dwarf_set_validate (&c->dwarf, 1);
  }
#endif

  Debug (1, "(cursor=%p, ip=0x%016lx, cfa=0x%016lx)\n",
         c, c->dwarf.ip, c->dwarf.cfa);

  /* Try DWARF-based unwinding... */
  c->sigcontext_format = X86_64_SCF_NONE;
  int ret = dwarf_step (&c->dwarf);

#if CONSERVATIVE_CHECKS
  if (c->dwarf.as == unw_local_addr_space) {
    dwarf_set_validate (&c->dwarf, val);
  }
#endif

  if (ret < 0 && ret != -UNW_ENOINFO)
    {
      Debug (2, "failure in dwarf_step(), returning %d\n", ret);
      return ret;
    }

  if (ret == -UNW_ENOINFO)
    {
      /*
       * DWARF stepping failed. Use fallback strategies.
       */
      if (c->dwarf.as == unw_local_addr_space) {
        val = dwarf_get_validate (&c->dwarf);
        dwarf_set_validate (&c->dwarf, 1);
      }

      unw_word_t prev_ip = c->dwarf.ip;
      unw_word_t prev_cfa = c->dwarf.cfa;

      Debug (2, "dwarf_step() failed, trying fallback heuristics\n");
      ret = _unw_step_fallback (c, cursor);

      /*
       * Check for infinite call chain loops or no successful stepping.
       * If this situation is detected, assume the end of the call chain because
       * that's how it works on some OSes.
       */
      if (ret >= 0 && c->dwarf.ip == prev_ip && c->dwarf.cfa == prev_cfa)
        {
          Debug (2, "infinite call chain loop detected\n");
          ret = 0;
        }

      if (c->dwarf.as == unw_local_addr_space) {
        dwarf_set_validate (&c->dwarf, val);
      }
    }

  /*
   * If stepping was successful, increment the frame counter.
   */
  if (ret >= 0)
    {
    	++c->frames;
    }

  Debug (2, "returning %d\n", ret);
  return ret;
}

