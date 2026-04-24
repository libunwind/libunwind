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

/* End-to-end test for AArch64 LR fallback unwinding.

   Tests that libunwind can unwind through functions without .eh_frame
   (no DWARF unwind info) using the link register fallback path.  Exercises
   three prologue patterns:
     1) SUB SP, SP, #imm
     2) STP Xt1, Xt2, [SP, #-imm]!
     3) STR Xt, [SP, #-imm]!

   Each test variant:
     - Spawns a worker thread that calls the assembly stub in a loop
     - Main thread sends SIGUSR1 to the worker
     - Signal handler unwinds with unw_init_local2 + UNW_INIT_SIGNAL_FRAME
     - Verifies the backtrace reaches the outer caller function
     - Verifies unw_backtrace2() matches unw_step() through the same frames  */

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <ucontext.h>
#include <unistd.h>
#include <stdatomic.h>
#include <dlfcn.h>

#include "compiler.h"
#include "unw_test.h"

#include <libunwind.h>

/* Assembly functions from aarch64-test-lr-fallback-asm.S */
extern void no_eh_frame_sub_sp(atomic_bool *flag);
extern void no_eh_frame_stp_pre_index(atomic_bool *flag);
extern void no_eh_frame_str_pre_index(atomic_bool *flag);

/* Target function that the test expects to find in the backtrace. */
static const char *expected_caller = NULL;

/* Result communicated from signal handler to main thread. */
static volatile int handler_result = -1;  /* -1=not run, 0=pass, 1=fail */
static volatile int handler_frame_count = 0;

int verbose;

enum { MAX_BT_DEPTH = 20 };

static void
signal_handler (int sig UNUSED, siginfo_t *info UNUSED, void *ucontext)
{
  unw_cursor_t cursor;
  unw_word_t ip;
  char sym[256];
  unw_word_t offset;
  int found = 0;
  int frames = 0;
  void *step_ips[MAX_BT_DEPTH];
  void *bt2_ips[MAX_BT_DEPTH];
  int bt2_depth;

  if (unw_init_local2 (&cursor, (unw_context_t *) ucontext,
                        UNW_INIT_SIGNAL_FRAME) != 0)
    {
      handler_result = 1;
      return;
    }

  unw_get_reg (&cursor, UNW_REG_IP, &ip);
  step_ips[frames] = (void *) ip;
  frames++;

  /* Walk the stack looking for expected_caller, saving IPs. */
  while (unw_step (&cursor) > 0 && frames < MAX_BT_DEPTH)
    {
      unw_get_reg (&cursor, UNW_REG_IP, &ip);
      step_ips[frames] = (void *) ip;
      frames++;

      if (unw_get_proc_name (&cursor, sym, sizeof (sym), &offset) == 0)
        {
          if (expected_caller && strstr (sym, expected_caller))
            found = 1;
        }
    }

  handler_frame_count = frames;

  if (!found)
    {
      handler_result = 1;
      return;
    }

  /* Now call unw_backtrace2() on the same context and compare. */
  bt2_depth = unw_backtrace2 (bt2_ips, MAX_BT_DEPTH,
                              (unw_context_t *) ucontext,
                              UNW_INIT_SIGNAL_FRAME);

  if (bt2_depth != frames)
    {
      if (verbose)
        fprintf (stderr,
                 "FAILURE: unw_step depth (%d) != unw_backtrace2 depth (%d)\n",
                 frames, bt2_depth);
      handler_result = 1;
      return;
    }

  for (int i = 0; i < frames; i++)
    {
      if (labs ((long) step_ips[i] - (long) bt2_ips[i]) > 1)
        {
          if (verbose)
            fprintf (stderr,
                     "FAILURE: IPs differ at frame %d: step=%p bt2=%p\n",
                     i, step_ips[i], bt2_ips[i]);
          handler_result = 1;
          return;
        }
    }

  handler_result = 0;
}

typedef void (*stub_fn)(atomic_bool *);

static atomic_bool worker_running;
static stub_fn worker_stub;

static NOINLINE void
outer_caller (void)
{
  while (atomic_load (&worker_running))
    worker_stub (&worker_running);
}

static void *
worker_thread (void *arg UNUSED)
{
  outer_caller ();
  return NULL;
}

/*
 * Run a single test variant.
 */
static int
run_test (const char *name, stub_fn fn)
{
  pthread_t tid;
  int attempts;

  expected_caller = "outer_caller";
  worker_stub = fn;
  atomic_store (&worker_running, 1);

  if (pthread_create (&tid, NULL, worker_thread, NULL) != 0)
    {
      fprintf (stderr, "FAILURE: %s: pthread_create failed\n", name);
      return UNW_TEST_EXIT_HARD_ERROR;
    }

  /* Give the worker time to enter the spin loop. */
  usleep (10000);

  /* Send signals until the handler runs successfully or we give up. */
  for (attempts = 0; attempts < 10000; attempts++)
    {
      handler_result = -1;
      handler_frame_count = 0;
      pthread_kill (tid, SIGUSR1);
      usleep (100);

      if (handler_result == 0)
        break;
    }

  atomic_store (&worker_running, 0);
  pthread_join (tid, NULL);

  if (handler_result == 0)
    {
      if (verbose)
        printf ("  %-30s PASS (%d frames, %d attempts)\n",
                name, handler_frame_count, attempts + 1);
      return UNW_TEST_EXIT_PASS;
    }

  fprintf (stderr,
           "FAILURE: %s: outer_caller not found in %d frames after %d attempts\n",
           name, handler_frame_count, attempts + 1);
  return UNW_TEST_EXIT_FAIL;
}

int
main (int argc, char **argv UNUSED)
{
  int ret;
  struct sigaction sa;

  verbose = argc > 1;

  sa.sa_sigaction = signal_handler;
  sa.sa_flags = SA_SIGINFO;
  sigemptyset (&sa.sa_mask);
  if (sigaction (SIGUSR1, &sa, NULL) != 0)
    {
      fprintf (stderr, "FAILURE: sigaction failed\n");
      return UNW_TEST_EXIT_HARD_ERROR;
    }

  ret = run_test ("sub_sp_imm", no_eh_frame_sub_sp);
  if (ret != UNW_TEST_EXIT_PASS)
    return ret;

  ret = run_test ("stp_pre_index", no_eh_frame_stp_pre_index);
  if (ret != UNW_TEST_EXIT_PASS)
    return ret;

  ret = run_test ("str_pre_index", no_eh_frame_str_pre_index);
  if (ret != UNW_TEST_EXIT_PASS)
    return ret;

  if (verbose)
    printf ("SUCCESS\n");

  return UNW_TEST_EXIT_PASS;
}
