/* libunwind - a platform-independent unwind library
   Copyright (C) 2001-2003 Hewlett-Packard Co
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
#include <unistd.h>

#include <libunwind.h>

#define panic(args...)				\
	{ ++nerrors; fprintf (stderr, args); }

int nerrors = 0;
int verbose = 0;

static void b (void *);

static void
raise_exception (void *addr)
{
  unw_cursor_t cursor;
  unw_word_t ip;
  unw_context_t uc;

  unw_getcontext (&uc);
  if (unw_init_local (&cursor, &uc) < 0)
    {
      panic ("unw_init_local() failed!\n");
      return;
    }

  /* unwind to frame b(): */
  if (unw_step (&cursor) < 0)
    {
      panic ("unw_step() failed!\n");
      return;
    }

  /* unwind to frame a(): */
  if (unw_step (&cursor) < 0)
    {
      panic ("unw_step() failed!\n");
      return;
    }

  unw_get_reg (&cursor, UNW_REG_IP, &ip);
  if (verbose)
    printf ("old ip = %lx, new ip = %p\n", (long) ip, addr);

  if (unw_set_reg (&cursor, UNW_REG_IP, (unw_word_t) addr) < 0)
    {
      panic ("unw_set_reg() failed!\n");
      return;
    }

  unw_resume (&cursor);	/* transfer control to exception handler */
}

#if !UNW_TARGET_IA64

void *
__builtin_ia64_bsp (void)
{
  return NULL;
}

#endif

static int
a (void)
{
  long stack;

#ifdef __GNUC__
  if (verbose)
    printf ("a: sp=%p bsp=%p\n", &stack, __builtin_ia64_bsp ());
  b (&&handler);
  panic ("FAILURE: unexpected return from func()!\n");

#if UNW_TARGET_IA64
  asm volatile ("1:");	/* force a new bundle */
#endif
 handler:
  if (verbose)
    {
      printf ("exception handler: here we go (sp=%p, bsp=%p)...\n",
	      &stack, __builtin_ia64_bsp ());
      /* This call works around a bug in gcc (up-to pre3.4) which
	 causes invalid assembly code to be generated when
	 __builtin_ia64_bsp() gets predicated.  */
      getpid ();
    }
#else
  if (verbose)
    printf ("a: this test only works with GNU C compiler.\n");
#endif
  return 0;
}

static void
b (void *addr)
{
  if (verbose)
    printf ("b() calling raise_exception()\n");
  raise_exception (addr);
  panic ("FAILURE: b() returned from raise_exception()!!\n");
}

int
main (int argc, char **argv)
{
  if (argc > 1)
    ++verbose;

  if (a () != 0 || nerrors > 0)
    {
      fprintf (stderr, "FAILURE: test failed; try again?\n");
      exit (-1);
    }

  if (verbose)
    printf ("SUCCESS!\n");
  return 0;
}
