/* libunwind - a platform-independent unwind library
   Copyright (C) 2003-2004 Hewlett-Packard Co
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

#include <stdio.h>
#include <stdlib.h>

#include <libunwind.h>

#define NUM_RUNS		1
//#define MAX_CHECKS		1024
#define MAX_CHECKS		2
#define MAX_VALUES_PER_FUNC	4

#define panic(args...)							  \
	do { fprintf (stderr, args); ++nerrors; } while (0)

#define NELEMS(a)	((int) (sizeof (a) / sizeof ((a)[0])))

typedef void save_func_t (void *funcs, unsigned long *vals);
typedef unw_word_t *check_func_t (unw_cursor_t *c, unsigned long *vals);

extern save_func_t save_static_to_stacked;
static check_func_t check_static_to_stacked;

static int verbose;
static int nerrors;

static int num_checks;
static save_func_t *funcs[MAX_CHECKS + 1];
static check_func_t *checks[MAX_CHECKS];
static unw_word_t values[MAX_CHECKS*MAX_VALUES_PER_FUNC];

static struct
  {
    save_func_t *func;
    check_func_t *check;
  }
all_funcs[] =
  {
    { save_static_to_stacked, check_static_to_stacked }
  };

static unw_word_t *
check_static_to_stacked (unw_cursor_t *c, unw_word_t *vals)
{
  unw_word_t r[4];
  unw_word_t nat[4];
  int i, ret;

  if (verbose)
    printf ("%s()\n", __FUNCTION__);

  vals -= 4;

  for (i = 0; i < 4; ++i)
    if ((ret = unw_get_reg (c, UNW_IA64_GR + 4 + i, &r[i])) < 0)
      panic ("%s: failed to read register r%d, error=%d",
	     __FUNCTION__, 4 + i, ret);

  for (i = 0; i < 4; ++i)
    if ((ret = unw_get_reg (c, UNW_IA64_NAT + 4 + i, &nat[i])) < 0)
      panic ("%s: failed to read register nat%d, error=%d",
	     __FUNCTION__, 4 + i, ret);

  for (i = 0; i < 4; ++i)
    {
      if (verbose)
	printf ("  r%d = %c%016lx (expected %c%016lx)\n",
		4 + i, nat[i] ? '*' : ' ', r[i],
		(vals[i] & 1) ? '*' : ' ', vals[i]);

      if (vals[i] & 1)
	{
	  if (!nat[i])
	    panic ("%s: r%d not a NaT!\n", __FUNCTION__, 4 + i);
	}
      else
	{
	  if (nat[i])
	    panic ("%s: r%d a NaT!\n", __FUNCTION__, 4 + i);
	  if (r[i] != vals[i])
	    panic ("%s: r%d=%lx instead of %lx!\n",
		   __FUNCTION__, 4 + i, r[i], vals[i]);
	}
    }
  return vals;
}

static void
start_checks (void *funcs, unsigned long *vals)
{
  unw_context_t uc;
  unw_cursor_t c;
  int i, ret;

  unw_getcontext (&uc);

  if ((ret = unw_init_local (&c, &uc)) < 0)
    panic ("%s: unw_init_local (ret=%d)", __FUNCTION__, ret);

  if ((ret = unw_step (&c)) < 0)
    panic ("%s: unw_step (ret=%d)", __FUNCTION__, ret);

  for (i = 0; i < num_checks; ++i)
    {
      vals = (*checks[i]) (&c, vals);

      if ((ret = unw_step (&c)) < 0)
	panic ("%s: unw_step (ret=%d)", __FUNCTION__, ret);
    }
}

static void
run_check (int test)
{
  int index, i;

  num_checks = (random () % MAX_CHECKS) + 1;

  for (i = 0; i < num_checks * MAX_VALUES_PER_FUNC; ++i)
    values[i] = random ();

  for (i = 0; i < num_checks; ++i)
    {
      index = random () % NELEMS (all_funcs);
      funcs[i] = all_funcs[index].func;
      checks[i] = all_funcs[index].check;
    }

  funcs[num_checks] = start_checks;

printf("starting at funcs[0]=%p\n", funcs[0]);
  (*funcs[0]) (funcs + 1, values);
}

int
main (int argc, char **argv)
{
  int i;

  if (argc > 1)
    verbose = 1;

  for (i = 0; i < NUM_RUNS; ++i)
    run_check (i + 1);

  if (nerrors > 0)
    {
      fprintf (stderr, "FAILURE: detected %d errors\n", nerrors);
      exit (-1);
    }
  if (verbose)
    printf ("SUCCESS.\n");
  return 0;
}
