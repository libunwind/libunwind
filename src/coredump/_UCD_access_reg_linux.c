/* libunwind - a platform-independent unwind library
   Copyright (C) 2012 Tommi Rantala <tt.rantala@gmail.com>

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

#include <stddef.h>

#include "_UCD_internal.h"

#if defined(UNW_TARGET_PPC32) || defined(UNW_TARGET_PPC64) || defined(UNW_TARGET_HPPA) || defined(UNW_TARGET_ALPHA)
# include <asm/ptrace.h>
#endif

int
_UCD_access_reg (unw_addr_space_t  as UNUSED,
                 unw_regnum_t      regnum,
                 unw_word_t       *valp,
                 int               write,
                 void             *arg)
{
  struct UCD_info *ui = arg;

  if (write)
    {
      Debug(0, "write is not supported\n");
      return -UNW_EINVAL;
    }

  if (regnum < 0)
    goto badreg;

#if defined(UNW_TARGET_AARCH64)
  if (regnum >= UNW_AARCH64_FPCR)
    goto badreg;
#elif defined(UNW_TARGET_ARM)
  if (regnum >= 16)
    goto badreg;
#elif defined(UNW_TARGET_SH)
  if (regnum > UNW_SH_PR)
    goto badreg;
#elif defined(UNW_TARGET_IA64)
  if (regnum >= ARRAY_SIZE(ui->prstatus->pr_reg))
    goto badreg;
#elif defined(UNW_TARGET_PPC32) || defined(UNW_TARGET_PPC64)
  if ((unsigned) regnum <= 31u)
    /* R0-R31: UNW_PPCxx_Rn = PT_Rn = n, identity */;
  else
    {
      /* DWARF register numbers for special regs, same on ppc32 and ppc64
       * except NIP (PC): 77 on ppc32, 114 on ppc64.  */
      static const uint8_t ppc_unw[] =
        {
          65,   /* LR  */
          66,   /* CTR */
          68,   /* CCR */
          76,   /* XER */
#if defined(UNW_TARGET_PPC32)
          77,   /* NIP (PC) */
#else
          114,  /* NIP (PC) */
#endif
        };
      static const uint8_t ppc_pt[] =
        {
          PT_LNK, PT_CTR, PT_CCR, PT_XER, PT_NIP,
        };

      size_t i;
      for (i = 0; i < ARRAY_SIZE(ppc_unw); i++)
        if (ppc_unw[i] == (uint8_t) regnum)
          break;
      if (i == ARRAY_SIZE(ppc_unw))
        goto badreg;
      regnum = ppc_pt[i];
    }
#elif defined(UNW_TARGET_HPPA)
  if ((unsigned) regnum <= 31u)
    /* GR0-GR31: identity */;
  else if (regnum == UNW_HPPA_IP)
    regnum = offsetof(struct user_regs_struct, iaoq[0]) / sizeof(long);
  else if (regnum == UNW_HPPA_SAR)
    regnum = offsetof(struct user_regs_struct, sar) / sizeof(long);
  else
    goto badreg;
#elif defined(UNW_TARGET_RISCV)
  if (regnum == UNW_RISCV_PC)
    regnum = 0;
  else if (regnum > UNW_RISCV_X31)
    goto badreg;
#elif defined(UNW_TARGET_LOONGARCH64)
  /* UNW_LOONGARCH64_Rn = n = LOONGARCH_EF_Rn and
   * UNW_LOONGARCH64_PC = 33 = LOONGARCH_EF_CSR_ERA: identity mapping.
   * Index 32 is a gap in the enum; reject it and anything beyond PC.  */
  if ((unsigned) regnum > 31u && regnum != UNW_LOONGARCH64_PC)
    goto badreg;
#else
#if defined(UNW_TARGET_MIPS)

