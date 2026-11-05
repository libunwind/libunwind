/* libunwind - a platform-independent unwind library
   Copyright (C) 2008 CodeSourcery
   Copyright (C) 2021 Zhaofeng Li

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

static int
riscv_handle_signal_frame (unw_cursor_t *cursor)
{
  int ret, i;
  struct cursor *c = (struct cursor *) cursor;
  unw_word_t sp, sp_addr = c->dwarf.cfa;
  struct dwarf_loc sp_loc = DWARF_LOC (sp_addr, 0);

  if ((ret = dwarf_get (&c->dwarf, sp_loc, &sp)) < 0)
    return -UNW_EUNSPEC;

  if (!unw_is_signal_frame (cursor))
    return -UNW_EUNSPEC;

#ifdef __linux__
  /* rt_sigframe contains the siginfo structure, the ucontext, and then
     the trampoline. We store the mcontext inside ucontext as sigcontext_addr.
  */
  c->sigcontext_format = RISCV_SCF_LINUX_RT_SIGFRAME;
  c->sigcontext_addr = sp_addr + sizeof (siginfo_t) + UC_MCONTEXT_REGS_OFF;
  c->sigcontext_sp = sp_addr;
  c->sigcontext_pc = c->dwarf.ip;
#else
  /* Not making any assumption at all - You need to implement this */
  return -UNW_EUNSPEC;
#endif

  for (i = 0; i < DWARF_NUM_PRESERVED_REGS; ++i)
    c->dwarf.loc[i] = DWARF_NULL_LOC;

  /* Update the dwarf cursor.
     Set the location of the registers to the corresponding addresses of the
     uc_mcontext / sigcontext structure contents.  */

#define  SC_REG_OFFSET(X)   (8 * X)

  /* The PC is stored in place of X0 in sigcontext */
  c->dwarf.loc[UNW_TDEP_IP] = DWARF_LOC (c->sigcontext_addr + SC_REG_OFFSET(UNW_RISCV_X0), 0);

  for (i = UNW_RISCV_X1; i <= UNW_RISCV_F31; i++)
    {
      c->dwarf.loc[i] = DWARF_LOC (c->sigcontext_addr + SC_REG_OFFSET(i), 0);
    }

  /* Set SP/CFA and PC/IP.  */
  if ((ret = dwarf_get (&c->dwarf, c->dwarf.loc[UNW_TDEP_SP], &c->dwarf.cfa)) < 0)
    return ret;
  if ((ret = dwarf_get (&c->dwarf, c->dwarf.loc[UNW_TDEP_IP], &c->dwarf.ip)) < 0)
    return ret;

  c->dwarf.pi_valid = 0;
  c->dwarf.use_prev_instr = 0;

  return 1;
}

/* Read the 16-bit instruction parcel at ADDR.  */
static int
read_insn_parcel (struct cursor *c, unw_word_t addr, uint16_t *parcel)
{
  unw_accessors_t *a = unw_get_accessors_int (c->dwarf.as);
  unw_word_t base = addr & ~(unw_word_t) (sizeof (unw_word_t) - 1);
  unw_word_t w;
  int ret;

  if ((ret = (*a->access_mem) (c->dwarf.as, base, &w, 0, c->dwarf.as_arg)) < 0)
    return ret;
  *parcel = (uint16_t) (w >> (8 * (addr - base)));
  return 0;
}

static int32_t
sign_extend (uint32_t val, int bits)
{
  uint32_t m = 1u << (bits - 1);
  return (int32_t) ((val ^ m) - m);
}

#if __riscv_xlen == 64
# define INSN_S_SP      0x00013023      /* sd rs2, imm(sp) */
# define INSN_L_SP      0x00013003      /* ld rd, imm(sp) */
# define CINSN_S_SP     0xe002          /* c.sdsp rs2, imm */
# define CINSN_L_SP     0x6002          /* c.ldsp rd, imm */
#else
# define INSN_S_SP      0x00012023      /* sw rs2, imm(sp) */
# define INSN_L_SP      0x00012003      /* lw rd, imm(sp) */
# define CINSN_S_SP     0xc002          /* c.swsp rs2, imm */
# define CINSN_L_SP     0x4002          /* c.lwsp rd, imm */
#endif

