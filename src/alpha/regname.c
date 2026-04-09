/* libunwind - a platform-independent unwind library

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

static const char *regname[] =
  {
    [UNW_ALPHA_R0]="v0",
    [UNW_ALPHA_R1]="t0",
    [UNW_ALPHA_R2]="t1",
    [UNW_ALPHA_R3]="t2",
    [UNW_ALPHA_R4]="t3",
    [UNW_ALPHA_R5]="t4",
    [UNW_ALPHA_R6]="t5",
    [UNW_ALPHA_R7]="t6",
    [UNW_ALPHA_R8]="t7",
    [UNW_ALPHA_R9]="s0",
    [UNW_ALPHA_R10]="s1",
    [UNW_ALPHA_R11]="s2",
    [UNW_ALPHA_R12]="s3",
    [UNW_ALPHA_R13]="s4",
    [UNW_ALPHA_R14]="s5",
    [UNW_ALPHA_R15]="fp",
    [UNW_ALPHA_R16]="a0",
    [UNW_ALPHA_R17]="a1",
    [UNW_ALPHA_R18]="a2",
    [UNW_ALPHA_R19]="a3",
    [UNW_ALPHA_R20]="a4",
    [UNW_ALPHA_R21]="a5",
    [UNW_ALPHA_R22]="t8",
    [UNW_ALPHA_R23]="t9",
    [UNW_ALPHA_R24]="t10",
    [UNW_ALPHA_R25]="t11",
    [UNW_ALPHA_R26]="ra",
    [UNW_ALPHA_R27]="pv",
    [UNW_ALPHA_R28]="at",
    [UNW_ALPHA_R29]="gp",
    [UNW_ALPHA_R30]="sp",
    [UNW_ALPHA_R31]="zero",

    [UNW_ALPHA_F0]="$f0",
    [UNW_ALPHA_F1]="$f1",
    [UNW_ALPHA_F2]="$f2",
    [UNW_ALPHA_F3]="$f3",
    [UNW_ALPHA_F4]="$f4",
    [UNW_ALPHA_F5]="$f5",
    [UNW_ALPHA_F6]="$f6",
    [UNW_ALPHA_F7]="$f7",
    [UNW_ALPHA_F8]="$f8",
    [UNW_ALPHA_F9]="$f9",
    [UNW_ALPHA_F10]="$f10",
    [UNW_ALPHA_F11]="$f11",
    [UNW_ALPHA_F12]="$f12",
    [UNW_ALPHA_F13]="$f13",
    [UNW_ALPHA_F14]="$f14",
    [UNW_ALPHA_F15]="$f15",
    [UNW_ALPHA_F16]="$f16",
    [UNW_ALPHA_F17]="$f17",
    [UNW_ALPHA_F18]="$f18",
    [UNW_ALPHA_F19]="$f19",
    [UNW_ALPHA_F20]="$f20",
    [UNW_ALPHA_F21]="$f21",
    [UNW_ALPHA_F22]="$f22",
    [UNW_ALPHA_F23]="$f23",
    [UNW_ALPHA_F24]="$f24",
    [UNW_ALPHA_F25]="$f25",
    [UNW_ALPHA_F26]="$f26",
    [UNW_ALPHA_F27]="$f27",
    [UNW_ALPHA_F28]="$f28",
    [UNW_ALPHA_F29]="$f29",
    [UNW_ALPHA_F30]="$f30",
    [UNW_ALPHA_F31]="$f31",

    [UNW_ALPHA_PC]="pc"
   };

const char *
unw_regname (unw_regnum_t reg)
{
  if (reg < (unw_regnum_t) ARRAY_SIZE (regname))
    return regname[reg];
  else
    return "???";
}
