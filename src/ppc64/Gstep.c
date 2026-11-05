/* libunwind - a platform-independent unwind library
   Copyright (C) 2006-2007 IBM
   Contributed by
     Corey Ashford <cjashfor@us.ibm.com>
     Jose Flavio Aguilar Paulino <jflavio@br.ibm.com> <joseflavio@gmail.com>

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
#include "ucontext_i.h"
#include "remote.h"
#include <signal.h>

/* This definition originates in /usr/include/asm-ppc64/ptrace.h, but is
   defined there only when __KERNEL__ is defined.  We reproduce it here for
   our use at the user level in order to locate the ucontext record, which
   appears to be at this offset relative to the stack pointer when in the
   context of the signal handler return trampoline code -
   __kernel_sigtramp_rt64.  */
#define __SIGNAL_FRAMESIZE 128

/* Read the instruction at ADDR.  */
static int
read_insn (struct cursor *c, unw_word_t addr, uint32_t *insn)
{
  unw_accessors_t *a = unw_get_accessors_int (c->dwarf.as);
  unw_word_t base = addr & ~(unw_word_t) 7, w;
  int ret;

  if ((ret = (*a->access_mem) (c->dwarf.as, base, &w, 0, c->dwarf.as_arg)) < 0)
    return ret;
  if (tdep_big_endian (c->dwarf.as))
    *insn = (addr == base) ? w >> 32 : w & 0xffffffff;
  else
    *insn = (addr == base) ? w & 0xffffffff : w >> 32;
  return 0;
}

/* Whether INSN saves registers in a way that find_saved_regs() does not
   follow: CR or VRSAVE moved to a GPR to be stored, vector stores, stores
   relative to r11 or r12, as for large frames, and r11 or r12 set from
   r1 for that.  */
static int
is_unknown_save (uint32_t insn)
{
  uint32_t op = insn >> 26, rt = (insn >> 21) & 0x1f, ra = (insn >> 16) & 0x1f;
  uint32_t xo = (insn >> 1) & 0x3ff;

  if (op == 31)
    return xo == 19                                   /* mfcr, mfocrf */
           || (xo == 339 && ((insn >> 11) & 0x3ff) == 8) /* mfvrsave */
           || xo == 199 || xo == 231 || xo == 487     /* stvewx, stvx, stvxl */
           || xo == 396 || xo == 908 || xo == 940     /* stxvx, stxvw4x, stxvh8x */
           || xo == 972 || xo == 1004                 /* stxvd2x, stxvb16x */
           || (xo == 444 && ra != rt && (ra == 11 || ra == 12)
               && rt == 1 && ((insn >> 11) & 0x1f) == 1); /* mr r11/r12, r1 */
  if (op == 61)                                       /* stxv, stxsd, ... */
    return 1;
  if (op == 14 && ra == 1 && (rt == 11 || rt == 12))  /* addi r11/r12, r1, d */
    return 1;
  if ((op == 36 || op == 37 || op == 44 || op == 45 || op == 52 || op == 53
       || op == 54 || op == 55 || op == 62)           /* D/DS-form stores */
      && (ra == 11 || ra == 12))
    return 1;
  return 0;
}

/* For a function without unwind info that has set up a stack frame, find
   where its prologue saved the non-volatile GPRs and FPRs.  IP is in the
   function, LOOKUP_IP the address to look its start up by, SP its stack
   pointer and CFA its caller's.  The prologue stores the registers
   relative to r1, before or after the stdu that creates the frame; the
   first branch after the stdu ends it.

   Returns 1 if the prologue could be followed up to its end or the IP,
   and the locations of the registers that it saved have been set; the
   other registers still hold the caller's values then.  Returns 0 if the
   start of the function is not known, or the prologue may have saved
   registers in a way that is not followed: out of line, as GCC does with
   _savegpr0_N at -Os, which shows as a branch before the stdu, or as
   is_unknown_save() describes.  */
static int
find_saved_regs (struct cursor *c, unw_word_t ip, unw_word_t lookup_ip,
                 unw_word_t sp, unw_word_t cfa)
{
  unw_accessors_t *a = unw_get_accessors_int (c->dwarf.as);
  int validate = c->dwarf.as->validate;
  unw_word_t start, offp, addr;
  int framed = 0, ret = 0;
  char name[128];

  if (a->get_proc_name == NULL
      || (*a->get_proc_name) (c->dwarf.as, lookup_ip, name, sizeof (name),
                              &offp, c->dwarf.as_arg) != 0
      || offp > 0x10000)
    return 0;
  start = lookup_ip - offp;

  c->dwarf.as->validate = 1;
  for (addr = start; addr < start + 4 * 64; addr += 4)
    {
      uint32_t insn, s, op;
      int32_t d;

      if (addr >= ip)
        {
          ret = 1;
          break;
        }
      if (read_insn (c, addr, &insn) < 0)
        break;
      s = (insn >> 21) & 0x1f;
      op = insn >> 26;

      if ((insn & 0xffff0003) == 0xf8210001          /* stdu r1, d(r1) */
          || (insn & 0xffff07ff) == 0x7c21016a)      /* stdux r1, r1, rN */
        framed = 1;
      else if ((insn & 0xfc1f0003) == 0xf8010000)     /* std rS, d(r1) */
        {
          d = (int16_t) (insn & 0xfffc);
          if (s >= 14)
            c->dwarf.loc[UNW_PPC64_R0 + s] =
              DWARF_LOC ((framed ? sp : cfa) + d, 0);
        }
      else if ((insn & 0xfc1f0000) == 0xd8010000)     /* stfd fS, d(r1) */
        {
          d = (int16_t) (insn & 0xffff);
          if (s >= 14)
            c->dwarf.loc[UNW_PPC64_F0 + s] =
              DWARF_LOC ((framed ? sp : cfa) + d, 0);
        }
      else if (op == 16 || op == 18
               || (op == 19 && (((insn >> 1) & 0x3ff) == 16      /* bclr */
                                || ((insn >> 1) & 0x3ff) == 528  /* bcctr */
                                || ((insn >> 1) & 0x3ff) == 560))) /* bctar */
        {
          ret = framed;
          break;
        }
      else if (is_unknown_save (insn))
        break;
    }
  c->dwarf.as->validate = validate;

  Debug (3, "prologue at 0x%lx %s\n", (long) start,
         ret ? "followed" : "not understood");
  return ret;
}

