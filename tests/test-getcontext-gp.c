/* libunwind - a platform-independent unwind library
   Copyright (C) 2025 Matt Turner <mattst88@gmail.com>

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

/* Verify that unw_getcontext saves the correct GP register value.

   On Alpha and MIPS, the global pointer register is set up per-module
   in each function's prologue.  When getcontext is called across a
   shared library boundary with lazy PLT binding, the dynamic linker's
   PLT resolver can trash GP before the assembly getcontext function
   can save it.

   The inline wrapper in unw_tdep_getcontext captures GP before the
   cross-module call and overwrites the saved value afterward.  This test
   verifies that the saved GP matches the caller's actual value.

   This test is linked with -Wl,-z,lazy to exercise the lazy PLT binding
   path where the bug manifests.  */

#include <libunwind.h>
#include <stdio.h>

#include "unw_test.h"

#if defined(__alpha__)

int
main (void)
{
  unw_context_t uc;
  unsigned long my_gp;

  __asm__ __volatile__ ("mov $29, %0" : "=r" (my_gp));
  unw_getcontext (&uc);

  UNW_TEST_ASSERT((unsigned long) uc.uc_mcontext.sc_regs[29] == my_gp,
    "saved GP=0x%lx, expected 0x%lx\n",
    (unsigned long) uc.uc_mcontext.sc_regs[29], my_gp);

  printf ("PASSED: getcontext saved correct GP (0x%lx)\n", my_gp);
  return UNW_TEST_EXIT_PASS;
}

#elif defined(__mips__)

int
main (void)
{
  unw_context_t uc;
  unsigned long my_gp;

  __asm__ __volatile__ ("move %0, $28" : "=r" (my_gp));
  unw_getcontext (&uc);

  UNW_TEST_ASSERT((unsigned long) uc.uc_mcontext.gregs[28] == my_gp,
    "saved GP=0x%lx, expected 0x%lx\n",
    (unsigned long) uc.uc_mcontext.gregs[28], my_gp);

  printf ("PASSED: getcontext saved correct GP (0x%lx)\n", my_gp);
  return UNW_TEST_EXIT_PASS;
}

#else

int
main (void)
{
  return UNW_TEST_EXIT_SKIP;
}

#endif
