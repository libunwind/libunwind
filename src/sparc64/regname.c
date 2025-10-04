/* libunwind - a platform-independent unwind library
   Copyright (C) 2014 Oracle Inc.
   Contributed by
      Jose E. Marchesi <jose.marchesi@oracle.com>

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
    [UNW_SPARC64_G0] = "G0",
    [UNW_SPARC64_G1] = "G1",
    [UNW_SPARC64_G2] = "G2",
    [UNW_SPARC64_G3] = "G3",
    [UNW_SPARC64_G4] = "G4",
    [UNW_SPARC64_G5] = "G5",
    [UNW_SPARC64_G6] = "G6",
    [UNW_SPARC64_G7] = "G7",
    [UNW_SPARC64_O0] = "O0",
    [UNW_SPARC64_O1] = "O1",
    [UNW_SPARC64_O2] = "O2",
    [UNW_SPARC64_O3] = "O3",
    [UNW_SPARC64_O4] = "O4",
    [UNW_SPARC64_O5] = "O5",
    [UNW_SPARC64_O6] = "O6",
    [UNW_SPARC64_O7] = "O7",
    [UNW_SPARC64_L0] = "L0",
    [UNW_SPARC64_L1] = "L1",
    [UNW_SPARC64_L2] = "L2",
    [UNW_SPARC64_L3] = "L3",
    [UNW_SPARC64_L4] = "L4",
    [UNW_SPARC64_L5] = "L5",
    [UNW_SPARC64_L6] = "L6",
    [UNW_SPARC64_L7] = "L7",
    [UNW_SPARC64_I0] = "I0",
    [UNW_SPARC64_I1] = "I1",
    [UNW_SPARC64_I2] = "I2",
    [UNW_SPARC64_I3] = "I3",
    [UNW_SPARC64_I4] = "I4",
    [UNW_SPARC64_I5] = "I5",
    [UNW_SPARC64_I6] = "I6",
    [UNW_SPARC64_I7] = "I7",
    [UNW_SPARC64_F0] = "F0",
    [UNW_SPARC64_F1] = "F1",
    [UNW_SPARC64_F2] = "F2",
    [UNW_SPARC64_F3] = "F3",
    [UNW_SPARC64_F4] = "F4",
    [UNW_SPARC64_F5] = "F5",
    [UNW_SPARC64_F6] = "F6",
    [UNW_SPARC64_F7] = "F7",
    [UNW_SPARC64_F8] = "F8",
    [UNW_SPARC64_F9] = "F9",
    [UNW_SPARC64_F10] = "F10",
    [UNW_SPARC64_F11] = "F11",
    [UNW_SPARC64_F12] = "F12",
    [UNW_SPARC64_F13] = "F13",
    [UNW_SPARC64_F14] = "F14",
    [UNW_SPARC64_F15] = "F15",
    [UNW_SPARC64_F16] = "F16",
    [UNW_SPARC64_F17] = "F17",
    [UNW_SPARC64_F18] = "F18",
    [UNW_SPARC64_F19] = "F19",
    [UNW_SPARC64_F20] = "F20",
    [UNW_SPARC64_F21] = "F21",
    [UNW_SPARC64_F22] = "F22",
    [UNW_SPARC64_F23] = "F23",
    [UNW_SPARC64_F24] = "F24",
    [UNW_SPARC64_F25] = "F25",
    [UNW_SPARC64_F26] = "F26",
    [UNW_SPARC64_F27] = "F27",
    [UNW_SPARC64_F28] = "F28",
    [UNW_SPARC64_F29] = "F29",
    [UNW_SPARC64_F30] = "F30",
    [UNW_SPARC64_F31] = "F31",
    [UNW_SPARC64_F32] = "F32",
    [UNW_SPARC64_F33] = "F33",
    [UNW_SPARC64_F34] = "F34",
    [UNW_SPARC64_F35] = "F35",
    [UNW_SPARC64_F36] = "F36",
    [UNW_SPARC64_F37] = "F37",
    [UNW_SPARC64_F38] = "F38",
    [UNW_SPARC64_F39] = "F39",
    [UNW_SPARC64_F40] = "F40",
    [UNW_SPARC64_F41] = "F41",
    [UNW_SPARC64_F42] = "F42",
    [UNW_SPARC64_F43] = "F43",
    [UNW_SPARC64_F44] = "F44",
    [UNW_SPARC64_F45] = "F45",
    [UNW_SPARC64_F46] = "F46",
    [UNW_SPARC64_F47] = "F47",
    [UNW_SPARC64_F48] = "F48",
    [UNW_SPARC64_F49] = "F49",
    [UNW_SPARC64_F50] = "F50",
    [UNW_SPARC64_F51] = "F51",
    [UNW_SPARC64_F52] = "F52",
    [UNW_SPARC64_F53] = "F53",
    [UNW_SPARC64_F54] = "F54",
    [UNW_SPARC64_F55] = "F55",
    [UNW_SPARC64_F56] = "F56",
    [UNW_SPARC64_F57] = "F57",
    [UNW_SPARC64_F58] = "F58",
    [UNW_SPARC64_F59] = "F59",
    [UNW_SPARC64_F60] = "F60",
    [UNW_SPARC64_F61] = "F61",
    [UNW_SPARC64_F62] = "F62",
    [UNW_SPARC64_F63] = "F63",
    [UNW_SPARC64_FCC0] = "FCC0",
    [UNW_SPARC64_FCC1] = "FCC1",
    [UNW_SPARC64_FCC2] = "FCC2",
    [UNW_SPARC64_FCC3] = "FCC3",
    [UNW_SPARC64_ICC]  = "ICC",
    [UNW_SPARC64_SFP]  = "SFP",
    [UNW_SPARC64_GSR]  = "GSR"
   };

const char *
unw_regname (unw_regnum_t reg)
{
  if (reg < (unw_regnum_t) ARRAY_SIZE (regname))
    return regname[reg];
  else
    return "???";
}
