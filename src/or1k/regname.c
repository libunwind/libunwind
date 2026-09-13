/* libunwind - a platform-independent unwind library
   Copyright (C) 2012 Tommi Rantala <tt.rantala@gmail.com>
   Copyright (C) 2026 Ali Ahmet Memiş <aliamemis@disroot.org>

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

static const char *const regname[] =
  {
    [UNW_OR1K_R0]  = "r0",
    [UNW_OR1K_R1]  = "r1",
    [UNW_OR1K_R2]  = "r2",
    [UNW_OR1K_R3]  = "r3",
    [UNW_OR1K_R4]  = "r4",
    [UNW_OR1K_R5]  = "r5",
    [UNW_OR1K_R6]  = "r6",
    [UNW_OR1K_R7]  = "r7",
    [UNW_OR1K_R8]  = "r8",
    [UNW_OR1K_R9]  = "r9",
    [UNW_OR1K_R10] = "r10",
    [UNW_OR1K_R11] = "r11",
    [UNW_OR1K_R12] = "r12",
    [UNW_OR1K_R13] = "r13",
    [UNW_OR1K_R14] = "r14",
    [UNW_OR1K_R15] = "r15",
    [UNW_OR1K_R16] = "r16",
    [UNW_OR1K_R17] = "r17",
    [UNW_OR1K_R18] = "r18",
    [UNW_OR1K_R19] = "r19",
    [UNW_OR1K_R20] = "r20",
    [UNW_OR1K_R21] = "r21",
    [UNW_OR1K_R22] = "r22",
    [UNW_OR1K_R23] = "r23",
    [UNW_OR1K_R24] = "r24",
    [UNW_OR1K_R25] = "r25",
    [UNW_OR1K_R26] = "r26",
    [UNW_OR1K_R27] = "r27",
    [UNW_OR1K_R28] = "r28",
    [UNW_OR1K_R29] = "r29",
    [UNW_OR1K_R30] = "r30",
    [UNW_OR1K_R31] = "r31",
    [UNW_OR1K_PC]  = "pc",
  };

const char *
unw_regname (unw_regnum_t reg)
{
  if (reg < (unw_regnum_t) ARRAY_SIZE (regname) && regname[reg] != NULL)
    return regname[reg];
  else
    return "???";
}
