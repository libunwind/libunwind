/* libunwind - a platform-independent unwind library

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

/* This test tries to verify that the cache lock is not
   double-unlocked if fetch_proc_info fails during the handling of a
   cache miss.

   When global caching is enabled and the cache is flushed, unw_step
   releases the cache lock and then calls fetch_proc_info.  If
   fetch_proc_info fails and returns < 0, the code must set cache to
   NULL as a signal to the cleanup code to not call put_rs_cache on a
   lock we no longer hold.

   The test forces the failure by installing a custom iterate_phdr
   callback that can return 0 without calling dl_iterate_phdr, causing
   dwarf_find_proc_info to return -UNW_ENOINFO.  After causing the
   failure path to be taken, it performs a normal unwind to verify the
   cache lock is still in a valid state.  If a double-unlock corrupted
   the mutex, the normal unwind can deadlock.  A watchdog thread
   detects the deadlock by exiting after a timeout.  */

#if defined(HAVE_CONFIG_H)
# include "config.h"
#endif

#if defined(HAVE_DL_ITERATE_PHDR)
# include <link.h>
# include <pthread.h>
# include <stdio.h>
# include <stdlib.h>
# include <unistd.h>

#include "compiler.h"
#include "unw_test.h"

#define UNW_LOCAL_ONLY
#include <libunwind.h>

#define WATCHDOG_TIMEOUT 5

int fail_mode;
int forced_fail_count;
int verbose;

static void *
watchdog (void *arg UNUSED)
{
  sleep (WATCHDOG_TIMEOUT);
  fprintf (stderr, "FAILURE: deadlock detected (watchdog fired after %ds)\n",
           WATCHDOG_TIMEOUT);
  exit (UNW_TEST_EXIT_FAIL);
}

static int
my_iterate_phdr (unw_iterate_phdr_callback_t callback, void *data)
{
  if (fail_mode)
    {
      forced_fail_count++;
      return 0;  /* Forces dwarf_find_proc_info to return -UNW_ENOINFO */
    }

  return dl_iterate_phdr (callback, data);
}

int
main (int argc, char **argv UNUSED)
{
  unw_cursor_t cursor;
  unw_context_t context;
  pthread_t watchdog_thread;

  verbose = argc > 1;

  unw_set_caching_policy (unw_local_addr_space, UNW_CACHE_GLOBAL);
  unw_flush_cache (unw_local_addr_space, 0, 0);
  unw_set_iterate_phdr_function (unw_local_addr_space, my_iterate_phdr);

  if (pthread_create (&watchdog_thread, NULL, watchdog, NULL) != 0)
    {
      fprintf (stderr, "FAILURE: pthread_create failed\n");
      return UNW_TEST_EXIT_HARD_ERROR;
    }

  fail_mode = 1;
  forced_fail_count = 0;
  unw_getcontext (&context);
  UNW_TEST_ASSERT (unw_init_local (&cursor, &context) >= 0,
		   "unw_init_local failed in the forced failure path\n");

  unw_step (&cursor);
  UNW_TEST_ASSERT (forced_fail_count > 0,
		   "forced iterate_phdr failure path not exercised\n");

  fail_mode = 0;

  /* Restore default behavior before a normal unwind. */
  unw_set_iterate_phdr_function (unw_local_addr_space, NULL);

  unw_flush_cache (unw_local_addr_space, 0, 0);
  unw_getcontext (&context);
  unw_init_local (&cursor, &context);
  UNW_TEST_ASSERT (unw_step (&cursor) >= 0,
		   "normal unwind failed after taking the error path\n");

  if (verbose)
    printf ("SUCCESS\n");

  return UNW_TEST_EXIT_PASS;
}

#else
int
main (void)
{
  return UNW_TEST_EXIT_SKIP; /* dl_iterate_phdr is not available */
}
#endif
