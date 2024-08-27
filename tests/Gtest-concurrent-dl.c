/*  Verify that libunwind can call into the dynamic loader and vice versa
    without deadlocking. */
/*
 * This file is part of libunwind.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <link.h>

#include "compiler.h"

#include <libunwind.h>
#include <limits.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define NITER 100
#define NATTEMPTS 10

#define panic(...)						\
  do { fprintf (stderr, __VA_ARGS__); ++nerrors; } while (0)

int nerrors;
int verbose;
atomic_bool worker_done;

void
do_unwind (void)
{
  unw_word_t ip;
  unw_context_t uc;
  unw_cursor_t c;
  int ret;

  unw_getcontext (&uc);
  unw_init_local (&c, &uc);
  do
    {
      unw_get_reg (&c, UNW_REG_IP, &ip);
      if (verbose)
	printf ("%lx: IP=%lx\n", (long) pthread_self (), (unsigned long) ip);
    }
  while ((ret = unw_step (&c)) > 0);

  if (ret < 0)
    panic ("unw_step() returned %d\n", ret);
}

int
dl_callback(struct dl_phdr_info *info, size_t size, void *data)
{
  do_unwind();
  return 0;
}

void *
unwind_outside_dl(void *arg UNUSED)
{
  do_unwind();
  return NULL;
}

void *
unwind_inside_dl (void *arg UNUSED)
{
  dl_iterate_phdr(dl_callback, NULL);
  return NULL;
}

void
iter_work (void)
{
  pthread_t t1, t2;
  pthread_attr_t attr;

  pthread_attr_init (&attr);
  pthread_attr_setstacksize (&attr, PTHREAD_STACK_MIN + 64*1024);

  pthread_create (&t1, &attr, unwind_inside_dl, NULL);
  pthread_create (&t2, &attr, unwind_outside_dl, NULL);
  pthread_join (t1, NULL);
  pthread_join (t1, NULL);
}

void *
dispatch_work (void *arg UNUSED)
{
  for (int i = 0; i < NITER; ++i)
    iter_work ();

  atomic_store (&worker_done, true);
  return NULL;
}

int
main(int argc, char* argv[])
{
  if (argc > 1)
    verbose = 1;

  pthread_t worker;
  pthread_create (&worker, NULL, dispatch_work, NULL);

  for (int i = 0; i < NATTEMPTS; ++i)
    {
      if (atomic_load(&worker_done))
        {
          if (verbose)
            printf ("SUCCESS\n");

          pthread_join (worker, NULL);
          return 0;
        }
      sleep(1);
    }

  fprintf (stderr, "FAILURE: detected a deadlock between libunwind and DL\n");
  return -1;
}
