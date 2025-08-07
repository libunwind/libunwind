/**
 * @file tests/Garm64-test-sve-signal.c
 *
 * Verify that unwinding from a signal handler works when variable width
 * SVE registers are pushed onto the stack.
 *
 * Requires that both libunwind is built with SVE support enabled (that is, the
 * compiler supports `-march=armv8-a+sve` and that this test is being run on an
 * OS with SVE support enabled (which requires both hardware support and OS
 * support).
 */
/*
 * This file is part of libunwind.
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

#include "unw_test.h"

#include <stdio.h>

#if defined(__ARM_FEATURE_SVE) && defined(__ARM_FEATURE_SVE_VECTOR_OPERATORS)

# include <arm_sve.h>
# include <signal.h>
# include <stdbool.h>
# include <stdlib.h>
# include <string.h>
# include <time.h>
# include <unistd.h>

# include "libunwind.h"

# define UNW_TEST_SIGNAL_FRAME "signal_frame"

# if defined(__linux__)
#  include <sys/auxv.h>

#  define UNW_TEST_KILL_SYSCALL "kill"

/**
 * Probe for SVE support in the host Linux kernel.
 */
bool
sve_is_enabled (void)
{
  return (getauxval (AT_HWCAP) & HWCAP_SVE) ? true : false;
}

# elif defined(__QNX__)
#  include <sys/syspage.h>

#  define UNW_TEST_KILL_SYSCALL "SignalKill"

/**
 * Probe for SVE support in the host QNX OS kernel.
 */
bool
sve_is_enabled (void)
{
  return SYSPAGE_ENTRY (cpuinfo)->flags & AARCH64_CPU_FLAG_SVE;
}
# else

#  define UNW_TEST_KILL_SYSCALL "kill"

/**
 * Assume there is no SVE suppoirt in the host kernel.
 */
bool
sve_is_enabled (void)
{
  return false;
}
# endif

bool    verbose = false;

int64_t z[100];

void
signal_handler (int signum)
{
  unw_cursor_t  cursor;
  unw_context_t context;

  const char *  expected[] = {
    UNW_TEST_SIGNAL_FRAME, UNW_TEST_KILL_SYSCALL, "sum", "square", "main",
  };

  unw_getcontext (&context);
  unw_init_local (&cursor, &context);

  for (unsigned int depth = 0; depth < sizeof (expected) / sizeof (expected[0]); ++depth)
    {
      unw_word_t pc;
      int        unw_rc = unw_step (&cursor);
      if (unw_rc <= 0)
        {
          fprintf (stderr, "Frame: %d  unw_step error: %d\n", depth, unw_rc);
          exit (UNW_TEST_EXIT_FAIL);
        }

      unw_rc = unw_get_reg (&cursor, UNW_REG_IP, &pc);
      if (pc == 0 || unw_rc != 0)
        {
          fprintf (stderr, "Frame: %d  unw_get_reg error: %d\n", depth, unw_rc);
          exit (UNW_TEST_EXIT_FAIL);
        }

      char sym[256];
      unw_rc = unw_is_signal_frame (&cursor);
      if (unw_rc > 0)
        {
          strcpy (sym, UNW_TEST_SIGNAL_FRAME);
        }
      else if (unw_rc < 0)
        {
          fprintf (stderr, "Frame: %d  unw_is_signal_frame error: %d\n", depth, unw_rc);
          exit (UNW_TEST_EXIT_FAIL);
        }
      else
        {
          unw_word_t offset;
          unw_rc = unw_get_proc_name (&cursor, sym, sizeof (sym), &offset);
          if (unw_rc)
            {
              fprintf (stderr, "Frame: %d  unw_get_proc_name error: %d\n", depth, unw_rc);
              exit (UNW_TEST_EXIT_FAIL);
            }
        }
      if (verbose)
        fprintf (stdout, " IP=%#010lx \"%s\"\n", (unsigned long)pc, sym);

      if (strcmp (sym, expected[depth]) != 0)
        {
          fprintf (stderr, "Frame: %d  expected %s but found %s\n", depth, expected[depth], sym);
          exit (UNW_TEST_EXIT_FAIL);
        }
    }

  exit (UNW_TEST_EXIT_PASS);
}

int64_t
sum (svint64_t z0)
{
  int64_t ret = svaddv_s64 (svptrue_b64 (), z0);
  kill (getpid (), SIGUSR1);
  return ret;
}

int64_t
square (svint64_t z0)
{
  int64_t res = 0;
  for (int i = 0; i < 100; ++i)
    {
      z0 = svmul_s64_z (svptrue_b64 (), z0, z0);
      res += sum (z0);
    }
  return res;
}

int
main (int argc, char *argv[])
{
  verbose = (argc > 1);
  if (!sve_is_enabled ())
    {
      fprintf (stderr, "SVE is not enabled: skip\n");
      return UNW_TEST_EXIT_SKIP;
    }

  signal (SIGUSR1, signal_handler);
  for (unsigned int i = 0; i < sizeof (z) / sizeof (z[0]); ++i)
    z[i] = rand ();

  svint64_t z0 = svld1 (svptrue_b64 (), &z[0]);
  square (z0);

  /*
   * Shouldn't get here, exit is called from signal handler
   */
  fprintf (stderr, "Signal handler wasn't called\n");
  return UNW_TEST_EXIT_HARD_ERROR;
}

#else /* !__ARM_FEATURE_SVE */
int
main (int argc, char *argv[])
{
  int verbose = (argc > 1);
  if (verbose)
    fprintf (stdout, "SVE is not enabled: skip\n");

  return UNW_TEST_EXIT_SKIP;
}
#endif
