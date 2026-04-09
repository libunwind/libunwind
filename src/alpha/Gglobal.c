/* libunwind - a platform-independent unwind library
   Copyright (c) 2003, 2005 Hewlett-Packard Development Company, L.P.
        Contributed by David Mosberger-Tang <davidm@hpl.hp.com>

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

#include "config.h"
#include "unwind_i.h"
#include "dwarf_i.h"

HIDDEN define_lock (alpha_lock);
HIDDEN atomic_bool tdep_init_done = 0;

/* The API register numbers are exactly the same as the DWARF register
   numbers for Alpha.  */
HIDDEN const uint8_t dwarf_to_unw_regnum_map[DWARF_NUM_PRESERVED_REGS] =
  {
    /* 0-31: integer registers */
    UNW_ALPHA_R0,
    UNW_ALPHA_R1,
    UNW_ALPHA_R2,
    UNW_ALPHA_R3,
    UNW_ALPHA_R4,
    UNW_ALPHA_R5,
    UNW_ALPHA_R6,
    UNW_ALPHA_R7,
    UNW_ALPHA_R8,
    UNW_ALPHA_R9,
    UNW_ALPHA_R10,
    UNW_ALPHA_R11,
    UNW_ALPHA_R12,
    UNW_ALPHA_R13,
    UNW_ALPHA_R14,
    UNW_ALPHA_R15,
    UNW_ALPHA_R16,
    UNW_ALPHA_R17,
    UNW_ALPHA_R18,
    UNW_ALPHA_R19,
    UNW_ALPHA_R20,
    UNW_ALPHA_R21,
    UNW_ALPHA_R22,
    UNW_ALPHA_R23,
    UNW_ALPHA_R24,
    UNW_ALPHA_R25,
    UNW_ALPHA_R26,
    UNW_ALPHA_R27,
    UNW_ALPHA_R28,
    UNW_ALPHA_R29,
    UNW_ALPHA_R30,
    UNW_ALPHA_R31,

    /* 32-63: floating-point registers */
    UNW_ALPHA_F0,
    UNW_ALPHA_F1,
    UNW_ALPHA_F2,
    UNW_ALPHA_F3,
    UNW_ALPHA_F4,
    UNW_ALPHA_F5,
    UNW_ALPHA_F6,
    UNW_ALPHA_F7,
    UNW_ALPHA_F8,
    UNW_ALPHA_F9,
    UNW_ALPHA_F10,
    UNW_ALPHA_F11,
    UNW_ALPHA_F12,
    UNW_ALPHA_F13,
    UNW_ALPHA_F14,
    UNW_ALPHA_F15,
    UNW_ALPHA_F16,
    UNW_ALPHA_F17,
    UNW_ALPHA_F18,
    UNW_ALPHA_F19,
    UNW_ALPHA_F20,
    UNW_ALPHA_F21,
    UNW_ALPHA_F22,
    UNW_ALPHA_F23,
    UNW_ALPHA_F24,
    UNW_ALPHA_F25,
    UNW_ALPHA_F26,
    UNW_ALPHA_F27,
    UNW_ALPHA_F28,
    UNW_ALPHA_F29,
    UNW_ALPHA_F30,
    UNW_ALPHA_F31,

    /* 64: PC */
    UNW_ALPHA_PC,

    /* 65: CFA (not a real register) */
    UNW_ALPHA_CFA,
  };

HIDDEN void
tdep_init (void)
{
  intrmask_t saved_mask;

  sigfillset (&unwi_full_mask);

  lock_acquire (&alpha_lock, saved_mask);
  {
    if (atomic_load(&tdep_init_done))
      /* another thread else beat us to it... */
      goto out;

    mi_init ();

    dwarf_init ();

#ifndef UNW_REMOTE_ONLY
    alpha_local_addr_space_init ();
#endif
    atomic_store(&tdep_init_done, 1); /* signal that we're initialized... */
  }
 out:
  lock_release (&alpha_lock, saved_mask);
}
