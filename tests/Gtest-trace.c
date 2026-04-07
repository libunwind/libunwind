/**
 * @file tests/Gtest-trace.c
 * @brief Verify unw_backtrace() (and backtrace()) give the same backtrace as
 * looping through unw_step().
 */
/*
 * This file is part of libunwind - a platform-independent unwind library.
 *   Copyright (C) 2010, 2011 by FERMI NATIONAL ACCELERATOR LABORATORY
 *   Copyright 2026 Blackberry Limited.
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

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif
#include "unw_test.h"

#include "compiler.h"

#include <errno.h>
#if HAVE_EXECINFO_H
# include <execinfo.h>
#else
  extern int backtrace (void **, int);
#endif
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <libunwind.h>

#include "ident.h"

#define SIG_STACK_SIZE 0x100000

bool verbose = false;       /**< enable verbose mode? */
int repeat_count = 3;       /**< number of times to exercise the signal handler */
int num_errors = 0;         /**< cumulative error count */

enum test_id {
    TEST_UNW_STEP,
    TEST_BACKTRACE,
    TEST_UNW_BACKTRACE,
    TEST_UNW_BACKTRACE2
};
enum { MAX_BACKTRACE_SIZE = 128 };

struct {
	char const *name;
	void       *addresses[MAX_BACKTRACE_SIZE];
} trace[] = {
	{ "unw_step", {0} },
	{ "backtrace", {0} },
	{ "unw_backtrace", {0} },
	{ "unw_backtrace2", {0} }
};

/*
 * These variables are global because they can cause the signal stack to
 * overflow
 */
unw_cursor_t  cursor;
unw_context_t uc;
char          function_name[256];

/**
 * Dump a backtrace (in verbose mode).
 */
static void
dump_backtrace(size_t bt_count, enum test_id id)
{
  if (verbose)
    {
      printf ("### %s() [depth is %zu]\n", trace[id].name, bt_count);
      for (size_t i = 0; i < bt_count; ++i)
        {
          unw_word_t offset = 0;
          int ret = unw_get_proc_name_by_ip (unw_local_addr_space,
                                             (unw_word_t)trace[id].addresses[i],
                                             function_name, sizeof(function_name),
                                             &offset, NULL);
          printf ("    frame %-3zu ip=%10p %s+%#010lx\n",
                  i, trace[id].addresses[i],
                  ret == 0 ? function_name : "???",
                  (long)offset);
        }
    }
}

/**
 * Compare a backtrace from a backtrace call with a backtrace from unw_step().
 */
static void
compare_backtraces(int step_count, int backtrace_count, enum test_id id)
{
  if (step_count != backtrace_count)
    {
      printf ("FAILURE: unw_step() loop and %s() depths differ: %d vs. %d\n",
              trace[id].name,
              step_count,
              backtrace_count);
      ++num_errors;
    }
  else
    {
      for (int i = 1; i < step_count; ++i)
        {
          if (labs (trace[TEST_UNW_STEP].addresses[i] - trace[id].addresses[i]) > 1)
            {
              printf ("FAILURE: unw_step() loop and %s() addresses differ at %d: %p vs. %p\n",
                      trace[id].name, i,
                      trace[TEST_UNW_STEP].addresses[i],
                      trace[id].addresses[i]);
              ++num_errors;
            }
        }
    }
}

/**
 * Exercises `unw_backtrace()`. Compares the result of a raw `unw_step()` loop
 * with the result of calling the system-supplied `backtrace()` (if any) with the
 * result of the `unw_backtrace()` call, which may have taken the "fast path"
 * (using cached data).
 *
 * If there is no system-supplied `backtrace()` call its just an alias for
 * `unw_backtrace()` so the result better match.
 *
 * Also exercises `unw_backtrace2()` using the same context, since the code
 * differs from `unw_backtrace()`.
 */