/* RA and the callee-saved GPRs: s0/fp, s1 and s2-s11.  */
#define TRACKED_REGS    ((1u << 1) | (1u << 8) | (1u << 9) | (0x3ffu << 18))

/* For a function without unwind info, follow the straight-line code from
   its start to the IP, tracking how much stack it allocated and where it
   saved RA and the callee-saved registers, so the caller's SP and
   registers can be found.  musl, for instance, is built without unwind
   tables:

      <kill>:
      addi  sp,sp,-16
      sd    ra,8(sp)
      li    a7,129
      ecall
      ...

   Calls are followed (they return to the next instruction).  Any other
   control flow, or another kind of write to SP, makes the result unknown.
   Returns 1 and sets *FRAME_SIZE (the caller's SP is SP + *FRAME_SIZE),
   *SAVED (a mask of the registers saved on the stack) and CFA_OFFSET (the
   register Xn is at CFA + CFA_OFFSET[n]) when the code could be followed,
   and 0 otherwise.  */
static int
scan_prologue (unw_cursor_t *cursor, int32_t *frame_size, uint32_t *saved,
               int32_t cfa_offset[32])
{
  struct cursor *c = (struct cursor *) cursor;
  unw_accessors_t *a = unw_get_accessors_int (c->dwarf.as);
  unw_word_t start_ip, ip, offp;
  /* For a return address, look up the call rather than what follows it,
     which may be another function after a call to a noreturn one.  */
  unw_word_t lookup_ip = c->dwarf.ip - c->dwarf.use_prev_instr;
  int32_t size = 0, offset[32];
  uint32_t mask = 0;
  char name[128];

  if (a->get_proc_name == NULL
      || (*a->get_proc_name) (c->dwarf.as, lookup_ip, name, sizeof (name),
                              &offp, c->dwarf.as_arg) != 0
      || offp > 4096)
    return 0;

  start_ip = lookup_ip - offp;

  for (ip = start_ip; ip < c->dwarf.ip; )
    {
      uint16_t lo, hi;
      uint32_t insn, r;
      int32_t imm;

      if (read_insn_parcel (c, ip, &lo) < 0)
        return 0;

      if ((lo & 3) != 3)
        {
          /* 16-bit compressed instruction */
          uint32_t h = lo;
          ip += 2;

          if ((h & 0xef83) == 0x6101)           /* c.addi16sp sp, imm */
            size -= sign_extend ((((h >> 12) & 1) << 9) | (((h >> 6) & 1) << 4)
                                 | (((h >> 5) & 1) << 6) | (((h >> 3) & 3) << 7)
                                 | (((h >> 2) & 1) << 5), 10);
          else if ((h & 0xef83) == 0x0101)      /* c.addi sp, imm */
            size -= sign_extend ((((h >> 12) & 1) << 5) | ((h >> 2) & 0x1f), 6);
          else if ((h & 0xe003) == CINSN_S_SP)
            {
              r = (h >> 2) & 0x1f;
#if __riscv_xlen == 64
              imm = (((h >> 10) & 7) << 3) | (((h >> 7) & 7) << 6);
#else
              imm = (((h >> 9) & 0xf) << 2) | (((h >> 7) & 3) << 6);
#endif
              if (TRACKED_REGS & (1u << r))
                {
                  mask |= 1u << r;
                  offset[r] = imm - size;
                }
            }
          else if ((h & 0xe003) == CINSN_L_SP)
            {
              r = (h >> 7) & 0x1f;
              if (r == 2)
                return 0;                       /* load into sp */
              mask &= ~(1u << r);
            }
          else if ((h & 0xe003) == 0xa001       /* c.j */
                   || (h & 0xc003) == 0xc001)   /* c.beqz, c.bnez */
            return 0;
          else if ((h & 0xf07f) == 0x8002 && (h & 0x0f80))
            return 0;                           /* c.jr (incl. ret) */
          else if ((h & 0xef83) == 0x8102 && (h & 0x007c))
            return 0;                           /* c.mv/c.add sp, rs2 */
          continue;
        }

      if (read_insn_parcel (c, ip + 2, &hi) < 0)
        return 0;
      insn = lo | ((uint32_t) hi << 16);
      ip += 4;

      uint32_t opcode = insn & 0x7f;
      uint32_t rd = (insn >> 7) & 0x1f;

      if ((insn & 0x000fffff) == 0x00010113)    /* addi sp, sp, imm */
        size -= (int32_t) insn >> 20;
      else if ((insn & 0x000ff07f) == INSN_S_SP)
        {
          r = (insn >> 20) & 0x1f;
          imm = (((int32_t) insn >> 25) << 5) | ((insn >> 7) & 0x1f);
          if (TRACKED_REGS & (1u << r))
            {
              mask |= 1u << r;
              offset[r] = imm - size;
            }
        }
      else if ((insn & 0x000ff07f) == INSN_L_SP && rd != 2)
        mask &= ~(1u << rd);
      else if (opcode == 0x6f || opcode == 0x67)  /* jal, jalr */
        {
          if (rd != 1)                          /* not a call */
            return 0;
        }
      else if (opcode == 0x63)                  /* branches */
        return 0;
      else if (rd == 2
               && (opcode == 0x03 || opcode == 0x13 || opcode == 0x17
                   || opcode == 0x1b || opcode == 0x33 || opcode == 0x37
                   || opcode == 0x3b))
        return 0;                               /* other write to sp */
    }

  if (size < 0)
    return 0;

  *frame_size = size;
  *saved = mask;
  for (int i = 0; i < 32; ++i)
    if (mask & (1u << i))
      cfa_offset[i] = offset[i];
  Debug (3, "[start_ip = 0x%lx, ip = 0x%lx) => frame size %d, saved 0x%x\n",
         (long) start_ip, (long) c->dwarf.ip, size, mask);
  return 1;
}

