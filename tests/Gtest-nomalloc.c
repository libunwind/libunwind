/**
 * @file tests/Gtest-nomalloc.c
 *
 * Verify that @c malloc() is not called during an unwinding operation.
 */
/*
 * This file is part of libunwind.
 *   Copyright 2025 Stephen M. Webb <stephen.webb@bregmasoft.ca>
 *   Copyright (C) 2009 Google, Inc
 *   Contributed by Arun Sharma <arun.sharma@google.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#include <dlfcn.h>
#include <libunwind.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "unw_test.h"

int malloc_call_count;
int in_unwind;

/**
 * Intercepted malloc() call.
 *
 * If invoked during unwinding this call will increment the test error count
 * and indicate a failure by returning NULL. Otherwise it just calls the real
 * malloc().
 */
void *
malloc (size_t sz)
{
  typedef void *(*malloc_t) (size_t);

  static malloc_t real_malloc = NULL;
  if (real_malloc == NULL)
    {
      real_malloc = (malloc_t)(intptr_t)dlsym (RTLD_NEXT, "malloc");
      if (real_malloc == NULL)
        {
          fprintf (stderr, "no malloc() found\n");
          exit (UNW_TEST_EXIT_HARD_ERROR);                                          \
        }
    }

  if (in_unwind)
    {
      malloc_call_count++;
    }
  return real_malloc (sz);
}

static void
do_backtrace (void)
{
  unw_cursor_t  cursor;
  unw_context_t uc;
  int           ret;

  in_unwind = 1;
  unw_getcontext (&uc);
  if (unw_init_local (&cursor, &uc) < 0)
    {
      fprintf (stderr, "unw_init_local failed!\n");
      exit (UNW_TEST_EXIT_HARD_ERROR);                                          \
    }

  do
    {
      ret = unw_step (&cursor);
    }
  while (ret > 0);
  in_unwind = 0;
}

void
foo3 (void)
{
  do_backtrace ();
}

void
foo2 (void)
{
  foo3 ();
}

void
foo1 (void)
{
  foo2 ();
}

int
main (void)
{
  foo1 ();

  if (malloc_call_count > 0)
    {
      fprintf (stderr, "FAILURE: malloc called %d times, expected 0\n", malloc_call_count);
      exit (UNW_TEST_EXIT_FAIL);
    }
  exit (UNW_TEST_EXIT_PASS);
}
