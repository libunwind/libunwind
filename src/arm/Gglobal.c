/* libunwind - a platform-independent unwind library
   Copyright (C) 2008 CodeSourcery

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

HIDDEN define_lock (arm_lock);
HIDDEN atomic_bool tdep_init_done = 0;

/* Unwinding methods to use. See UNW_METHOD_ enums */
#if defined(__ANDROID__)
/* Android only supports three types of unwinding methods. */
int unwi_unwind_method = UNW_ARM_METHOD_DWARF | UNW_ARM_METHOD_EXIDX | UNW_ARM_METHOD_LR;
#else
int unwi_unwind_method = UNW_ARM_METHOD_ALL;
#endif

HIDDEN void
tdep_init (void)
{
  intrmask_t saved_mask;

  sigfillset (&unwi_full_mask);

  lock_acquire (&arm_lock, saved_mask);
  {
    if (atomic_load(&tdep_init_done))
      /* another thread else beat us to it... */
      goto out;

    /* read ARM unwind method setting */
    const char* str = getenv ("UNW_ARM_UNWIND_METHOD");
    if (str)
      {
        unwi_unwind_method = atoi (str);
      }

    mi_init ();

    dwarf_init ();

#ifndef UNW_REMOTE_ONLY
    arm_local_addr_space_init ();
#endif
    atomic_store(&tdep_init_done, 1); /* signal that we're initialized... */
  }
 out:
  lock_release (&arm_lock, saved_mask);
}
