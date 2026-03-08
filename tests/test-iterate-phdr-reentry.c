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

/* This test verifies that find_reg_state does not deadlock when
   iterate_phdr_function reenters libunwind.

   When global caching is enabled and the cache is flushed, the first
   unw_step misses the cache and invokes the iterate_phdr callback,
   which may transitively call unw_step.  If the cache lock is not
   released before calling fetch_proc_info, the second unw_step will
   deadlock on the same lock.  A watchdog thread detects the deadlock
   by exiting after a timeout.  */

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

int reentry_depth;
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
my_iterate_phdr(unw_iterate_phdr_callback_t callback, void *data)
{
  if (reentry_depth == 0)
    {
      unw_cursor_t cursor;
      unw_context_t context;

      reentry_depth++;
      unw_getcontext (&context);
      unw_init_local (&cursor, &context);
      unw_step (&cursor);
      reentry_depth--;
    }

  return dl_iterate_phdr (callback, data);
}

int
main (int argc, char **argv UNUSED)
{
  unw_cursor_t cursor;
  unw_context_t context;
  pthread_t watchdog_thread;
  int ret;

  verbose = argc > 1;

  unw_set_caching_policy (unw_local_addr_space, UNW_CACHE_GLOBAL);
  unw_flush_cache (unw_local_addr_space, 0, 0);
  unw_set_iterate_phdr_function (unw_local_addr_space, my_iterate_phdr);

  if (pthread_create (&watchdog_thread, NULL, watchdog, NULL) != 0)
    {
      fprintf (stderr, "FAILURE: pthread_create failed\n");
      return UNW_TEST_EXIT_HARD_ERROR;
    }

  unw_getcontext (&context);
  UNW_TEST_ASSERT (unw_init_local (&cursor, &context) >= 0,
		   "unw_init_local failed\n");

  ret = unw_step (&cursor);
  UNW_TEST_ASSERT (ret >= 0,
		   "unw_step failed in reentry test (%d)\n", ret);

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