/* glibc and musl use different names */
#ifdef __GLIBC__
#define EF_REG(x) EF_REG ## x
#else
#include <sys/reg.h>
#define EF_REG(x) EF_R ## x
#endif

  static const uint8_t remap_regs[] =
    {
      [UNW_MIPS_R0]  = EF_REG(0),
      [UNW_MIPS_R1]  = EF_REG(1),
      [UNW_MIPS_R2]  = EF_REG(2),
      [UNW_MIPS_R3]  = EF_REG(3),
      [UNW_MIPS_R4]  = EF_REG(4),
      [UNW_MIPS_R5]  = EF_REG(5),
      [UNW_MIPS_R6]  = EF_REG(6),
      [UNW_MIPS_R7]  = EF_REG(7),
      [UNW_MIPS_R8]  = EF_REG(8),
      [UNW_MIPS_R9]  = EF_REG(9),
      [UNW_MIPS_R10] = EF_REG(10),
      [UNW_MIPS_R11] = EF_REG(11),
      [UNW_MIPS_R12] = EF_REG(12),
      [UNW_MIPS_R13] = EF_REG(13),
      [UNW_MIPS_R14] = EF_REG(14),
      [UNW_MIPS_R15] = EF_REG(15),
      [UNW_MIPS_R16] = EF_REG(16),
      [UNW_MIPS_R17] = EF_REG(17),
      [UNW_MIPS_R18] = EF_REG(18),
      [UNW_MIPS_R19] = EF_REG(19),
      [UNW_MIPS_R20] = EF_REG(20),
      [UNW_MIPS_R21] = EF_REG(21),
      [UNW_MIPS_R22] = EF_REG(22),
      [UNW_MIPS_R23] = EF_REG(23),
      [UNW_MIPS_R24] = EF_REG(24),
      [UNW_MIPS_R25] = EF_REG(25),
      [UNW_MIPS_R28] = EF_REG(28),
      [UNW_MIPS_R29] = EF_REG(29),
      [UNW_MIPS_R30] = EF_REG(30),
      [UNW_MIPS_R31] = EF_REG(31),
      [UNW_MIPS_PC]  = EF_CP0_EPC,
    };
#elif defined(UNW_TARGET_S390X)
  static const uint8_t remap_regs[] =
    {
      [UNW_S390X_IP]  = offsetof(struct user_regs_struct, psw.addr) / sizeof(long),
      [UNW_S390X_R0]  = offsetof(struct user_regs_struct, gprs[0]) / sizeof(long),
      [UNW_S390X_R1]  = offsetof(struct user_regs_struct, gprs[1]) / sizeof(long),
      [UNW_S390X_R2]  = offsetof(struct user_regs_struct, gprs[2]) / sizeof(long),
      [UNW_S390X_R3]  = offsetof(struct user_regs_struct, gprs[3]) / sizeof(long),
      [UNW_S390X_R4]  = offsetof(struct user_regs_struct, gprs[4]) / sizeof(long),
      [UNW_S390X_R5]  = offsetof(struct user_regs_struct, gprs[5]) / sizeof(long),
      [UNW_S390X_R6]  = offsetof(struct user_regs_struct, gprs[6]) / sizeof(long),
      [UNW_S390X_R7]  = offsetof(struct user_regs_struct, gprs[7]) / sizeof(long),
      [UNW_S390X_R8]  = offsetof(struct user_regs_struct, gprs[8]) / sizeof(long),
      [UNW_S390X_R9]  = offsetof(struct user_regs_struct, gprs[9]) / sizeof(long),
      [UNW_S390X_R10] = offsetof(struct user_regs_struct, gprs[10]) / sizeof(long),
      [UNW_S390X_R11] = offsetof(struct user_regs_struct, gprs[11]) / sizeof(long),
      [UNW_S390X_R12] = offsetof(struct user_regs_struct, gprs[12]) / sizeof(long),
      [UNW_S390X_R13] = offsetof(struct user_regs_struct, gprs[13]) / sizeof(long),
      [UNW_S390X_R14] = offsetof(struct user_regs_struct, gprs[14]) / sizeof(long),
      [UNW_S390X_R15] = offsetof(struct user_regs_struct, gprs[15]) / sizeof(long),
    };
#elif defined(UNW_TARGET_X86)
  static const uint8_t remap_regs[] =
    {
      /* names from libunwind-x86.h */
      [UNW_X86_EAX]    = offsetof(struct user_regs_struct, eax) / sizeof(long),
      [UNW_X86_EDX]    = offsetof(struct user_regs_struct, edx) / sizeof(long),
      [UNW_X86_ECX]    = offsetof(struct user_regs_struct, ecx) / sizeof(long),
      [UNW_X86_EBX]    = offsetof(struct user_regs_struct, ebx) / sizeof(long),
      [UNW_X86_ESI]    = offsetof(struct user_regs_struct, esi) / sizeof(long),
      [UNW_X86_EDI]    = offsetof(struct user_regs_struct, edi) / sizeof(long),
      [UNW_X86_EBP]    = offsetof(struct user_regs_struct, ebp) / sizeof(long),
      [UNW_X86_ESP]    = offsetof(struct user_regs_struct, esp) / sizeof(long),
      [UNW_X86_EIP]    = offsetof(struct user_regs_struct, eip) / sizeof(long),
      [UNW_X86_EFLAGS] = offsetof(struct user_regs_struct, eflags) / sizeof(long),
      [UNW_X86_TRAPNO] = offsetof(struct user_regs_struct, orig_eax) / sizeof(long),
    };