/* Whether REG is preserved across calls by the ELF ABI (r13, the thread
   pointer, never changes).  */
static int
is_nonvolatile (unw_regnum_t reg)
{
  return (reg >= UNW_PPC64_R13 && reg <= UNW_PPC64_R31)
         || (reg >= UNW_PPC64_F14 && reg <= UNW_PPC64_F31)
         || (reg >= UNW_PPC64_V20 && reg <= UNW_PPC64_V31)
         || (reg >= UNW_PPC64_CR2 && reg <= UNW_PPC64_CR4)
         || reg == UNW_PPC64_VRSAVE;
}

/* Find the ucontext of the signal frame whose trampoline the cursor is
   at.  The kernel places it __SIGNAL_FRAMESIZE bytes above the stack
   pointer it gives the handler, i.e. above the CFA of the trampoline
   frame.  qemu-user leaves another 16 bytes in between.  Both set
   sigcontext.regs to point at the gp_regs that follow it, so look for
   that at either offset, and assume the kernel's layout if neither
   matches.  */
static unw_word_t
signal_frame_ucontext (struct cursor *c)
{
#if defined(__linux__)
  static const unw_word_t offsets[] = { __SIGNAL_FRAMESIZE,
                                        __SIGNAL_FRAMESIZE + 16 };
  unsigned int i;

  for (i = 0; i < sizeof (offsets) / sizeof (offsets[0]); ++i)
    {
      unw_word_t uc = c->dwarf.cfa + offsets[i];
      unw_word_t regs;

      if (dwarf_get (&c->dwarf, DWARF_LOC (uc + UC_MCONTEXT_REGS_PTR, 0),
                     &regs) >= 0
          && regs == uc + UC_MCONTEXT_GREGS_R0)
        {
          Debug (3, "ucontext at CFA%+ld\n", (long) offsets[i]);
          return uc;
        }
    }
#endif
  return c->dwarf.cfa + __SIGNAL_FRAMESIZE;
}

/* This definition comes from the document "64-bit PowerPC ELF Application
   Binary Interface Supplement 1.9", section 3.2.2.
   http://www.linux-foundation.org/spec/ELF/ppc64/PPC-elf64abi-1.9.html#STACK */

typedef struct
{
  long unsigned back_chain;
  long unsigned cr_save;
  long unsigned lr_save;
  /* many more fields here, but they are unused by this code */
} stack_frame_t;

/* Read a single 32-bit instruction at ADDR via the accessors.  The access_mem
   callback reads sizeof(unw_word_t) (8 bytes on ppc64) at a time and requires
   the kernel's word alignment (PTRACE_PEEKDATA on Linux rejects misaligned
   addresses), so we always read the containing 8-byte-aligned word and
   extract the half we want.  */
static int
_read_instruction (struct dwarf_cursor *c, unw_word_t addr, uint32_t *insn)
{
  unw_accessors_t *a = unw_get_accessors (c->as);
  unw_word_t aligned = addr & ~(unw_word_t) 7;
  unw_word_t w;

  if ((*a->access_mem) (c->as, aligned, &w, 0, c->as_arg) < 0)
    return -1;

  /* On BE ppc, the low-addressed 4 bytes live in the high 32 bits of the
     8-byte word; on LE it's the opposite.  */
  if (c->as->big_endian)
    *insn = (addr == aligned) ? (uint32_t) (w >> 32) : (uint32_t) w;
  else
    *insn = (addr == aligned) ? (uint32_t) w : (uint32_t) (w >> 32);
  return 0;
}

/* Recognise BE ELFv1 PLT call stubs, e.g.:
      std   r2, 40(r1)
      addis r11, r2, X
      ld    r12, X(r11)
      mtctr r12
      ld    r2, X(r11)
      bctr
   IP may be anywhere inside the 24-byte stub.  */
static int
_is_plt_entry_elfv1 (struct dwarf_cursor *c)
{
  static const struct { uint32_t pattern, mask; } insns[6] =
    {
      {0xf8410028, 0xffffffff},  /* std   r2, 40(r1)  */
      {0x3d620000, 0xffff0000},  /* addis r11, r2, X  */
      {0xe98b0000, 0xffff0000},  /* ld    r12, X(r11) */
      {0x7d8903a6, 0xffffffff},  /* mtctr r12         */
      {0xe84b0000, 0xffff0000},  /* ld    r2, X(r11)  */
      {0x4e800420, 0xffffffff},  /* bctr              */
    };
  int offset;

  for (offset = 0; offset < 6; ++offset)
    {
      unw_word_t start = c->ip - (unw_word_t) (offset * 4);
      int i, ok = 1;

      for (i = 0; i < 6; ++i)
        {
          uint32_t insn;
          if (_read_instruction (c, start + i * 4, &insn) < 0
              || (insn & insns[i].mask) != insns[i].pattern)
            {
              ok = 0;
              break;
            }
        }
      if (ok)
        return 1;
    }
  return 0;
}

