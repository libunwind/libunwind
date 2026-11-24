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

/* Unit test for AArch64 Linux signal frame detection and decoding.  */

#include "compiler.h"
#include "unw_test.h"

#include <stdio.h>

#if defined(__linux__)

#include <libunwind.h>
#include <signal.h>
#include <stdint.h>
#include <string.h>
#include <ucontext.h>
#include <unistd.h>

#define FPSIMD_MAGIC 0x46508001
#define ESR_MAGIC    0x45535201

/* Header of a record in the reserved area of the sigcontext.  */
struct record
  {
    uint32_t magic;
    uint32_t size;
  };

/* The sigreturn trampoline of the kernel.  */
static const uint32_t restorer[] = {
  0xd2801168, /* mov x8, #0x8b */
  0xd4000001, /* svc #0 */
};

/* Unrelated code that has every bit of the trampoline set.  */
static const uint32_t not_restorer[][2] = {
  {
    0xd2801fe8, /* mov x8, #0xff */
    0xd4000001, /* svc #0 */
  },
  {
    0xf2bffde9, /* movk x9, #0xffef, lsl #16 */
    0xfd40b909, /* ldr d9, [x8, #368] */
  },
};

/* What the kernel pushes on the stack for a signal.  */
static struct
  {
    siginfo_t info;
    ucontext_t uc;
  }
frame;

int verbose;

static void
init_cursor (unw_cursor_t *cursor, unw_context_t *uc, const uint32_t *ip)
{
  int ret;

  unw_getcontext (uc);
  uc->uc_mcontext.pc = (uintptr_t) ip;
  uc->uc_mcontext.sp = (uintptr_t) &frame;
  ret = unw_init_local (cursor, uc);
  UNW_TEST_ASSERT (ret == 0, "unw_init_local() returned %d\n", ret);
}

static void
test_is_signal_frame (void)
{
  unw_context_t uc;
  unw_cursor_t cursor;
  size_t i;
  int ret;

  init_cursor (&cursor, &uc, restorer);
  ret = unw_is_signal_frame (&cursor);
  UNW_TEST_ASSERT (ret > 0, "trampoline: unw_is_signal_frame() returned %d\n",
                   ret);

  for (i = 0; i < ARRAY_SIZE (not_restorer); ++i)
    {
      init_cursor (&cursor, &uc, not_restorer[i]);
      ret = unw_is_signal_frame (&cursor);
      UNW_TEST_ASSERT (ret == 0, "%08x %08x: unw_is_signal_frame() returned %d\n",
                       not_restorer[i][0], not_restorer[i][1], ret);
    }
}

/* Puts RECORDS one after the other in the signal frame, like the kernel
   does, and steps from the trampoline through that frame.  */
static int
step_with_records (const struct record *records, size_t count,
                   unw_word_t *ip)
{
  unsigned char *area = (unsigned char *) &frame.uc.uc_mcontext.__reserved;
  unw_context_t uc;
  unw_cursor_t cursor;
  size_t i, off = 0;
  int ret;

  memset (&frame, 0, sizeof (frame));
  frame.uc.uc_mcontext.pc = (uintptr_t) &test_is_signal_frame;
  frame.uc.uc_mcontext.sp = (uintptr_t) &frame;
  for (i = 0; i < count; ++i)
    {
      memcpy (area + off, &records[i], sizeof (records[i]));
      off += records[i].size;
    }

  init_cursor (&cursor, &uc, restorer);
  ret = unw_step (&cursor);
  if (ret > 0)
    unw_get_reg (&cursor, UNW_REG_IP, ip);
  return ret;
}

static void
on_alarm (int signum UNUSED)
{
  static const char msg[] = "unw_step() did not return\n";
  ssize_t n = write (STDERR_FILENO, msg, sizeof (msg) - 1);

  (void) n;
  _exit (UNW_TEST_EXIT_FAIL);
}

static void
test_records (void)
{
  static const struct record good[] = {
    { FPSIMD_MAGIC, 528 }, { ESR_MAGIC, 16 }, { 0, 0 },
  };
  static const struct record bad[] = {
    { 0x1b, 0 }, { ESR_MAGIC, 8 }, { FPSIMD_MAGIC, 520 },
  };
  unw_word_t ip = 0;
  size_t i;
  int ret;

  ret = step_with_records (good, ARRAY_SIZE (good), &ip);
  UNW_TEST_ASSERT (ret > 0, "valid records: unw_step() returned %d\n", ret);
  UNW_TEST_ASSERT (ip == (uintptr_t) &test_is_signal_frame,
                   "valid records: IP is %#lx\n", (unsigned long) ip);

  signal (SIGALRM, on_alarm);
  alarm (10);
  for (i = 0; i < ARRAY_SIZE (bad); ++i)
    {
      ret = step_with_records (&bad[i], 1, &ip);
      UNW_TEST_ASSERT (ret < 0, "record of size %u: unw_step() returned %d\n",
                       bad[i].size, ret);
    }
  alarm (0);
}

int
main (int argc, char **argv UNUSED)
{
  verbose = argc > 1;

  test_is_signal_frame ();
  test_records ();

  if (verbose)
    printf ("SUCCESS\n");
  return UNW_TEST_EXIT_PASS;
}

#else /* !__linux__ */

int
main (void)
{
  return UNW_TEST_EXIT_SKIP;
}

#endif /* __linux__ */