static void
do_backtrace (void)
{
  unw_word_t ip;
  int ret = -UNW_ENOINFO;
  size_t unw_step_depth = 0;
  size_t backtrace_depth = 0;
  size_t unw_backtrace_depth = 0;

  if (verbose)
    printf ("## backtrace with unw_getcontext()\n");

  ret = unw_getcontext (&uc);
  UNW_TEST_ASSERT(ret == UNW_ESUCCESS, "failure in unw_getcontext()\n");

  ret = unw_init_local (&cursor, &uc);
  UNW_TEST_ASSERT(ret == UNW_ESUCCESS, "failure in unw_init_local()\n");

  do
    {
      unw_get_reg (&cursor, UNW_REG_IP, &ip);
      trace[TEST_UNW_STEP].addresses[unw_step_depth++] = (void *) ip;
    }
  while ((ret = unw_step (&cursor)) > 0 && unw_step_depth < MAX_BACKTRACE_SIZE);
#ifdef UNW_TARGET_ARM
  if (ret == -UNW_ESTOPUNWIND)
    ret = 0;
#endif
  if (ret < 0)
    {
      unw_get_reg (&cursor, UNW_REG_IP, &ip);
      printf ("FAILURE: unw_step() returned %d for ip=%#010lx\n", ret, (long) ip);
      ++num_errors;
    }
  dump_backtrace(unw_step_depth, TEST_UNW_STEP);

  /*
   * Call the system-supplied `backtrace()` (maybe) and compare with the
   * `unw_step()`results. They should be identical.
   */
  backtrace_depth = backtrace (trace[TEST_BACKTRACE].addresses, MAX_BACKTRACE_SIZE);
  dump_backtrace(backtrace_depth, TEST_BACKTRACE);
  compare_backtraces(unw_step_depth, backtrace_depth, TEST_BACKTRACE);

  /* 
   * Call `unw_backtrace()` and compare with the `unw_step()`results. They
   * should be identical.
   */
  unw_backtrace_depth = unw_backtrace (trace[TEST_UNW_BACKTRACE].addresses,
                                       MAX_BACKTRACE_SIZE);
  dump_backtrace(unw_backtrace_depth, TEST_UNW_BACKTRACE);
  compare_backtraces(unw_step_depth, unw_backtrace_depth, TEST_UNW_BACKTRACE);

  /*
   * Call `unw_backtrace2()` on the current context and compare with the
   * `unw_step()`results.  They should be identical.
   */
  int unw_backtrace2_depth = unw_backtrace2 (trace[TEST_UNW_BACKTRACE2].addresses,
                                             MAX_BACKTRACE_SIZE,
                                             &uc,
                                             0);
  dump_backtrace(unw_backtrace2_depth, TEST_UNW_BACKTRACE2);
  compare_backtraces(unw_step_depth, unw_backtrace2_depth, TEST_UNW_BACKTRACE2);
}

/**
 * Exercises `unw_backtrace2()` using the context passed in to a signal
 * handler. Compares the result of a raw `unw_step()` loop with the result of
 * the `unw_backtrace2()` call, which may have taken the "fast path" (using
 * cached data).
 */