//! Recognise PLT entries
/*! For example:
   0x000000001000d1f0 <+0>:     18 00 41 f8     std     r2,24(r1)
   0x000000001000d1f4 <+4>:     30 87 82 e9     ld      r12,-30928(r2)
   0x000000001000d1f8 <+8>:     a6 03 89 7d     mtctr   r12
   0x000000001000d1fc <+12>:    20 04 80 4e     bctr
*/
static int
_is_plt_entry (struct dwarf_cursor *c)
{
  unw_word_t w0, w1;
  unw_accessors_t *a;

  if (c->as->big_endian)
    return _is_plt_entry_elfv1 (c);

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
      // ppc64le
      {0xf8410018,0xffffffff},
      {0xe9820000,0xffff0000},
      {0x7d8903a6,0xffffffff},
      {0x4e800420,0xffffffff},
    };

  a = unw_get_accessors (c->as);

  if( (*a->access_mem) (c->as, c->ip, &w0, 0, c->as_arg) < 0 )
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
  if(( w0      & instructions[0].mask) == instructions[0].pattern &&
     ((w0>>32) & instructions[1].mask) == instructions[1].pattern)
  {
    if( (*a->access_mem) (c->as, c->ip+8, &w1, 0, c->as_arg) >= 0 &&
        ( w1      & instructions[2].mask) == instructions[2].pattern &&
        ((w1>>32) & instructions[3].mask) == instructions[3].pattern )
      {
        return 1;
      }
    else
      {
        return 0;
      }
  }
  else if(( w0 & instructions[2].mask) == instructions[2].pattern &&
     ((w0>>32) & instructions[3].mask) == instructions[3].pattern)
  {
    w1 = w0;
    if( (*a->access_mem) (c->as, c->ip-8, &w0, 0, c->as_arg) >= 0 &&
        ( w0      & instructions[0].mask) == instructions[0].pattern &&
        ((w0>>32) & instructions[1].mask) == instructions[1].pattern )
      {
        return 1;
      }
    else
      {
        return 0;
      }
  }
  else if(( w0 & instructions[1].mask) == instructions[1].pattern &&
     ((w0>>32) & instructions[2].mask) == instructions[2].pattern)
  {
    if( (*a->access_mem) (c->as, c->ip-4, &w0, 0, c->as_arg) < 0 ||
        (*a->access_mem) (c->as, c->ip+4, &w1, 0, c->as_arg) < 0 )
    {
      return 0;
    }
  }
  else if( (w0 & instructions[3].mask) == instructions[3].pattern )
  {
    if( (*a->access_mem) (c->as, c->ip-12, &w0, 0, c->as_arg) < 0 ||
        (*a->access_mem) (c->as, c->ip-4, &w1, 0, c->as_arg) < 0 )
    {
      return 0;
    }
  }

  if(( w0      & instructions[0].mask) == instructions[0].pattern &&
     ((w0>>32) & instructions[1].mask) == instructions[1].pattern &&
     ( w1      & instructions[2].mask) == instructions[2].pattern &&
     ((w1>>32) & instructions[3].mask) == instructions[3].pattern )
    {
      return 1;
    }
  else
    {
      return 0;
    }
}


int
unw_is_plt_entry (unw_cursor_t *uc)
{
	return _is_plt_entry (&((struct cursor *)uc)->dwarf);
}