int
unw_step (unw_cursor_t *cursor)
{
  struct cursor *c = (struct cursor *) cursor;
  int validate = c->validate;
  int ret;

  Debug (1, "(cursor=%p, ip=0x%016lx, sp=0x%016lx)\n",
         c, c->dwarf.ip, c->dwarf.cfa);

  /* Validate all addresses before dereferencing. */
  c->validate = 1;

  /* Special handling the signal frame. */
  if (unw_is_signal_frame (cursor) > 0)
    {
      ret = riscv_handle_signal_frame (cursor);
      c->validate = validate;
      return ret;
    }

  /* Restore default memory validation state */
  c->validate = validate;

  /* Try DWARF-based unwinding... */
  c->sigcontext_format = RISCV_SCF_NONE;
  ret = dwarf_step (&c->dwarf);

  if (unlikely (ret == -UNW_ESTOPUNWIND))
    return ret;

  /* DWARF unwinding didn't work, let's tread carefully here */
  if (unlikely (ret < 0))
    {
      Debug (1, "DWARF unwinding failed (cursor=%p, ip=0x%016lx, sp=0x%016lx)\n", c, c->dwarf.ip, c->dwarf.cfa);

      /* Follow the function's prologue if possible: the caller's SP is
         past the stack it allocated, and RA and the callee-saved
         registers may have been saved there.  */
      int32_t frame_size, cfa_offset[32];
      uint32_t saved;
      if (scan_prologue (cursor, &frame_size, &saved, cfa_offset))
        {
          c->dwarf.cfa += frame_size;
          for (int r = 1; r < 32; ++r)
            if (saved & (1u << r))
              c->dwarf.loc[UNW_RISCV_X0 + r] =
                DWARF_LOC (c->dwarf.cfa + cfa_offset[r], 0);
        }

      /* Try RA/X1? */
      c->dwarf.loc[UNW_RISCV_PC] = c->dwarf.loc[UNW_RISCV_X1];
      c->dwarf.loc[UNW_RISCV_X1] = DWARF_NULL_LOC;
      if (!DWARF_IS_NULL_LOC (c->dwarf.loc[UNW_RISCV_PC]))
        {
          ret = dwarf_get (&c->dwarf, c->dwarf.loc[UNW_RISCV_PC], &c->dwarf.ip);
          if (ret < 0)
            {
              Debug (2, "Failed to get PC from return address: %d\n", ret);
              return ret;
            }

          Debug (2, "ra= 0x%016lx\n", c->dwarf.ip);
          /* The IP is a return address now.  */
          c->dwarf.use_prev_instr = 1;
          ret = 1;
        }
      else
        {
          c->dwarf.ip = 0;
        }
    }

  return (c->dwarf.ip == 0) ? 0 : 1;
}
