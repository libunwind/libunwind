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

  /* cur_rbp must be >= CFA: on x86_64, the frame pointer is always above
     (or equal to) the stack pointer.  If it's below, it's not a real frame
     pointer (e.g. a general-purpose register value or a fake stack address
     inserted by sanitizers), so bail before dereferencing it. */
  if (cur_rbp < c->dwarf.cfa)
    {
      Debug (1, "cur_rbp %#010lx is below CFA %#010lx, not a frame pointer\n",
             cur_rbp, c->dwarf.cfa);
      return -UNW_EBADFRAME;
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
 * @brief Read the byte at @p addr.
 */
static int
_read_byte (struct cursor *c, unw_word_t addr, uint8_t *byte)
{
  unw_word_t w;
  int ret = dwarf_get (&c->dwarf, DWARF_MEM_LOC (c->dwarf, addr & ~(unw_word_t) 7), &w);
  if (ret < 0)
    return ret;
  *byte = (uint8_t) (w >> (8 * (addr & 7)));
  return 0;
}

/**
 * @brief Whether @p ip is a plausible return address, i.e. follows a call.
 *
 * Checks for the encodings compilers use for calls, each ending at @p ip:
 *
 *   e8 rel32                  call rel32                     (5 bytes)
 *   ff /2, mod 11             call *%reg                     (2 bytes)
 *   ff /2, mod 00             call *(%reg)                   (2 bytes)
 *   ff /2, mod 01 + disp8     call *d8(%reg)                 (3 bytes)
 *   ff 15 + disp32            call *d32(%rip)                (6 bytes)
 *   ff /2, mod 10 + disp32    call *d32(%reg)                (6 bytes)
 *
 * The forms with a SIB byte are not recognized.  A REX prefix does not
 * matter, as only the bytes from the opcode on are checked.
 */
static int
_follows_call (struct cursor *c, unw_word_t ip)
{
  uint8_t b[7];    /* b[i] is the byte at ip - 7 + i */
  int i;

  for (i = 0; i < 7; ++i)
    if (_read_byte (c, ip - 7 + i, &b[i]) < 0)
      return 0;

#define MODRM_CALL(m, mod) (((m) & 0xf8) == (((mod) << 6) | 0x10))
  if (b[2] == 0xe8)
    return 1;
  if (b[5] == 0xff && MODRM_CALL (b[6], 3))
    return 1;
  if (b[5] == 0xff && MODRM_CALL (b[6], 0)
      && (b[6] & 7) != 4 && (b[6] & 7) != 5)
    return 1;
  if (b[4] == 0xff && MODRM_CALL (b[5], 1) && (b[5] & 7) != 4)
    return 1;
  if (b[1] == 0xff && b[2] == 0x15)
    return 1;
  if (b[1] == 0xff && MODRM_CALL (b[2], 2) && (b[2] & 7) != 4)
    return 1;
#undef MODRM_CALL
  return 0;
}

/* No save slot is known for a register.  */
#define NO_SLOT ((unw_word_t) -1)

/* The DWARF register number of each register encoded in the low three
   bits of a push or pop opcode, extended by REX.B.  */
static const uint8_t _hw_to_dwarf[16] =
  {
    RAX, RCX, RDX, RBX, RSP, RBP, RSI, RDI,
    R8, R8 + 1, R8 + 2, R8 + 3, R12, R12 + 1, R12 + 2, R15
  };

/**
 * @brief Find how much stack an epilogue at the IP still has to release.
 * @param[in]  c        Pointer to the unwind cursor
 * @param[out] frame    The offset of the return address from %rsp
 * @param[out] slot     For each register, by its encoding, the offset from
 *                      %rsp that a pop still ahead of the IP restores it
 *                      from, or NO_SLOT
 *
 * Decodes the straight-line sequence
 *
 *     add $imm,%rsp       (optional)
 *     pop %reg            (any number)
 *     ret
 *
 * starting at the IP.  Then the return address is at %rsp plus the
 * immediate and 8 bytes for each pop, however the function got there,
 * and each pop restores its register from the slot after the ones
 * before it.
 *
 * @returns 1 if the IP is at such a sequence
 * @returns 0 if the IP is at another instruction
 * @returns -1 if the IP is at an epilogue that cannot be followed, such
 *          as leave, or a pop that is not followed by ret
 */
static int
_scan_epilogue (struct cursor *c, unw_word_t *frame, unw_word_t slot[16])
{
  unw_word_t addr = c->dwarf.ip;
  uint8_t b0, b1, b2, b3;
  int i;

  *frame = 0;
  for (i = 0; i < 16; ++i)
    slot[i] = NO_SLOT;
  for (i = 0; i < 64; ++i)
    {
      if (_read_byte (c, addr, &b0) < 0)
        return i ? -1 : 0;
      if (b0 == 0xc3)                                     /* ret */
        return 1;
      if (b0 == 0x5c)                                     /* pop %rsp */
        return i ? -1 : 0;
      if (b0 >= 0x58 && b0 <= 0x5f)                       /* pop %reg */
        {
          if (slot[b0 - 0x58] == NO_SLOT)
            slot[b0 - 0x58] = *frame;
          *frame += 8;
          addr += 1;
          continue;
        }
      if (b0 == 0xc9)                                     /* leave */
        return -1;
      if (_read_byte (c, addr + 1, &b1) < 0)
        return i ? -1 : 0;
      if (b0 == 0xf3 && b1 == 0xc3)                       /* rep ret */
        return 1;
      if (b0 == 0x41 && b1 >= 0x58 && b1 <= 0x5f)         /* pop %r8-%r15 */
        {
          if (slot[b1 - 0x58 + 8] == NO_SLOT)
            slot[b1 - 0x58 + 8] = *frame;
          *frame += 8;
          addr += 2;
          continue;
        }
      if (i == 0 && b0 == 0x48
          && _read_byte (c, addr + 2, &b2) == 0 && b2 == 0xc4
          && _read_byte (c, addr + 3, &b3) == 0)
        {
          if (b1 == 0x83 && !(b3 & 0x80))                 /* add $imm8,%rsp */
            {
              *frame = b3;
              addr += 4;
              continue;
            }
          if (b1 == 0x81)                                 /* add $imm32,%rsp */
            {
              uint32_t imm = b3;
              int j;
              for (j = 1; j < 4; ++j)
                {
                  uint8_t b;
                  if (_read_byte (c, addr + 3 + j, &b) < 0)
                    return -1;
                  imm |= (uint32_t) b << (8 * j);
                }
              if (imm & 0x80000000)
                return -1;
              *frame = imm;
              addr += 7;
              continue;
            }
        }
      return i ? -1 : 0;
    }
  return -1;
}

/**
 * @brief Step out of a function without unwind info that a signal
 *        interrupted, by following its prologue or its epilogue.
 * @param[in]  c        Pointer to the unwind cursor
 *
 * C libraries built without unwind tables, such as musl, leave nothing to
 * unwind their functions with, and are often built without frame
 * pointers, so the %rbp walk does not work either.  For example:
 *
 *     <kill>:
 *     sub    $0x8,%rsp
 *     movslq %edi,%rdi
 *     movslq %esi,%rsi
 *     mov    $0x3e,%eax
 *     syscall
 *
 * If the IP is at an epilogue, follow it to the ret.  Otherwise, when the
 * start of the function is known, follow its prologue from there:
 * endbr64, pushes and sub $imm,%rsp, up to the first other instruction,
 * assuming that the stack pointer does not change after that before the
 * IP.  The return address is then at %rsp plus the space allocated;
 * accept it only if it follows a call.  The callee-saved registers that
 * the prologue pushed, or that the epilogue has yet to pop, are then
 * restored from their slots; the others are assumed to still hold the
 * caller's values.  If the prologue sets up %rbp as
 * a frame pointer, leave the frame to the %rbp walk, which also copes
 * with alloca() and stack realignment.
 *
 * Only the frame that a signal interrupted can have its IP within the
 * prologue or the epilogue; other frames without unwind info are left
 * to the other heuristics.  So are frames that cannot be followed, such
 * as frames with an epilogue that ends in a tail call.
 *
 * @returns UNW_ESUCCESS on success
 * @returns otherwise on failure
 */
static int
_try_prologue_frame (struct cursor *c)
{
  unw_accessors_t *a = unw_get_accessors_int (c->dwarf.as);
  unw_word_t start, offp, addr, rsp, new_ip;
  unw_word_t frame = 0, slot[16];
  char name[128];
  int ret, i;

  if (c->dwarf.use_prev_instr)
    return -UNW_ENOINFO;

  ret = _scan_epilogue (c, &frame, slot);
  if (ret < 0)
    return -UNW_ENOINFO;
  if (ret > 0)
    {
      Debug (2, "in an epilogue, %lu bytes left to release\n", (long) frame);
      goto found;
    }

  if (a->get_proc_name == NULL
      || (*a->get_proc_name) (c->dwarf.as, c->dwarf.ip, name, sizeof (name),
                              &offp, c->dwarf.as_arg) != 0
      || offp > 0x10000)
    return -UNW_ENOINFO;
  start = c->dwarf.ip - offp;

  for (i = 0; i < 16; ++i)
    slot[i] = NO_SLOT;

  for (addr = start; addr < c->dwarf.ip; )
    {
      uint8_t b0, b1, b2, b3;

      if (_read_byte (c, addr, &b0) < 0)
        return -UNW_EBADFRAME;
      if (b0 >= 0x50 && b0 <= 0x57)                       /* push %reg */
        {
          frame += 8;
          if (slot[b0 - 0x50] == NO_SLOT)
            slot[b0 - 0x50] = frame;
          addr += 1;
          continue;
        }
      if (_read_byte (c, addr + 1, &b1) < 0
          || _read_byte (c, addr + 2, &b2) < 0
          || _read_byte (c, addr + 3, &b3) < 0)
        return -UNW_EBADFRAME;
      if (b0 == 0x41 && b1 >= 0x50 && b1 <= 0x57)         /* push %r8-%r15 */
        {
          frame += 8;
          if (slot[b1 - 0x50 + 8] == NO_SLOT)
            slot[b1 - 0x50 + 8] = frame;
          addr += 2;
        }
      else if (b0 == 0xf3 && b1 == 0x0f && b2 == 0x1e && b3 == 0xfa)
        addr += 4;                                        /* endbr64 */
      else if (b0 == 0x48 && b1 == 0x89 && b2 == 0xe5)    /* mov %rsp,%rbp */
        {
          Debug (2, "%%rbp is the frame pointer\n");
          return -UNW_ENOINFO;
        }
      else if (b0 == 0x48 && b1 == 0x83 && b2 == 0xec)    /* sub $imm8,%rsp */
        {
          if (b3 & 0x80)
            return -UNW_ENOINFO;
          frame += b3;
          addr += 4;
        }
      else if (b0 == 0x48 && b1 == 0x81 && b2 == 0xec)    /* sub $imm32,%rsp */
        {
          uint32_t imm = 0;
          int j;
          for (j = 0; j < 4; ++j)
            {
              uint8_t b;
              if (_read_byte (c, addr + 3 + j, &b) < 0)
                return -UNW_EBADFRAME;
              imm |= (uint32_t) b << (8 * j);
            }
          if (imm & 0x80000000)
            return -UNW_ENOINFO;
          frame += imm;
          addr += 7;
        }
      else
        break;
    }

  /* A push went to the return address's slot minus the stack allocated
     up to and including it.  */
  for (i = 0; i < 16; ++i)
    if (slot[i] != NO_SLOT)
      slot[i] = frame - slot[i];

found:
  ret = dwarf_get (&c->dwarf, c->dwarf.loc[RSP], &rsp);
  if (ret < 0)
    return ret;
  ret = dwarf_get (&c->dwarf, DWARF_MEM_LOC (c->dwarf, rsp + frame), &new_ip);
  if (ret < 0)
    return ret;
  if (!_follows_call (c, new_ip))
    {
      Debug (2, "%#lx at %%rsp+%lu does not follow a call\n",
             (long) new_ip, (long) frame);
      return -UNW_ENOINFO;
    }

  Debug (2, "frame of %lu bytes, return address %#lx\n",
         (long) frame, (long) new_ip);

  c->frame_info.cfa_reg_offset = frame + 8;
  c->frame_info.cfa_reg_rsp = 1;
  c->frame_info.rbp_cfa_offset = -1;
  c->frame_info.rsp_cfa_offset = -1;
  c->frame_info.frame_type = UNW_X86_64_FRAME_OTHER;

  /* Restore the callee-saved registers that were pushed, or have yet to
     be popped, from their slots.  Like the other heuristics, assume that
     the others still hold the caller's values.  */
  for (i = 0; i < 16; ++i)
    {
      int reg = _hw_to_dwarf[i];

      if (slot[i] != NO_SLOT
          && (reg == RBX || reg == RBP || (reg >= R12 && reg <= R15)))
        {
          Debug (3, "register %d saved at %%rsp+%lu\n", reg, (long) slot[i]);
          c->dwarf.loc[reg] = DWARF_MEM_LOC (c->dwarf, rsp + slot[i]);
        }
    }
  c->dwarf.loc[RIP] = DWARF_MEM_LOC (c->dwarf, rsp + frame);
  c->dwarf.loc[RSP] = DWARF_VAL_LOC (c->dwarf, rsp + frame + 8);
  c->dwarf.cfa = rsp + frame + 8;
  c->dwarf.ip = new_ip;
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
      if (ret < 0)
        {
          Debug (2, "returning %d\n", ret);
          return ret;
        }

      /* The handler points the register locations at the saved context;
         the IP is the interrupted PC, not a return address. */
      ret = dwarf_get (&c->dwarf, c->dwarf.loc[RIP], &c->dwarf.ip);
      if (ret < 0)
        {
          Debug (2, "returning %d\n", ret);
          return ret;
        }
      c->dwarf.pi_valid = 0;
      c->dwarf.use_prev_instr = 0;

      Debug (2, "returning 1\n");
      return 1;
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

  /* Try following the prologue or epilogue of a function interrupted by
     a signal */
  Debug (2, ".. checking for a prologue or epilogue ..\n");
  if (_try_prologue_frame (c) == UNW_ESUCCESS)
    {
      Debug (2, "returning 1\n");
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

  /* Check for end of call chain: a zero return address marks the
     outermost frame, as in dwarf_step(), whatever %rbp was found to be.  */
  Debug (2, ".. checking NULL %%rip indicating end of call stack ..\n");
  if (c->dwarf.ip == 0)
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