#elif defined(UNW_TARGET_X86_64)
  static const int8_t remap_regs[] =
    {
      [UNW_X86_64_RAX]    = offsetof(struct user_regs_struct, rax) / sizeof(long),
      [UNW_X86_64_RDX]    = offsetof(struct user_regs_struct, rdx) / sizeof(long),
      [UNW_X86_64_RCX]    = offsetof(struct user_regs_struct, rcx) / sizeof(long),
      [UNW_X86_64_RBX]    = offsetof(struct user_regs_struct, rbx) / sizeof(long),
      [UNW_X86_64_RSI]    = offsetof(struct user_regs_struct, rsi) / sizeof(long),
      [UNW_X86_64_RDI]    = offsetof(struct user_regs_struct, rdi) / sizeof(long),
      [UNW_X86_64_RBP]    = offsetof(struct user_regs_struct, rbp) / sizeof(long),
      [UNW_X86_64_RSP]    = offsetof(struct user_regs_struct, rsp) / sizeof(long),
      [UNW_X86_64_RIP]    = offsetof(struct user_regs_struct, rip) / sizeof(long),
    };
#elif defined(UNW_TARGET_ALPHA)
  if (regnum > UNW_ALPHA_R30 && regnum != UNW_ALPHA_PC)
    goto badreg;

  /* NT_PRSTATUS pr_reg stores R0-R30 sequentially at indices 0-30, with PC
   * at index 31. The EF_* constants in <asm/reg.h> describe the kernel
   * exception frame layout and must not be used to index pr_reg. */
  static const uint8_t remap_regs[UNW_ALPHA_PC + 1] =
    {
      [UNW_ALPHA_R0]  = 0,
      [UNW_ALPHA_R1]  = 1,
      [UNW_ALPHA_R2]  = 2,
      [UNW_ALPHA_R3]  = 3,
      [UNW_ALPHA_R4]  = 4,
      [UNW_ALPHA_R5]  = 5,
      [UNW_ALPHA_R6]  = 6,
      [UNW_ALPHA_R7]  = 7,
      [UNW_ALPHA_R8]  = 8,
      [UNW_ALPHA_R9]  = 9,
      [UNW_ALPHA_R10] = 10,
      [UNW_ALPHA_R11] = 11,
      [UNW_ALPHA_R12] = 12,
      [UNW_ALPHA_R13] = 13,
      [UNW_ALPHA_R14] = 14,
      [UNW_ALPHA_R15] = 15,
      [UNW_ALPHA_R16] = 16,
      [UNW_ALPHA_R17] = 17,
      [UNW_ALPHA_R18] = 18,
      [UNW_ALPHA_R19] = 19,
      [UNW_ALPHA_R20] = 20,
      [UNW_ALPHA_R21] = 21,
      [UNW_ALPHA_R22] = 22,
      [UNW_ALPHA_R23] = 23,
      [UNW_ALPHA_R24] = 24,
      [UNW_ALPHA_R25] = 25,
      [UNW_ALPHA_R26] = 26,
      [UNW_ALPHA_R27] = 27,
      [UNW_ALPHA_R28] = 28,
      [UNW_ALPHA_R29] = 29,
      [UNW_ALPHA_R30] = 30,
      [UNW_ALPHA_PC]  = 31,
    };
#else
#error Port me
#endif

  if (regnum >= (unw_regnum_t)ARRAY_SIZE(remap_regs))
    goto badreg;

  regnum = remap_regs[regnum];
#endif

  /* pr_reg is a long[] array, but it contains struct user_regs_struct's
   * image.
   */
  Debug(1, "pr_reg[%d]:%ld (0x%lx)\n", regnum,
                (long)ui->prstatus->pr_reg[regnum],
                (long)ui->prstatus->pr_reg[regnum]
  );
  *valp = ui->prstatus->pr_reg[regnum];

  return 0;

badreg:
  Debug(0, "bad regnum:%d\n", regnum);
  return -UNW_EINVAL;
}
