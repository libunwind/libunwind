/* libunwind - a platform-independent unwind library

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

#include "_UCD_internal.h"

int
_UCD_access_reg (unw_addr_space_t as,
                                unw_regnum_t regnum, unw_word_t *valp,
                                int write, void *arg)
{
  if (write)
    {
      Debug(0, "write is not supported\n");
      return -UNW_EINVAL;
    }

  struct UCD_info *ui = arg;

#if defined(UNW_TARGET_X86)
  switch (regnum) {
  case UNW_X86_EAX:
     *valp = ui->prstatus->pr_reg.r_eax;
     break;
  case UNW_X86_EDX:
     *valp = ui->prstatus->pr_reg.r_edx;
     break;
  case UNW_X86_ECX:
     *valp = ui->prstatus->pr_reg.r_ecx;
     break;
  case UNW_X86_EBX:
     *valp = ui->prstatus->pr_reg.r_ebx;
     break;
  case UNW_X86_ESI:
     *valp = ui->prstatus->pr_reg.r_esi;
     break;
  case UNW_X86_EDI:
     *valp = ui->prstatus->pr_reg.r_edi;
     break;
  case UNW_X86_EBP:
     *valp = ui->prstatus->pr_reg.r_ebp;
     break;
  case UNW_X86_ESP:
     *valp = ui->prstatus->pr_reg.r_esp;
     break;
  case UNW_X86_EIP:
     *valp = ui->prstatus->pr_reg.r_eip;
     break;
  case UNW_X86_EFLAGS:
     *valp = ui->prstatus->pr_reg.r_eflags;
     break;
  case UNW_X86_TRAPNO:
     *valp = ui->prstatus->pr_reg.r_trapno;
     break;
  default:
      Debug(0, "bad regnum:%d\n", regnum);
      return -UNW_EINVAL;
  }
#elif defined(UNW_TARGET_X86_64)
  switch (regnum) {
  case UNW_X86_64_RAX:
     *valp = ui->prstatus->pr_reg.r_rax;
     break;
  case UNW_X86_64_RDX:
     *valp = ui->prstatus->pr_reg.r_rdx;
     break;
  case UNW_X86_64_RCX:
     *valp = ui->prstatus->pr_reg.r_rcx;
     break;
  case UNW_X86_64_RBX:
     *valp = ui->prstatus->pr_reg.r_rbx;
     break;
  case UNW_X86_64_RSI:
     *valp = ui->prstatus->pr_reg.r_rsi;
     break;
  case UNW_X86_64_RDI:
     *valp = ui->prstatus->pr_reg.r_rdi;
     break;
  case UNW_X86_64_RBP:
     *valp = ui->prstatus->pr_reg.r_rbp;
     break;
  case UNW_X86_64_RSP:
     *valp = ui->prstatus->pr_reg.r_rsp;
     break;
  case UNW_X86_64_RIP:
     *valp = ui->prstatus->pr_reg.r_rip;
     break;
  default:
      Debug(0, "bad regnum:%d\n", regnum);
      return -UNW_EINVAL;
  }
#elif defined(UNW_TARGET_ARM)
  if (regnum >= UNW_ARM_R0 && regnum <= UNW_ARM_R12) {
     *valp = ui->prstatus->pr_reg.r[regnum];
  } else {
     switch (regnum) {
     case UNW_ARM_R13:
       *valp = ui->prstatus->pr_reg.r_sp;
       break;
     case UNW_ARM_R14:
       *valp = ui->prstatus->pr_reg.r_lr;
       break;
     case UNW_ARM_R15:
       *valp = ui->prstatus->pr_reg.r_pc;
       break;
     default:
       Debug(0, "bad regnum:%d\n", regnum);
       return -UNW_EINVAL;
     }
  }
#elif defined(UNW_TARGET_AARCH64)
  if (regnum >= UNW_AARCH64_X0 && regnum < UNW_AARCH64_X30) {
     *valp = ui->prstatus->pr_reg.x[regnum];
  } else {
     switch (regnum) {
     case UNW_AARCH64_SP:
       *valp = ui->prstatus->pr_reg.sp;
       break;
     case UNW_AARCH64_X30:
       *valp = ui->prstatus->pr_reg.lr;
       break;
     case UNW_AARCH64_PC:
       *valp = ui->prstatus->pr_reg.elr;
       break;
     default:
       Debug(0, "bad regnum:%d\n", regnum);
       return -UNW_EINVAL;
     }
  }
#elif defined(UNW_TARGET_RISCV)
  if (regnum >= UNW_RISCV_X0 && regnum <= UNW_RISCV_X31) {
    switch (regnum) {
    case UNW_RISCV_X0:  *valp = 0;                                      break;
    case UNW_RISCV_X1:  *valp = ui->prstatus->pr_reg.ra;                break;
    case UNW_RISCV_X2:  *valp = ui->prstatus->pr_reg.sp;                break;
    case UNW_RISCV_X3:  *valp = ui->prstatus->pr_reg.gp;                break;
    case UNW_RISCV_X4:  *valp = ui->prstatus->pr_reg.tp;                break;
    case UNW_RISCV_X5:  *valp = ui->prstatus->pr_reg.t[0];              break;
    case UNW_RISCV_X6:  *valp = ui->prstatus->pr_reg.t[1];              break;
    case UNW_RISCV_X7:  *valp = ui->prstatus->pr_reg.t[2];              break;
    case UNW_RISCV_X8:  *valp = ui->prstatus->pr_reg.s[0];              break;
    case UNW_RISCV_X9:  *valp = ui->prstatus->pr_reg.s[1];              break;
    case UNW_RISCV_X10: *valp = ui->prstatus->pr_reg.a[0];              break;
    case UNW_RISCV_X11: *valp = ui->prstatus->pr_reg.a[1];              break;
    case UNW_RISCV_X12: *valp = ui->prstatus->pr_reg.a[2];              break;
    case UNW_RISCV_X13: *valp = ui->prstatus->pr_reg.a[3];              break;
    case UNW_RISCV_X14: *valp = ui->prstatus->pr_reg.a[4];              break;
    case UNW_RISCV_X15: *valp = ui->prstatus->pr_reg.a[5];              break;
    case UNW_RISCV_X16: *valp = ui->prstatus->pr_reg.a[6];              break;
    case UNW_RISCV_X17: *valp = ui->prstatus->pr_reg.a[7];              break;
    case UNW_RISCV_X18: *valp = ui->prstatus->pr_reg.s[2];              break;
    case UNW_RISCV_X19: *valp = ui->prstatus->pr_reg.s[3];              break;
    case UNW_RISCV_X20: *valp = ui->prstatus->pr_reg.s[4];              break;
    case UNW_RISCV_X21: *valp = ui->prstatus->pr_reg.s[5];              break;
    case UNW_RISCV_X22: *valp = ui->prstatus->pr_reg.s[6];              break;
    case UNW_RISCV_X23: *valp = ui->prstatus->pr_reg.s[7];              break;
    case UNW_RISCV_X24: *valp = ui->prstatus->pr_reg.s[8];              break;
    case UNW_RISCV_X25: *valp = ui->prstatus->pr_reg.s[9];              break;
    case UNW_RISCV_X26: *valp = ui->prstatus->pr_reg.s[10];             break;
    case UNW_RISCV_X27: *valp = ui->prstatus->pr_reg.s[11];             break;
    case UNW_RISCV_X28: *valp = ui->prstatus->pr_reg.t[3];              break;
    case UNW_RISCV_X29: *valp = ui->prstatus->pr_reg.t[4];              break;
    case UNW_RISCV_X30: *valp = ui->prstatus->pr_reg.t[5];              break;
    case UNW_RISCV_X31: *valp = ui->prstatus->pr_reg.t[6];              break;
    }
  } else {
    switch (regnum) {
    case UNW_RISCV_PC:
      *valp = ui->prstatus->pr_reg.sepc;
      break;
    default:
      Debug(0, "bad regnum:%d\n", regnum);
      return -UNW_EINVAL;
    }
  }

#else
#error Port me
#endif

  return 0;
}