void
do_backtrace_with_context(void *context)
{
  unw_word_t ip;
  int ret = -UNW_ENOINFO;
  int unw_step_depth = 0;
  int unw_backtrace2_depth;

  if (verbose)
    printf ("\n## backtrace with with signal context\n");

  ret = unw_init_local2 (&cursor, (unw_context_t*)context, UNW_INIT_SIGNAL_FRAME);
  UNW_TEST_ASSERT(ret == UNW_ESUCCESS, "failure in unw_init_local2()\n");

  do
    {
      unw_get_reg (&cursor, UNW_REG_IP, &ip);
      trace[TEST_UNW_STEP].addresses[unw_step_depth++] = (void *) ip;
    }
  while ((ret = unw_step (&cursor)) > 0 && unw_step_depth < MAX_BACKTRACE_SIZE);
#ifdef UNW_TARGET_ARM
  if (ret == -UNW_ESTOPUNWIND)
    ret = 0;
#endif
  if (ret < 0)
    {
      unw_get_reg (&cursor, UNW_REG_IP, &ip);
      printf ("FAILURE: unw_step() returned %d for ip=%#010lx\n", ret, (long) ip);
      ++num_errors;
    }
  dump_backtrace(unw_step_depth, TEST_UNW_STEP);

  /*
   * Call `unw_backtrace2()` on the passed context and compare with the
   * `unw_step()`results.  They should be identical.
   */
  unw_backtrace2_depth = unw_backtrace2 (trace[TEST_UNW_BACKTRACE2].addresses,
                                         MAX_BACKTRACE_SIZE,
                                         (unw_context_t*)context,
                                         UNW_INIT_SIGNAL_FRAME);
  dump_backtrace(unw_backtrace2_depth, TEST_UNW_BACKTRACE2);
  compare_backtraces(unw_step_depth, unw_backtrace2_depth, TEST_UNW_BACKTRACE2);
}

void
foo (long val UNUSED)
{
  do_backtrace ();
}

