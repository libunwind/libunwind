/* libunwind - a platform-independent unwind library
   Copyright (C) 2012 Tommi Rantala <tt.rantala@gmail.com>

   Modified for Alpha by Matt Turner <mattst88@gmail.com>

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

/* The Linux kernel signal trampoline on Alpha is:
     mov  $30, $16        (0x47fe0410)
     ldi  $0, __NR_xxx    (0x201f0000 + NR)
     callsys              (0x00000083)

   Where __NR_sigreturn = 103 and __NR_rt_sigreturn = 351.
*/

#define INSN_MOV_R30_R16        0x47fe0410
#define INSN_LDI_R0_SIGRETURN   (0x201f0000 + 103)
#define INSN_LDI_R0_RT_SIGRETURN (0x201f0000 + 351)
#define INSN_CALLSYS            0x00000083

int
unw_is_signal_frame (unw_cursor_t *cursor)
{
#ifdef __linux__
  struct cursor *c = (struct cursor *) cursor;
  unw_word_t w0, w1, ip;
  unw_addr_space_t as;
  unw_accessors_t *a;
  void *arg;
  int ret;

  as = c->dwarf.as;
  a = unw_get_accessors (as);
  arg = c->dwarf.as_arg;

  ip = c->dwarf.ip;

  /* Alpha instructions are 4 bytes and 4-byte aligned.
     Read 3 consecutive instructions at IP.  */
  ret = (*a->access_mem) (as, ip, &w0, 0, arg);
  if (ret < 0)
    return ret;

  ret = (*a->access_mem) (as, ip + 8, &w1, 0, arg);
  if (ret < 0)
    return ret;

  /* Alpha is little-endian; each 8-byte read gets two 4-byte instructions.
     Extract the 3 instructions we need:
       insn0 = low 32 bits of w0  (at ip)
       insn1 = high 32 bits of w0 (at ip+4)
       insn2 = low 32 bits of w1  (at ip+8)  */
  uint32_t insn0 = (uint32_t)(w0);
  uint32_t insn1 = (uint32_t)(w0 >> 32);
  uint32_t insn2 = (uint32_t)(w1);

  /* Check for the sigreturn trampoline */
  if (insn0 == INSN_MOV_R30_R16 &&
      insn1 == INSN_LDI_R0_SIGRETURN &&
      insn2 == INSN_CALLSYS)
    return ALPHA_SCF_LINUX_SIGFRAME;

  /* Check for the rt_sigreturn trampoline */
  if (insn0 == INSN_MOV_R30_R16 &&
      insn1 == INSN_LDI_R0_RT_SIGRETURN &&
      insn2 == INSN_CALLSYS)
    return ALPHA_SCF_LINUX_RT_SIGFRAME;

  return 0;

#else
  return -UNW_ENOINFO;
#endif
}
