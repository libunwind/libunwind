/* libunwind - a platform-independent unwind library
   Copyright (C) 2001-2002 Hewlett-Packard Co
	Contributed by David Mosberger-Tang <davidm@hpl.hp.com>

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

/* This illustrates the basics of using the unwind interface for
   exception handling.  */

#include <stdio.h>
#include <stdlib.h>

#define UNW_LOCAL_ONLY
#include <libunwind.h>

#define panic(args...)				\
	{ fprintf (stderr, args); exit (-1); }

int true = 1;

static void
raise_exception (void *addr)
{
  unw_cursor_t cursor;
  unw_word_t ip;
  unw_context_t uc;

  unw_getcontext (&uc);
  if (unw_init_local (&cursor, &uc) < 0)
    panic ("unw_init_local() failed!\n");

  /* unwind to frame b(): */
  if (unw_step (&cursor) < 0)
    panic ("unw_step() failed!\n");

  /* unwind to frame a(): */
  if (unw_step (&cursor) < 0)
    panic ("unw_step() failed!\n");

  unw_get_reg (&cursor, UNW_REG_IP, &ip);
  printf ("ip = %lx\n", ip);

  if (unw_set_reg (&cursor, UNW_REG_IP, (unw_word_t) addr) < 0)
    panic ("unw_set_reg() failed!\n");

  unw_resume (&cursor);	/* transfer control to exception handler */
}

static void
b (void *addr)
{
  printf ("b() calling raise_exception()\n");
  raise_exception (addr);
  printf ("b(): back from raise_exception!!\n");
}

static int
a (void)
{
  register long sp asm ("r12");
  printf("a: sp=%lx bsp=%p\n", sp, __builtin_ia64_bsp ());
  b (&&handler);
  printf ("unexpected return from func()!\n");

  if (true)
    return -1;

 handler:
  printf ("exception handler: here we go (sp=%lx, bsp=%p)...\n",
	  sp, __builtin_ia64_bsp ());
  return 0;
}

int
main (int argc, char **argv)
{
  if (a () == 0)
    printf ("test succeeded!\n");
  else
    printf ("bummer: test failed; try again?\n");
  return 0;
}
