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
int depth = 13;

extern void b (int);

void
raise_exception (void)
{
  unw_cursor_t cursor;
  unw_context_t uc;
  int i;

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

  /* unwind to top-most frame a(): */
  for (i = 0; i < depth - 1; ++i)
    if (unw_step (&cursor) < 0)
      {
	panic ("unw_step() failed!\n");
	return;
      }
  unw_resume (&cursor);	/* transfer control to exception handler */
}

#if !UNW_TARGET_IA64 || defined(__INTEL_COMPILER)

void *
__builtin_ia64_bsp (void)
{
  return NULL;
}

#endif

int
a (int n)
{
  long stack;

  if (verbose)
    printf ("a(n=%d)\n", n);

  if (n > 0)
    return a (n - 1);

  if (verbose)
    printf ("a: sp=%p bsp=%p\n", &stack, __builtin_ia64_bsp ());

  b (16);

  if (verbose)
    {
      printf ("exception handler: here we go (sp=%p, bsp=%p)...\n",
	      &stack, __builtin_ia64_bsp ());
      /* This call works around a bug in gcc (up-to pre3.4) which
	 causes invalid assembly code to be generated when
	 __builtin_ia64_bsp() gets predicated.  */
      getpid ();
    }
  return 0;
}

void
b (int n)
{
  if ((n & 1) == 0)
    {
      if (verbose)
	printf ("b(n=%d) calling raise_exception()\n", n);
      raise_exception ();
    }
  panic ("FAILURE: b() returned from raise_exception()!!\n");
}

int
main (int argc, char **argv)
{
  if (argc > 1)
    {
      ++verbose;
      depth = atol (argv[1]);
    }

  if (a (depth) != 0 || nerrors > 0)
    {
      fprintf (stderr, "FAILURE: test failed; try again?\n");
      exit (-1);
    }

  if (verbose)
    printf ("SUCCESS!\n");
  return 0;
}
