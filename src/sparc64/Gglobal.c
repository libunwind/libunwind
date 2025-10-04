/* libunwind - a platform-independent unwind library
   Copyright (C) 2014 Oracle Inc
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
#include "dwarf_i.h"

HIDDEN define_lock (sparc64_lock);
HIDDEN int tdep_init_done;

/* The API register numbers are exactly the same as the .eh_frame
   registers  */
HIDDEN const uint8_t dwarf_to_unw_regnum_map[DWARF_REGNUM_MAP_LENGTH] =
  {

    [UNW_SPARC64_G0] = UNW_SPARC64_G0,
    [UNW_SPARC64_G1] = UNW_SPARC64_G1,
    [UNW_SPARC64_G2] = UNW_SPARC64_G2,
    [UNW_SPARC64_G3] = UNW_SPARC64_G3,
    [UNW_SPARC64_G4] = UNW_SPARC64_G4,
    [UNW_SPARC64_G5] = UNW_SPARC64_G5,
    [UNW_SPARC64_G6] = UNW_SPARC64_G6,
    [UNW_SPARC64_G7] = UNW_SPARC64_G7,

    /* Output registers.  */

    [UNW_SPARC64_O0] = UNW_SPARC64_O0,
    [UNW_SPARC64_O1] = UNW_SPARC64_O1,
    [UNW_SPARC64_O2] = UNW_SPARC64_O2,
    [UNW_SPARC64_O3] = UNW_SPARC64_O3,
    [UNW_SPARC64_O4] = UNW_SPARC64_O4,
    [UNW_SPARC64_O5] = UNW_SPARC64_O5,
    [UNW_SPARC64_O6] = UNW_SPARC64_O6,
    [UNW_SPARC64_O7] = UNW_SPARC64_O7,

    /* Local registers.  */

    [UNW_SPARC64_L0] = UNW_SPARC64_L0,
    [UNW_SPARC64_L1] = UNW_SPARC64_L1,
    [UNW_SPARC64_L2] = UNW_SPARC64_L2,
    [UNW_SPARC64_L3] = UNW_SPARC64_L3,
    [UNW_SPARC64_L4] = UNW_SPARC64_L4,
    [UNW_SPARC64_L5] = UNW_SPARC64_L5,
    [UNW_SPARC64_L6] = UNW_SPARC64_L6,
    [UNW_SPARC64_L7] = UNW_SPARC64_L7,

    /* Input registers.  */

    [UNW_SPARC64_I0] = UNW_SPARC64_I0,
    [UNW_SPARC64_I1] = UNW_SPARC64_I1,
    [UNW_SPARC64_I2] = UNW_SPARC64_I2,
    [UNW_SPARC64_I3] = UNW_SPARC64_I3,
    [UNW_SPARC64_I4] = UNW_SPARC64_I4,
    [UNW_SPARC64_I5] = UNW_SPARC64_I5,
    [UNW_SPARC64_I6] = UNW_SPARC64_I6,
    [UNW_SPARC64_I7] = UNW_SPARC64_I7,

    /* Floating-point registers.  */

    [UNW_SPARC64_F0] = UNW_SPARC64_F0,
    [UNW_SPARC64_F1] = UNW_SPARC64_F1,
    [UNW_SPARC64_F2] = UNW_SPARC64_F2,
    [UNW_SPARC64_F3] = UNW_SPARC64_F3,
    [UNW_SPARC64_F4] = UNW_SPARC64_F4,
    [UNW_SPARC64_F5] = UNW_SPARC64_F5,
    [UNW_SPARC64_F6] = UNW_SPARC64_F6,
    [UNW_SPARC64_F7] = UNW_SPARC64_F7,
    [UNW_SPARC64_F8] = UNW_SPARC64_F8,
    [UNW_SPARC64_F9] = UNW_SPARC64_F9,
    [UNW_SPARC64_F10] = UNW_SPARC64_F10,
    [UNW_SPARC64_F11] = UNW_SPARC64_F11,
    [UNW_SPARC64_F12] = UNW_SPARC64_F12,
    [UNW_SPARC64_F13] = UNW_SPARC64_F13,
    [UNW_SPARC64_F14] = UNW_SPARC64_F14,
    [UNW_SPARC64_F15] = UNW_SPARC64_F15,
    [UNW_SPARC64_F16] = UNW_SPARC64_F16,
    [UNW_SPARC64_F17] = UNW_SPARC64_F17,
    [UNW_SPARC64_F18] = UNW_SPARC64_F18,
    [UNW_SPARC64_F19] = UNW_SPARC64_F19,
    [UNW_SPARC64_F20] = UNW_SPARC64_F20,
    [UNW_SPARC64_F21] = UNW_SPARC64_F21,
    [UNW_SPARC64_F22] = UNW_SPARC64_F22,
    [UNW_SPARC64_F23] = UNW_SPARC64_F23,
    [UNW_SPARC64_F24] = UNW_SPARC64_F24,
    [UNW_SPARC64_F25] = UNW_SPARC64_F25,
    [UNW_SPARC64_F26] = UNW_SPARC64_F26,
    [UNW_SPARC64_F27] = UNW_SPARC64_F27,
    [UNW_SPARC64_F28] = UNW_SPARC64_F28,
    [UNW_SPARC64_F29] = UNW_SPARC64_F29,
    [UNW_SPARC64_F30] = UNW_SPARC64_F30,
    [UNW_SPARC64_F31] = UNW_SPARC64_F31,
    [UNW_SPARC64_F32] = UNW_SPARC64_F32,
    [UNW_SPARC64_F33] = UNW_SPARC64_F33,
    [UNW_SPARC64_F34] = UNW_SPARC64_F34,
    [UNW_SPARC64_F35] = UNW_SPARC64_F35,
    [UNW_SPARC64_F36] = UNW_SPARC64_F36,
    [UNW_SPARC64_F37] = UNW_SPARC64_F37,
    [UNW_SPARC64_F38] = UNW_SPARC64_F38,
    [UNW_SPARC64_F39] = UNW_SPARC64_F39,
    [UNW_SPARC64_F40] = UNW_SPARC64_F40,
    [UNW_SPARC64_F41] = UNW_SPARC64_F41,
    [UNW_SPARC64_F42] = UNW_SPARC64_F42,
    [UNW_SPARC64_F43] = UNW_SPARC64_F43,
    [UNW_SPARC64_F44] = UNW_SPARC64_F44,
    [UNW_SPARC64_F45] = UNW_SPARC64_F45,
    [UNW_SPARC64_F46] = UNW_SPARC64_F46,
    [UNW_SPARC64_F47] = UNW_SPARC64_F47,
    [UNW_SPARC64_F48] = UNW_SPARC64_F48,
    [UNW_SPARC64_F49] = UNW_SPARC64_F49,
    [UNW_SPARC64_F50] = UNW_SPARC64_F50,
    [UNW_SPARC64_F51] = UNW_SPARC64_F51,
    [UNW_SPARC64_F52] = UNW_SPARC64_F52,
    [UNW_SPARC64_F53] = UNW_SPARC64_F53,
    [UNW_SPARC64_F54] = UNW_SPARC64_F54,
    [UNW_SPARC64_F55] = UNW_SPARC64_F55,
    [UNW_SPARC64_F56] = UNW_SPARC64_F56,
    [UNW_SPARC64_F57] = UNW_SPARC64_F57,
    [UNW_SPARC64_F58] = UNW_SPARC64_F58,
    [UNW_SPARC64_F59] = UNW_SPARC64_F59,
    [UNW_SPARC64_F60] = UNW_SPARC64_F60,
    [UNW_SPARC64_F61] = UNW_SPARC64_F61,
    [UNW_SPARC64_F62] = UNW_SPARC64_F62,
    [UNW_SPARC64_F63] = UNW_SPARC64_F63,
    [UNW_SPARC64_FCC0] = UNW_SPARC64_FCC0,
    [UNW_SPARC64_FCC1] = UNW_SPARC64_FCC1,
    [UNW_SPARC64_FCC2] = UNW_SPARC64_FCC2,
    [UNW_SPARC64_FCC3] = UNW_SPARC64_FCC3,
    [UNW_SPARC64_ICC] = UNW_SPARC64_ICC,
    [UNW_SPARC64_SFP] = UNW_SPARC64_SFP
  };

HIDDEN void
tdep_init (void)
{
  intrmask_t saved_mask;

  sigfillset (&unwi_full_mask);

  lock_acquire (&sparc64_lock, saved_mask);
  {
    if (tdep_init_done)
      /* another thread else beat us to it... */
      goto out;

    mi_init ();

    dwarf_init ();

#ifndef UNW_REMOTE_ONLY
    sparc64_local_addr_space_init ();
#endif
    tdep_init_done = 1;	/* signal that we're initialized... */
  }
 out:
  lock_release (&sparc64_lock, saved_mask);
}