int
unw_step (unw_cursor_t * cursor)
{
  struct cursor *c = (struct cursor *) cursor;
  stack_frame_t dummy;
  unw_word_t back_chain_offset, lr_save_offset, v_regs_ptr;
  struct dwarf_loc back_chain_loc, lr_save_loc, sp_loc, ip_loc, v_regs_loc;
  int ret, i;
  int validate = c->dwarf.as->validate;

  Debug (1, "(cursor=%p, ip=0x%016lx)\n", c, (unsigned long) c->dwarf.ip);

  /* Check for signal frame before trying DWARF-based unwinding.
     This ensures sigcontext_format and sigcontext_addr are always set
     when stepping through a signal trampoline, even when DWARF CFI
     covers the trampoline.  unw_resume needs these to resume via
     rt_sigreturn instead of setcontext().  */
  if (unlikely (unw_is_signal_frame (cursor) > 0))
    goto signal_frame;

  /* Try DWARF-based unwinding... */

  c->sigcontext_format = PPC_SCF_NONE;
  c->dwarf.as->validate = 1;
  ret = dwarf_step (&c->dwarf);
  c->dwarf.as->validate = validate;

  /* -UNW_EBADREG means that a register that the CFI needs is unknown,
     e.g. the caller's r31 after a back-chain step, below; dwarf_step()
     then has not changed the cursor, so fall back to the back chain,
     which needs no registers.  */
  if (ret < 0 && ret != -UNW_ENOINFO && ret != -UNW_EBADREG)
    {
      Debug (2, "returning %d\n", ret);
      return ret;
    }

  if (unlikely (ret < 0))
    {
      /* DWARF failed. */
      if (_is_plt_entry (&c->dwarf))
            {
              Debug (2, "found plt entry\n");

              /*
                Fallback mode that uses the link register. This is important
                for cases where a function without unwind information has been
                called, but not yet set-up its stack frame (basically PLT calls).

                In this case we can't trust c->dwarf.cfa (the stack frame)
                because it will currently point to the caller's stack frame -
                but we can use the current value of the link register to get
                back to the caller. We then have to hope that libunwind manages
                to resume unwinding properly from the caller IP.
              */
              c->dwarf.loc[UNW_PPC64_NIP] = c->dwarf.loc[UNW_PPC64_LR];
              c->dwarf.loc[UNW_PPC64_LR] = DWARF_NULL_LOC;
              if (!DWARF_IS_NULL_LOC (c->dwarf.loc[UNW_PPC64_NIP]))
                {
                  ret = dwarf_get (&c->dwarf, c->dwarf.loc[UNW_PPC64_NIP], &c->dwarf.ip);
                  if (ret < 0)
                    {
                      Debug (2, "failed to get pc from link register: %d\n", ret);
                      return ret;
                    }
                  Debug (2, "link register = 0x%016lx\n", c->dwarf.ip);
                  ret = 1;
                }
              else
                {
                  Debug (2, "link register was not saved\n");
                  c->dwarf.ip = 0;
                }
            }
          else
            {
              /*
                Fallback mode that uses a conventional stack unwinding. This is
                important for functions without proper DWARF unwind information,
                in particular without this fallback the clone() function will not
                unwind properly.

                We do the unwind by first looking for the caller stack pointer
                saved in the current stack frame. This will point to the caller's
                'linkage area'. If the caller stack pointer is null, we assume we
                have reached the top of the stack.

                The address 24(SP) (where SP is the caller stack pointer) contains
                the saved 'link register', the link register is effectively the
                return address for the called function - so we can use the link
                register to get an IP inside the calling function.

                Note: there is no requirement for leaf functions to save the
                stack pointer or link register, I'm not entirely sure why
                libunwind doesn't handle this case.
              */
              Debug (2, "fallback\n");

              back_chain_offset = ((void *) &dummy.back_chain - (void *) &dummy);
              lr_save_offset = ((void *) &dummy.lr_save - (void *) &dummy);

              unw_word_t frame_ip = c->dwarf.ip, frame_sp = c->dwarf.cfa;
              unw_word_t lookup_ip = frame_ip - c->dwarf.use_prev_instr;

              back_chain_loc = DWARF_LOC (c->dwarf.cfa + back_chain_offset, 0);

              if ((ret =
                   dwarf_get (&c->dwarf, back_chain_loc, &c->dwarf.cfa)) < 0)
                {
                  Debug (2,
                     "Unable to retrieve CFA from back chain in stack frame - %d\n",
                     ret);
                  return ret;
                }
              if (c->dwarf.cfa == 0)
                /* Unless the cursor or stack is corrupt or uninitialized we've most
                   likely hit the top of the stack */
                return 0;

              lr_save_loc = DWARF_LOC (c->dwarf.cfa + lr_save_offset, 0);

              if ((ret = dwarf_get (&c->dwarf, lr_save_loc, &c->dwarf.ip)) < 0)
                {
                  Debug (2,
                     "Unable to retrieve IP from lr save in stack frame - %d\n",
                     ret);
                  return ret;
                }
              ret = 1;
              /* The IP is a return address now.  */
              c->dwarf.use_prev_instr = 1;

              /* Describe the caller's registers as DWARF would: the
                 volatile registers, LR, CTR and XER are unknown, and the
                 non-volatile ones are where the prologue saved them, or
                 unchanged if it did not.  If the prologue cannot be
                 followed, which registers it saved and where is not
                 known either: then only r13, the thread pointer, keeps
                 its location.  The caller's r1 is the new CFA.  */
              for (i = 0; i < DWARF_NUM_PRESERVED_REGS; ++i)
                if (!is_nonvolatile (i))
                  c->dwarf.loc[i] = DWARF_NULL_LOC;
              if (!find_saved_regs (c, frame_ip, lookup_ip, frame_sp,
                                    c->dwarf.cfa))
                for (i = 0; i < DWARF_NUM_PRESERVED_REGS; ++i)
                  if (i != UNW_PPC64_R13)
                    c->dwarf.loc[i] = DWARF_NULL_LOC;
            }
    }

  goto step_done;

signal_frame:
    {
      /* Find the sigcontext record by taking the CFA and adjusting by
         the dummy signal frame size.

         Note that there isn't any way to determined if SA_SIGINFO was
         set in the sa_flags parameter to sigaction when the signal
         handler was established.  If it was not set, the ucontext
         record is not required to be on the stack, in which case the
         following code will likely cause a seg fault or other crash
         condition.  */

      unw_word_t ucontext = signal_frame_ucontext (c);

          Debug (1, "signal frame, skip over trampoline\n");

          c->sigcontext_format = PPC_SCF_LINUX_RT_SIGFRAME;
          c->sigcontext_addr = ucontext;

          sp_loc = DWARF_LOC ((ucontext + UC_MCONTEXT_GREGS_R1), 0);
          ip_loc = DWARF_LOC ((ucontext + UC_MCONTEXT_GREGS_NIP), 0);

          ret = dwarf_get (&c->dwarf, sp_loc, &c->dwarf.cfa);
          if (ret < 0)
            {
              Debug (2, "returning %d\n", ret);
              return ret;
            }
          ret = dwarf_get (&c->dwarf, ip_loc, &c->dwarf.ip);
          if (ret < 0)
            {
              Debug (2, "returning %d\n", ret);
              return ret;
            }

          /* Instead of just restoring the non-volatile registers, do all
             of the registers for now.  This will incur a performance hit,
             but it's rare enough not to cause too much of a problem, and
             might be useful in some cases.  */
          c->dwarf.loc[UNW_PPC64_R0] =
            DWARF_LOC (ucontext + UC_MCONTEXT_GREGS_R0, 0);
          c->dwarf.loc[UNW_PPC64_R1] =
            DWARF_LOC (ucontext + UC_MCONTEXT_GREGS_R1, 0);
          c->dwarf.loc[UNW_PPC64_R2] =
            DWARF_LOC (ucontext + UC_MCONTEXT_GREGS_R2, 0);
          c->dwarf.loc[UNW_PPC64_R3] =
            DWARF_LOC (ucontext + UC_MCONTEXT_GREGS_R3, 0);
          c->dwarf.loc[UNW_PPC64_R4] =
            DWARF_LOC (ucontext + UC_MCONTEXT_GREGS_R4, 0);
          c->dwarf.loc[UNW_PPC64_R5] =
            DWARF_LOC (ucontext + UC_MCONTEXT_GREGS_R5, 0);
          c->dwarf.loc[UNW_PPC64_R6] =
            DWARF_LOC (ucontext + UC_MCONTEXT_GREGS_R6, 0);
          c->dwarf.loc[UNW_PPC64_R7] =
            DWARF_LOC (ucontext + UC_MCONTEXT_GREGS_R7, 0);
          c->dwarf.loc[UNW_PPC64_R8] =
            DWARF_LOC (ucontext + UC_MCONTEXT_GREGS_R8, 0);
          c->dwarf.loc[UNW_PPC64_R9] =
            DWARF_LOC (ucontext + UC_MCONTEXT_GREGS_R9, 0);
          c->dwarf.loc[UNW_PPC64_R10] =
            DWARF_LOC (ucontext + UC_MCONTEXT_GREGS_R10, 0);
          c->dwarf.loc[UNW_PPC64_R11] =
            DWARF_LOC (ucontext + UC_MCONTEXT_GREGS_R11, 0);
          c->dwarf.loc[UNW_PPC64_R12] =
            DWARF_LOC (ucontext + UC_MCONTEXT_GREGS_R12, 0);
          c->dwarf.loc[UNW_PPC64_R13] =
            DWARF_LOC (ucontext + UC_MCONTEXT_GREGS_R13, 0);
          c->dwarf.loc[UNW_PPC64_R14] =
            DWARF_LOC (ucontext + UC_MCONTEXT_GREGS_R14, 0);
          c->dwarf.loc[UNW_PPC64_R15] =
            DWARF_LOC (ucontext + UC_MCONTEXT_GREGS_R15, 0);
          c->dwarf.loc[UNW_PPC64_R16] =
            DWARF_LOC (ucontext + UC_MCONTEXT_GREGS_R16, 0);
          c->dwarf.loc[UNW_PPC64_R17] =
            DWARF_LOC (ucontext + UC_MCONTEXT_GREGS_R17, 0);
          c->dwarf.loc[UNW_PPC64_R18] =
            DWARF_LOC (ucontext + UC_MCONTEXT_GREGS_R18, 0);
          c->dwarf.loc[UNW_PPC64_R19] =
            DWARF_LOC (ucontext + UC_MCONTEXT_GREGS_R19, 0);
          c->dwarf.loc[UNW_PPC64_R20] =
            DWARF_LOC (ucontext + UC_MCONTEXT_GREGS_R20, 0);
          c->dwarf.loc[UNW_PPC64_R21] =
            DWARF_LOC (ucontext + UC_MCONTEXT_GREGS_R21, 0);
          c->dwarf.loc[UNW_PPC64_R22] =
            DWARF_LOC (ucontext + UC_MCONTEXT_GREGS_R22, 0);
          c->dwarf.loc[UNW_PPC64_R23] =
            DWARF_LOC (ucontext + UC_MCONTEXT_GREGS_R23, 0);
          c->dwarf.loc[UNW_PPC64_R24] =
            DWARF_LOC (ucontext + UC_MCONTEXT_GREGS_R24, 0);
          c->dwarf.loc[UNW_PPC64_R25] =
            DWARF_LOC (ucontext + UC_MCONTEXT_GREGS_R25, 0);
          c->dwarf.loc[UNW_PPC64_R26] =
            DWARF_LOC (ucontext + UC_MCONTEXT_GREGS_R26, 0);
          c->dwarf.loc[UNW_PPC64_R27] =
            DWARF_LOC (ucontext + UC_MCONTEXT_GREGS_R27, 0);
          c->dwarf.loc[UNW_PPC64_R28] =
            DWARF_LOC (ucontext + UC_MCONTEXT_GREGS_R28, 0);
          c->dwarf.loc[UNW_PPC64_R29] =
            DWARF_LOC (ucontext + UC_MCONTEXT_GREGS_R29, 0);
          c->dwarf.loc[UNW_PPC64_R30] =
            DWARF_LOC (ucontext + UC_MCONTEXT_GREGS_R30, 0);
          c->dwarf.loc[UNW_PPC64_R31] =
            DWARF_LOC (ucontext + UC_MCONTEXT_GREGS_R31, 0);

          c->dwarf.loc[UNW_PPC64_LR] =
            DWARF_LOC (ucontext + UC_MCONTEXT_GREGS_LINK, 0);
          c->dwarf.loc[UNW_PPC64_CTR] =
            DWARF_LOC (ucontext + UC_MCONTEXT_GREGS_CTR, 0);
          /* This CR0 assignment is probably wrong.  There are 8 dwarf columns
             assigned to the CR registers, but only one CR register in the
             mcontext structure */
          c->dwarf.loc[UNW_PPC64_CR0] =
            DWARF_LOC (ucontext + UC_MCONTEXT_GREGS_CCR, 0);
          c->dwarf.loc[UNW_PPC64_XER] =
            DWARF_LOC (ucontext + UC_MCONTEXT_GREGS_XER, 0);
          c->dwarf.loc[UNW_PPC64_NIP] =
            DWARF_LOC (ucontext + UC_MCONTEXT_GREGS_NIP, 0);

          /* TODO: Is there a way of obtaining the value of the
             pseudo frame pointer (which is sp + some fixed offset, I
             assume), based on the contents of the ucontext record
             structure?  For now, set this loc to null. */
          c->dwarf.loc[UNW_PPC64_FRAME_POINTER] = DWARF_NULL_LOC;

          c->dwarf.loc[UNW_PPC64_F0] =
            DWARF_LOC (ucontext + UC_MCONTEXT_FREGS_R0, DWARF_LOC_TYPE_FP);
          c->dwarf.loc[UNW_PPC64_F1] =
            DWARF_LOC (ucontext + UC_MCONTEXT_FREGS_R1, DWARF_LOC_TYPE_FP);
          c->dwarf.loc[UNW_PPC64_F2] =
            DWARF_LOC (ucontext + UC_MCONTEXT_FREGS_R2, DWARF_LOC_TYPE_FP);
          c->dwarf.loc[UNW_PPC64_F3] =
            DWARF_LOC (ucontext + UC_MCONTEXT_FREGS_R3, DWARF_LOC_TYPE_FP);
          c->dwarf.loc[UNW_PPC64_F4] =
            DWARF_LOC (ucontext + UC_MCONTEXT_FREGS_R4, DWARF_LOC_TYPE_FP);
          c->dwarf.loc[UNW_PPC64_F5] =
            DWARF_LOC (ucontext + UC_MCONTEXT_FREGS_R5, DWARF_LOC_TYPE_FP);
          c->dwarf.loc[UNW_PPC64_F6] =
            DWARF_LOC (ucontext + UC_MCONTEXT_FREGS_R6, DWARF_LOC_TYPE_FP);
          c->dwarf.loc[UNW_PPC64_F7] =
            DWARF_LOC (ucontext + UC_MCONTEXT_FREGS_R7, DWARF_LOC_TYPE_FP);
          c->dwarf.loc[UNW_PPC64_F8] =
            DWARF_LOC (ucontext + UC_MCONTEXT_FREGS_R8, DWARF_LOC_TYPE_FP);
          c->dwarf.loc[UNW_PPC64_F9] =
            DWARF_LOC (ucontext + UC_MCONTEXT_FREGS_R9, DWARF_LOC_TYPE_FP);
          c->dwarf.loc[UNW_PPC64_F10] =
            DWARF_LOC (ucontext + UC_MCONTEXT_FREGS_R10, DWARF_LOC_TYPE_FP);
          c->dwarf.loc[UNW_PPC64_F11] =
            DWARF_LOC (ucontext + UC_MCONTEXT_FREGS_R11, DWARF_LOC_TYPE_FP);
          c->dwarf.loc[UNW_PPC64_F12] =
            DWARF_LOC (ucontext + UC_MCONTEXT_FREGS_R12, DWARF_LOC_TYPE_FP);
          c->dwarf.loc[UNW_PPC64_F13] =
            DWARF_LOC (ucontext + UC_MCONTEXT_FREGS_R13, DWARF_LOC_TYPE_FP);
          c->dwarf.loc[UNW_PPC64_F14] =
            DWARF_LOC (ucontext + UC_MCONTEXT_FREGS_R14, DWARF_LOC_TYPE_FP);
          c->dwarf.loc[UNW_PPC64_F15] =
            DWARF_LOC (ucontext + UC_MCONTEXT_FREGS_R15, DWARF_LOC_TYPE_FP);
          c->dwarf.loc[UNW_PPC64_F16] =
            DWARF_LOC (ucontext + UC_MCONTEXT_FREGS_R16, DWARF_LOC_TYPE_FP);
          c->dwarf.loc[UNW_PPC64_F17] =
            DWARF_LOC (ucontext + UC_MCONTEXT_FREGS_R17, DWARF_LOC_TYPE_FP);
          c->dwarf.loc[UNW_PPC64_F18] =
            DWARF_LOC (ucontext + UC_MCONTEXT_FREGS_R18, DWARF_LOC_TYPE_FP);
          c->dwarf.loc[UNW_PPC64_F19] =
            DWARF_LOC (ucontext + UC_MCONTEXT_FREGS_R19, DWARF_LOC_TYPE_FP);
          c->dwarf.loc[UNW_PPC64_F20] =
            DWARF_LOC (ucontext + UC_MCONTEXT_FREGS_R20, DWARF_LOC_TYPE_FP);
          c->dwarf.loc[UNW_PPC64_F21] =
            DWARF_LOC (ucontext + UC_MCONTEXT_FREGS_R21, DWARF_LOC_TYPE_FP);
          c->dwarf.loc[UNW_PPC64_F22] =
            DWARF_LOC (ucontext + UC_MCONTEXT_FREGS_R22, DWARF_LOC_TYPE_FP);
          c->dwarf.loc[UNW_PPC64_F23] =
            DWARF_LOC (ucontext + UC_MCONTEXT_FREGS_R23, DWARF_LOC_TYPE_FP);
          c->dwarf.loc[UNW_PPC64_F24] =
            DWARF_LOC (ucontext + UC_MCONTEXT_FREGS_R24, DWARF_LOC_TYPE_FP);
          c->dwarf.loc[UNW_PPC64_F25] =
            DWARF_LOC (ucontext + UC_MCONTEXT_FREGS_R25, DWARF_LOC_TYPE_FP);
          c->dwarf.loc[UNW_PPC64_F26] =
            DWARF_LOC (ucontext + UC_MCONTEXT_FREGS_R26, DWARF_LOC_TYPE_FP);
          c->dwarf.loc[UNW_PPC64_F27] =
            DWARF_LOC (ucontext + UC_MCONTEXT_FREGS_R27, DWARF_LOC_TYPE_FP);
          c->dwarf.loc[UNW_PPC64_F28] =
            DWARF_LOC (ucontext + UC_MCONTEXT_FREGS_R28, DWARF_LOC_TYPE_FP);
          c->dwarf.loc[UNW_PPC64_F29] =
            DWARF_LOC (ucontext + UC_MCONTEXT_FREGS_R29, DWARF_LOC_TYPE_FP);
          c->dwarf.loc[UNW_PPC64_F30] =
            DWARF_LOC (ucontext + UC_MCONTEXT_FREGS_R30, DWARF_LOC_TYPE_FP);
          c->dwarf.loc[UNW_PPC64_F31] =
            DWARF_LOC (ucontext + UC_MCONTEXT_FREGS_R31, DWARF_LOC_TYPE_FP);
          /* Note that there is no .eh_section register column for the
             FPSCR register.  I don't know why this is.  */

#if defined(__linux__)
          v_regs_loc = DWARF_LOC (ucontext + UC_MCONTEXT_V_REGS, 0);
          ret = dwarf_get (&c->dwarf, v_regs_loc, &v_regs_ptr);
#elif defined(__FreeBSD__)
          /* Offset into main structure. */
          v_regs_ptr = (ucontext + UC_MCONTEXT_V_REGS);
          ret = 0;
#endif

          if (ret < 0)
            {
              Debug (2, "returning %d\n", ret);
              return ret;
            }
          if (v_regs_ptr != 0)
            {
              /* The v_regs_ptr is not null.  Set all of the AltiVec locs */

              c->dwarf.loc[UNW_PPC64_V0] =
                DWARF_LOC (v_regs_ptr + UC_MCONTEXT_VREGS_R0, DWARF_LOC_TYPE_V);
              c->dwarf.loc[UNW_PPC64_V1] =
                DWARF_LOC (v_regs_ptr + UC_MCONTEXT_VREGS_R1, DWARF_LOC_TYPE_V);
              c->dwarf.loc[UNW_PPC64_V2] =
                DWARF_LOC (v_regs_ptr + UC_MCONTEXT_VREGS_R2, DWARF_LOC_TYPE_V);
              c->dwarf.loc[UNW_PPC64_V3] =
                DWARF_LOC (v_regs_ptr + UC_MCONTEXT_VREGS_R3, DWARF_LOC_TYPE_V);
              c->dwarf.loc[UNW_PPC64_V4] =
                DWARF_LOC (v_regs_ptr + UC_MCONTEXT_VREGS_R4, DWARF_LOC_TYPE_V);
              c->dwarf.loc[UNW_PPC64_V5] =
                DWARF_LOC (v_regs_ptr + UC_MCONTEXT_VREGS_R5, DWARF_LOC_TYPE_V);
              c->dwarf.loc[UNW_PPC64_V6] =
                DWARF_LOC (v_regs_ptr + UC_MCONTEXT_VREGS_R6, DWARF_LOC_TYPE_V);
              c->dwarf.loc[UNW_PPC64_V7] =
                DWARF_LOC (v_regs_ptr + UC_MCONTEXT_VREGS_R7, DWARF_LOC_TYPE_V);
              c->dwarf.loc[UNW_PPC64_V8] =
                DWARF_LOC (v_regs_ptr + UC_MCONTEXT_VREGS_R8, DWARF_LOC_TYPE_V);
              c->dwarf.loc[UNW_PPC64_V9] =
                DWARF_LOC (v_regs_ptr + UC_MCONTEXT_VREGS_R9, DWARF_LOC_TYPE_V);
              c->dwarf.loc[UNW_PPC64_V10] =
                DWARF_LOC (v_regs_ptr + UC_MCONTEXT_VREGS_R10, DWARF_LOC_TYPE_V);
              c->dwarf.loc[UNW_PPC64_V11] =
                DWARF_LOC (v_regs_ptr + UC_MCONTEXT_VREGS_R11, DWARF_LOC_TYPE_V);
              c->dwarf.loc[UNW_PPC64_V12] =
                DWARF_LOC (v_regs_ptr + UC_MCONTEXT_VREGS_R12, DWARF_LOC_TYPE_V);
              c->dwarf.loc[UNW_PPC64_V13] =
                DWARF_LOC (v_regs_ptr + UC_MCONTEXT_VREGS_R13, DWARF_LOC_TYPE_V);
              c->dwarf.loc[UNW_PPC64_V14] =
                DWARF_LOC (v_regs_ptr + UC_MCONTEXT_VREGS_R14, DWARF_LOC_TYPE_V);
              c->dwarf.loc[UNW_PPC64_V15] =
                DWARF_LOC (v_regs_ptr + UC_MCONTEXT_VREGS_R15, DWARF_LOC_TYPE_V);
              c->dwarf.loc[UNW_PPC64_V16] =
                DWARF_LOC (v_regs_ptr + UC_MCONTEXT_VREGS_R16, DWARF_LOC_TYPE_V);
              c->dwarf.loc[UNW_PPC64_V17] =
                DWARF_LOC (v_regs_ptr + UC_MCONTEXT_VREGS_R17, DWARF_LOC_TYPE_V);
              c->dwarf.loc[UNW_PPC64_V18] =
                DWARF_LOC (v_regs_ptr + UC_MCONTEXT_VREGS_R18, DWARF_LOC_TYPE_V);
              c->dwarf.loc[UNW_PPC64_V19] =
                DWARF_LOC (v_regs_ptr + UC_MCONTEXT_VREGS_R19, DWARF_LOC_TYPE_V);
              c->dwarf.loc[UNW_PPC64_V20] =
                DWARF_LOC (v_regs_ptr + UC_MCONTEXT_VREGS_R20, DWARF_LOC_TYPE_V);
              c->dwarf.loc[UNW_PPC64_V21] =
                DWARF_LOC (v_regs_ptr + UC_MCONTEXT_VREGS_R21, DWARF_LOC_TYPE_V);
              c->dwarf.loc[UNW_PPC64_V22] =
                DWARF_LOC (v_regs_ptr + UC_MCONTEXT_VREGS_R22, DWARF_LOC_TYPE_V);
              c->dwarf.loc[UNW_PPC64_V23] =
                DWARF_LOC (v_regs_ptr + UC_MCONTEXT_VREGS_R23, DWARF_LOC_TYPE_V);
              c->dwarf.loc[UNW_PPC64_V24] =
                DWARF_LOC (v_regs_ptr + UC_MCONTEXT_VREGS_R24, DWARF_LOC_TYPE_V);
              c->dwarf.loc[UNW_PPC64_V25] =
                DWARF_LOC (v_regs_ptr + UC_MCONTEXT_VREGS_R25, DWARF_LOC_TYPE_V);
              c->dwarf.loc[UNW_PPC64_V26] =
                DWARF_LOC (v_regs_ptr + UC_MCONTEXT_VREGS_R26, DWARF_LOC_TYPE_V);
              c->dwarf.loc[UNW_PPC64_V27] =
                DWARF_LOC (v_regs_ptr + UC_MCONTEXT_VREGS_R27, DWARF_LOC_TYPE_V);
              c->dwarf.loc[UNW_PPC64_V28] =
                DWARF_LOC (v_regs_ptr + UC_MCONTEXT_VREGS_R28, DWARF_LOC_TYPE_V);
              c->dwarf.loc[UNW_PPC64_V29] =
                DWARF_LOC (v_regs_ptr + UC_MCONTEXT_VREGS_R29, DWARF_LOC_TYPE_V);
              c->dwarf.loc[UNW_PPC64_V30] =
                DWARF_LOC (v_regs_ptr + UC_MCONTEXT_VREGS_R30, DWARF_LOC_TYPE_V);
              c->dwarf.loc[UNW_PPC64_V31] =
                DWARF_LOC (v_regs_ptr + UC_MCONTEXT_VREGS_R31, DWARF_LOC_TYPE_V);
              c->dwarf.loc[UNW_PPC64_VRSAVE] =
                DWARF_LOC (v_regs_ptr + UC_MCONTEXT_VREGS_VRSAVE, 0);
              c->dwarf.loc[UNW_PPC64_VSCR] =
                DWARF_LOC (v_regs_ptr + UC_MCONTEXT_VREGS_VSCR, 0);
            }
          else
            {
              c->dwarf.loc[UNW_PPC64_V0] = DWARF_NULL_LOC;
              c->dwarf.loc[UNW_PPC64_V1] = DWARF_NULL_LOC;
              c->dwarf.loc[UNW_PPC64_V2] = DWARF_NULL_LOC;
              c->dwarf.loc[UNW_PPC64_V3] = DWARF_NULL_LOC;
              c->dwarf.loc[UNW_PPC64_V4] = DWARF_NULL_LOC;
              c->dwarf.loc[UNW_PPC64_V5] = DWARF_NULL_LOC;
              c->dwarf.loc[UNW_PPC64_V6] = DWARF_NULL_LOC;
              c->dwarf.loc[UNW_PPC64_V7] = DWARF_NULL_LOC;
              c->dwarf.loc[UNW_PPC64_V8] = DWARF_NULL_LOC;
              c->dwarf.loc[UNW_PPC64_V9] = DWARF_NULL_LOC;
              c->dwarf.loc[UNW_PPC64_V10] = DWARF_NULL_LOC;
              c->dwarf.loc[UNW_PPC64_V11] = DWARF_NULL_LOC;
              c->dwarf.loc[UNW_PPC64_V12] = DWARF_NULL_LOC;
              c->dwarf.loc[UNW_PPC64_V13] = DWARF_NULL_LOC;
              c->dwarf.loc[UNW_PPC64_V14] = DWARF_NULL_LOC;
              c->dwarf.loc[UNW_PPC64_V15] = DWARF_NULL_LOC;
              c->dwarf.loc[UNW_PPC64_V16] = DWARF_NULL_LOC;
              c->dwarf.loc[UNW_PPC64_V17] = DWARF_NULL_LOC;
              c->dwarf.loc[UNW_PPC64_V18] = DWARF_NULL_LOC;
              c->dwarf.loc[UNW_PPC64_V19] = DWARF_NULL_LOC;
              c->dwarf.loc[UNW_PPC64_V20] = DWARF_NULL_LOC;
              c->dwarf.loc[UNW_PPC64_V21] = DWARF_NULL_LOC;
              c->dwarf.loc[UNW_PPC64_V22] = DWARF_NULL_LOC;
              c->dwarf.loc[UNW_PPC64_V23] = DWARF_NULL_LOC;
              c->dwarf.loc[UNW_PPC64_V24] = DWARF_NULL_LOC;
              c->dwarf.loc[UNW_PPC64_V25] = DWARF_NULL_LOC;
              c->dwarf.loc[UNW_PPC64_V26] = DWARF_NULL_LOC;
              c->dwarf.loc[UNW_PPC64_V27] = DWARF_NULL_LOC;
              c->dwarf.loc[UNW_PPC64_V28] = DWARF_NULL_LOC;
              c->dwarf.loc[UNW_PPC64_V29] = DWARF_NULL_LOC;
              c->dwarf.loc[UNW_PPC64_V30] = DWARF_NULL_LOC;
              c->dwarf.loc[UNW_PPC64_V31] = DWARF_NULL_LOC;
              c->dwarf.loc[UNW_PPC64_VRSAVE] = DWARF_NULL_LOC;
              c->dwarf.loc[UNW_PPC64_VSCR] = DWARF_NULL_LOC;
            }

      c->dwarf.pi_valid = 0;
      c->dwarf.use_prev_instr = 0;
      ret = 1;
    }

step_done:
  if (c->dwarf.ip == 0)
    {
      /* Unless the cursor or stack is corrupt or uninitialized,
         we've most likely hit the top of the stack */
      Debug (2, "returning 0\n");
      return 0;
    }

  // on ppc64, R2 register is used as pointer to TOC
  // section which is used for symbol lookup in PIC code
  // ppc64 linker generates "ld r2, 40(r1)" (ELFv1) or
  // "ld r2, 24(r1)" (ELFv2) instruction after each
  // @plt call. We need restore R2, but only for @plt calls
  {
    unw_word_t ip = c->dwarf.ip;
    unw_addr_space_t as = c->dwarf.as;
    unw_accessors_t *a = unw_get_accessors_int (as);
    void *arg = c->dwarf.as_arg;
    uint32_t toc_save = (as->abi == UNW_PPC64_ABI_ELFv2)? 24 : 40;
    int32_t inst;

    if ((ip & 0x3) == 0
	&& fetch32 (as, a, &ip, &inst, arg) >= 0
	&& (uint32_t)inst == (0xE8410000U + toc_save))
      {
	// @plt call, restoring R2 from CFA+toc_save
	c->dwarf.loc[UNW_PPC64_R2] = DWARF_LOC(c->dwarf.cfa + toc_save, 0);
      }
  }

  Debug (2, "returning %d with last return statement\n", ret);
  return ret;
}
