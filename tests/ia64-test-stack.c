/* libunwind - a platform-independent unwind library
   Copyright (C) 2003 Hewlett-Packard Co
	Contributed by David Mosberger-Tang <davidm@hpl.hp.com>

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

/* This file tests corner-cases of unwinding across multiple stacks.
   In particular, it verifies that the extreme case with a frame of 96
   stacked registers that are all backed up by separate stacks works
   as expected.  */

#include <libunwind.h>
#include <stdio.h>

/* The loadrs field in ar.rsc is 14 bits wide, which limits all ia64
   implementations to at most 2048 physical stacked registers
   (actually, slightly less than that, because loadrs also counts RNaT
   slots).  Since we can dirty 95 stacked registers per recursion, we
   need to recurse RECURSION_DEPTH times to ensure all physical
   stacked registers are in use. */
#define MAX_PHYS_STACKED	2048
#define RECURSION_DEPTH		((MAX_PHYS_STACKED + 94) / 95)

extern void touch_all (unsigned long recursion_depth);
extern void flushrs (void);

void
do_unwind_tests (void)
{
  unw_word_t ip, sp, bsp, v0, v1, v2, v3, n0, n1, n2, n3;
  unw_context_t uc;
  unw_cursor_t c;
  int ret, reg;

  printf ("do_unwind_tests: here we go!\n");

  unw_getcontext (&uc);
  unw_init_local (&c, &uc);

  do
    {
      if ((ret = unw_get_reg (&c, UNW_IA64_IP, &ip)) < 0
	  || (ret = unw_get_reg (&c, UNW_IA64_SP, &sp)) < 0
	  || (ret = unw_get_reg (&c, UNW_IA64_BSP, &bsp)) < 0)
	break;
      printf ("ip=0x%16lx sp=0x%16lx bsp=0x%16lx\n", ip, sp, bsp);

      for (reg = 32; reg < 128; reg += 4)
	{
	  v0 = v1 = v2 = v3 = 0;
	  n0 = n1 = n2 = n3 = 0;
	  ((ret = unw_get_reg (&c, UNW_IA64_GR + reg, &v0)) < 0
	   || (ret = unw_get_reg (&c, UNW_IA64_NAT + reg, &n0)) < 0
	   || (ret = unw_get_reg (&c, UNW_IA64_GR  + reg + 1, &v1)) < 0
	   || (ret = unw_get_reg (&c, UNW_IA64_NAT + reg + 1, &n1)) < 0
	   || (ret = unw_get_reg (&c, UNW_IA64_GR  + reg + 2, &v2)) < 0
	   || (ret = unw_get_reg (&c, UNW_IA64_NAT + reg + 2, &n2)) < 0
	   || (ret = unw_get_reg (&c, UNW_IA64_GR  + reg + 3, &v3)) < 0
	   || (ret = unw_get_reg (&c, UNW_IA64_NAT + reg + 3, &n3)) < 0);
	  if (reg < 100)
	    printf ("  r%d", reg);
	  else
	    printf (" r%d", reg);
	  printf (" %c%016lx %c%016lx %c%016lx %c%016lx\n",
		  n0 ? '*' : ' ', v0, n1 ? '*' : ' ', v1,
		  n2 ? '*' : ' ', v2, n3 ? '*' : ' ', v3);
	  if (ret < 0)
	    break;
	}
    }
  while ((ret = unw_step (&c)) > 0);

  if (ret < 0)
    printf ("libunwind returned %d\n", ret);
}

int
main (int argc, char **argv)
{
  touch_all (RECURSION_DEPTH);
  return 0;
}