void
bar (long v)
{
  int arr[v];
  arr[0] = 0;

  /* This is a vain attempt to use up lots of registers to force
     the frame-chain info to be saved on the memory stack on ia64.
     It happens to work with gcc v3.3.4 and gcc v3.4.1 but perhaps
     not with any other compiler.  */
  foo (f (arr[0]) + (f (v) + (f (v) + (f (v) + (f (v) + (f (v) + (f (v)
       + (f (v) + (f (v) + (f (v) + (f (v) + (f (v) + (f (v) + (f (v)
       + (f (v) + (f (v) + (f (v) + (f (v) + (f (v) + (f (v) + (f (v)
       + (f (v) + (f (v) + (f (v) + (f (v) + (f (v) + (f (v) + (f (v)
       + (f (v) + (f (v) + (f (v) + (f (v) + (f (v) + (f (v) + (f (v)
       + (f (v) + (f (v) + (f (v) + (f (v) + (f (v) + (f (v) + (f (v)
       + (f (v) + (f (v) + (f (v) + (f (v) + (f (v) + (f (v) + (f (v)
       + (f (v) + (f (v) + (f (v) + (f (v) + (f (v) + (f (v) + (f (v)
       + (f (v) + (f (v) + (f (v) + (f (v) + (f (v) + (f (v) + (f (v)
       + (f (v) + (f (v) + (f (v) + (f (v) + (f (v) + (f (v) + (f (v)
       + (f (v) + (f (v) + (f (v) + (f (v) + (f (v) + (f (v) + (f (v)
       + (f (v) + (f (v) + (f (v) + (f (v) + (f (v) + (f (v) + (f (v)
       + (f (v) + (f (v) + (f (v) + (f (v) + (f (v) + (f (v) + (f (v)
       + (f (v) + (f (v) + (f (v) + (f (v) + (f (v) + (f (v) + (f (v)
       + (f (v) + (f (v) + (f (v) + (f (v) + (f (v) + (f (v) + (f (v)
       + (f (v) + (f (v) + (f (v) + (f (v) + (f (v) + (f (v) + (f (v)
       + (f (v) + (f (v) + (f (v) + (f (v) + (f (v) + (f (v) + f (v))
       ))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))
       )))))))))))))))))))))))))))))))))))))))))))))))))))))));
}

void
sighandler (int signal, siginfo_t *siginfo UNUSED, void *context)
{
  if (verbose)
    {
      int sp;
      ucontext_t *ctxt UNUSED = context;

      printf ("sighandler: got signal %d, sp=%p", signal, (void *)&sp);
#if UNW_TARGET_IA64
# if defined(__linux__)
      printf (" @ %#010lx", ctxt->uc_mcontext.sc_ip);
# else
      {
        uint16_t reason;
        uint64_t ip;

        __uc_get_reason (ctxt, &reason);
        __uc_get_ip (ctxt, &ip);
        printf (" @ %#010lx (reason=%d)", ip, reason);
      }
# endif
#elif UNW_TARGET_X86
#if defined __linux__
      printf (" @ %#010lx", (unsigned long) ctxt->uc_mcontext.gregs[REG_EIP]);
#elif defined __FreeBSD__
      printf (" @ %#010lx", (unsigned long) ctxt->uc_mcontext.mc_eip);
#endif
#elif UNW_TARGET_X86_64
#if defined __linux__ || defined __sun
      printf (" @ %#010lx", (unsigned long) ctxt->uc_mcontext.gregs[REG_RIP]);
#elif defined __FreeBSD__
      printf (" @ %#010lx", (unsigned long) ctxt->uc_mcontext.mc_rip);
#endif
#elif defined UNW_TARGET_ARM
#if defined __linux__
      printf (" @ %#010lx", (unsigned long) ctxt->uc_mcontext.arm_pc);
#elif defined __FreeBSD__
      printf (" @ %#010lx", (unsigned long) ctxt->uc_mcontext.__gregs[_REG_PC]);
#endif
#endif
      printf ("\n");
    }

  do_backtrace();
#ifndef UNW_TARGET_ARM
  /* UNW_INIT_SIGNAL_FRAME is not implemented on ARM. */
  do_backtrace_with_context(context);
#endif
}

int
main (int argc, char **argv UNUSED)
{
  verbose = (argc > 1);

  if (verbose)
    printf ("# Normal backtrace\n");

  bar (1);

  if (verbose)
    printf ("\n# Backtrace across signal handler\n");

  struct sigaction act =
    {
      .sa_sigaction = sighandler,
      .sa_flags     = SA_SIGINFO
    };
  int ret = sigaction (SIGTERM, &act, NULL);
  UNW_TEST_ASSERT(ret == 0, "error %d in sigaction(SIGTERM): %s\n", errno, strerror(errno));

  /*
   * Repeatedly invoke the signal hander to make sure any global cache does not
   * get corrupted.
   */
  for (int i = 0; i < repeat_count; ++i)
    {
      kill (getpid (), SIGTERM);
    }

#ifdef HAVE_SIGALTSTACK
  if (verbose)
    printf ("\n# Backtrace across signal handler on alternate stack\n");
  stack_t stk =
    {
      .ss_size  = SIG_STACK_SIZE,
      .ss_flags = 0,
      .ss_sp    = malloc (SIG_STACK_SIZE)
    };
  UNW_TEST_ASSERT(stk.ss_sp != NULL, "failed to allocate %u bytes\n", SIG_STACK_SIZE);
  ret = sigaltstack (&stk, NULL);
  UNW_TEST_ASSERT(ret == 0, "error %d in sigaltstack(): %s\n", errno, strerror(errno));

  memset (&act, 0, sizeof (act));
  act.sa_sigaction = sighandler;
  act.sa_flags = SA_ONSTACK | SA_SIGINFO;
  ret = sigaction (SIGTERM, &act, NULL);
  UNW_TEST_ASSERT(ret == 0, "error %d in sigaction(SIGTERM): %s\n", errno, strerror(errno));
  kill (getpid (), SIGTERM);
#endif /* HAVE_SIGALTSTACK */

  UNW_TEST_ASSERT(num_errors == 0, "FAILURE: detected %d errors\n", num_errors);
  if (verbose)
    printf ("SUCCESS.\n");

  signal (SIGTERM, SIG_DFL);
#ifdef HAVE_SIGALTSTACK
  stk.ss_flags = SS_DISABLE;
  sigaltstack (&stk, NULL);
  free (stk.ss_sp);
#endif /* HAVE_SIGALTSTACK */

  return UNW_TEST_EXIT_PASS;
}
