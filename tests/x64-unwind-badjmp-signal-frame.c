/**
 * @file tests/x64-unwind-badjmp-signal-frame.c
 * @brief Test functionality of handling the signal raised after an invalid
 * function call on x86_64.
 */
/*
 * This file is part of libunwind - a platform-independent unwind library.
 *   Copyright (C) 2019 Brock York <twunknown AT gmail.com>
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
#include "unw_test.h"

#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#define UNW_LOCAL_ONLY
#include <libunwind.h>
#include "compiler.h"

#if defined(__QNX__)
# define SIGNAL_TRAMPOLINE "__signalstub"
#else
# define SIGNAL_TRAMPOLINE ""
#endif


static char const *names[] =
  {
    "handle_sigsegv",  /* signal handler */
    SIGNAL_TRAMPOLINE, /* signal trampoline */
#if defined(__FreeBSD__)
	"",                /* extra trampoline function 1 in FreeBSD */
	"",                /* extra trampoline function 2 in FreeBSD */
#endif
	"",                /* bad function */
    "main",
  };
static int const names_count = sizeof(names) / sizeof(*names);

static bool verbose = false;

/*
 * unwind in the signal handler checking the backtrace is correct
 * after a bad jump.
 */
void handle_sigsegv(int signal UNUSED, siginfo_t *info UNUSED, void *ucontext UNUSED)
{

  unw_context_t uc;
  int ret = unw_getcontext(&uc);
  UNW_TEST_ASSERT(ret == UNW_ESUCCESS, "failure in unw_getcontext()\n");

  unw_cursor_t cursor;
  ret = unw_init_local(&cursor, &uc);
  UNW_TEST_ASSERT(ret == UNW_ESUCCESS, "failure in unw_init_local()\n");

  int i;
  for (i = 0; i < names_count; ++i)
    {
      bool is_signal_frame = unw_is_signal_frame(&cursor);

      char name[1000];
      memset(name, 0, sizeof(char) * 1000);
      unw_word_t offset;
      ret = unw_get_proc_name(&cursor, name, sizeof(char) * 1000, &offset);
      if (verbose)
        {
          if (ret != UNW_ESUCCESS)
          {
            printf("frame %d: [unw_get_proc_name() returned %d]\n", i, ret);
          }

          unw_word_t ip;
          ret = unw_get_reg(&cursor, UNW_REG_IP, &ip);
          UNW_TEST_ASSERT(ret == UNW_ESUCCESS || ip == 0,
                          "unw_get_reg(UNW_REG_IP) returned %d\n", ret);

          unw_word_t sp;
          ret = unw_get_reg(&cursor, UNW_REG_SP, &sp);
          UNW_TEST_ASSERT(ret == UNW_ESUCCESS,
                          "unw_get_reg(UNW_REG_SP) returned %d\n", ret);

          printf("frame %d: ip = %#010lx, sp = %#010lx offset = %#010lx name = '%s'%s%s\n",
                 i, (long) ip, (long) sp, (long) offset, name,
                 ((long)ip == 0x00000001L) ? " [is bad function pointer]" : "",
                 is_signal_frame ? " [is signal frame]" : "");
        }

      /* The main test assertion: the stack frame is the one we expect. */
      UNW_TEST_ASSERT(strcmp(names[i], name) == 0, "frame %d: expected '%s' found '%s'\n",
                      i, names[i], name);

      if (strcmp(name, "main") == 0)
        break;

      ret = unw_step(&cursor);
      UNW_TEST_ASSERT(ret >= UNW_ESUCCESS, "unw_step() returned %d\n", ret);
      if (ret == UNW_ESUCCESS)
        break;
    }

  /* Convert ordinal i into cardinal i */
  i = i + 1;
  UNW_TEST_ASSERT(i == names_count, "Expected %d frames, found %d\n",
                  names_count, i);

  exit(UNW_TEST_EXIT_PASS);
}

void (*invalid_function)() = (void*)1;

int main(int argc, char *argv[] UNUSED)
{
  verbose = (argc > 1);

  struct sigaction sa =
    {
      .sa_sigaction = handle_sigsegv,
      .sa_flags     = SA_SIGINFO
    };
  int ret = sigaction(SIGSEGV, &sa, NULL);
  UNW_TEST_ASSERT(ret == 0, "error %d in sigaction(SIGSEGV): %s\n", errno, strerror(errno));

  invalid_function();

  /*
   * If we dont end up in the signal handler something went horribly wrong.
   */
  return UNW_TEST_EXIT_HARD_ERROR;
}
