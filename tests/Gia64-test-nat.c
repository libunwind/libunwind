/* libunwind - a platform-independent unwind library
   Copyright (C) 2004 Hewlett-Packard Co
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

/* This file tests corner-cases of NaT-bit handling.  */

#include <stdio.h>
#include <stdlib.h>

#include <libunwind.h>

//#define NUM_RUNS		1024
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

extern save_func_t save_static_to_fr;
static check_func_t check_static_to_fr;

extern save_func_t save_static_to_br;
static check_func_t check_static_to_br;

extern save_func_t save_static_to_mem;
static check_func_t check_static_to_mem;

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
    //    { save_static_to_stacked,	check_static_to_stacked },
    //    { save_static_to_fr,	check_static_to_fr }
    //    { save_static_to_br,	check_static_to_br }
    { save_static_to_mem,	check_static_to_mem }
  };

static unw_word_t *
check_static_to_stacked (unw_cursor_t *c, unw_word_t *vals)
{
  unw_word_t r[4];
  unw_word_t nat[4];
  int i, ret;

  if (verbose)
    printf ("  %s()\n", __FUNCTION__);

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
	printf ("    r%d = %c%016lx (expected %c%016lx)\n",
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

static unw_word_t *
check_static_to_fr (unw_cursor_t *c, unw_word_t *vals)
{
  unw_word_t r4;
  unw_word_t nat4;
  int ret;

  if (verbose)
    printf ("  %s()\n", __FUNCTION__);

  vals -= 1;

  if ((ret = unw_get_reg (c, UNW_IA64_GR + 4, &r4)) < 0)
    panic ("%s: failed to read register r4, error=%d", __FUNCTION__, ret);

  if ((ret = unw_get_reg (c, UNW_IA64_NAT + 4, &nat4)) < 0)
    panic ("%s: failed to read register nat4, error=%d", __FUNCTION__, ret);

  if (verbose)
    printf ("    r4 = %c%016lx (expected %c%016lx)\n",
	    nat4 ? '*' : ' ', r4, (vals[0] & 1) ? '*' : ' ', vals[0]);

  if (vals[0] & 1)
    {
      if (!nat4)
	panic ("%s: r4 not a NaT!\n", __FUNCTION__);
    }
  else
    {
      if (nat4)
	panic ("%s: r4 a NaT!\n", __FUNCTION__);
      if (r4 != vals[0])
	panic ("%s: r4=%lx instead of %lx!\n", __FUNCTION__, r4, vals[0]);
    }
  return vals;
}

static unw_word_t *
check_static_to_br (unw_cursor_t *c, unw_word_t *vals)
{
  unw_word_t r4, nat4, pr;
  int ret;

  if (verbose)
    printf ("  %s()\n", __FUNCTION__);

  if ((ret = unw_get_reg (c, UNW_IA64_PR, &pr)) < 0)
    panic ("%s: failed to read register pr, error=%d", __FUNCTION__, ret);

  if (!(pr & 1))
    /* r4 contained a NaT, so the routine didn't do anything.  */
    return vals;

  vals -= 1;

  if ((ret = unw_get_reg (c, UNW_IA64_GR + 4, &r4)) < 0)
    panic ("%s: failed to read register r4, error=%d", __FUNCTION__, ret);

  if ((ret = unw_get_reg (c, UNW_IA64_NAT + 4, &nat4)) < 0)
    panic ("%s: failed to read register nat4, error=%d", __FUNCTION__, ret);

  if (verbose)
    printf ("    r4 = %c%016lx (expected %c%016lx)\n",
	    nat4 ? '*' : ' ', r4, (vals[0] & 1) ? '*' : ' ', vals[0]);

  if (vals[0] & 1)
    {
      if (!nat4)
	panic ("%s: r4 not a NaT!\n", __FUNCTION__);
    }
  else
    {
      if (nat4)
	panic ("%s: r4 a NaT!\n", __FUNCTION__);
      if (r4 != vals[0])
	panic ("%s: r4=%lx instead of %lx!\n", __FUNCTION__, r4, vals[0]);
    }
  return vals;
}

static unw_word_t *
check_static_to_mem (unw_cursor_t *c, unw_word_t *vals)
{
  unw_word_t r5, nat5;
  int ret;

  if (verbose)
    printf ("  %s()\n", __FUNCTION__);

  vals -= 1;

  if ((ret = unw_get_reg (c, UNW_IA64_GR + 5, &r5)) < 0)
    panic ("%s: failed to read register r5, error=%d", __FUNCTION__, ret);

  if ((ret = unw_get_reg (c, UNW_IA64_NAT + 5, &nat5)) < 0)
    panic ("%s: failed to read register nat5, error=%d", __FUNCTION__, ret);

  if (verbose)
    printf ("    r5 = %c%016lx (expected %c%016lx)\n",
	    nat5 ? '*' : ' ', r5, (vals[0] & 1) ? '*' : ' ', vals[0]);

  if (vals[0] & 1)
    {
      if (!nat5)
	panic ("%s: r5 not a NaT!\n", __FUNCTION__);
    }
  else
    {
      if (nat5)
	panic ("%s: r5 a NaT!\n", __FUNCTION__);
      if (r5 != vals[0])
	panic ("%s: r5=%lx instead of %lx!\n", __FUNCTION__, r5, vals[0]);
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

  (*funcs[0]) (funcs + 1, values);
}

int
main (int argc, char **argv)
{
  int i;

  if (argc > 1)
    verbose = 1;

  for (i = 0; i < NUM_RUNS; ++i)
    {
      if (verbose)
	printf ("Run %d", i + 1);
      run_check (i + 1);
    }

  if (nerrors > 0)
    {
      fprintf (stderr, "FAILURE: detected %d errors\n", nerrors);
      exit (-1);
    }
  if (verbose)
    printf ("SUCCESS.\n");
  return 0;
}
