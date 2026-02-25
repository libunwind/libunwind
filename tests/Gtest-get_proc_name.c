/**
 * @file tests/Gtest-get_proc_name
 * @brief Unit tests for unw_get_proc_name()
 */
/*
 * This file is part of libunwind.
 *   Copyright 2025 Stephen M. Webb <stephen.webb@bregmasoft.ca>
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

#include <stdbool.h>
#include <string.h>

#include "compiler.h"
#include "libunwind.h"


static bool is_verbose_mode = false;
static const size_t buffer_size = 256;


static int
trace_back_1 (char const* const fname, size_t fname_len)
{
  char sym[buffer_size];
  UNW_TEST_ASSERT(fname_len < buffer_size, "bad symbol len\n");
  if (is_verbose_mode)
    {
      printf("looking for '%s' length %zu\n", fname, fname_len);
    }

  unw_context_t context;
  unw_getcontext(&context);

  unw_cursor_t cursor;
  unw_init_local(&cursor, &context);
  while (unw_step(&cursor) > 0)
    {
      unw_word_t ip;
      unw_get_reg(&cursor, UNW_REG_IP, &ip);
      if (ip == 0)
        {
          break;
        }
      if (is_verbose_mode)
        {
          printf("%#010lx:", (long)ip);
        }

      unw_word_t offset;
      int ret = unw_get_proc_name(&cursor, sym, fname_len, &offset);
      if (ret == UNW_ESUCCESS)
        {
          if (is_verbose_mode)
            {
              printf(" (%s+%#010lx)\n", sym, (long)offset);
            }
          UNW_TEST_ASSERT(strcmp(sym, fname) == 0,
                          "expected symbol '%s' and retrieved symbol '%s' do not match",
                          fname, sym);
        }
      else
        {
          if (is_verbose_mode)
            {
              printf(" ret=%d\n", ret);
            }
        }
      return ret;
    }
  return -UNW_ENOINFO;
}

static void NOINLINE
test_function_name_longer_than_buffer ()
{
  UNW_TEST_ASSERT(trace_back_1 (__FUNCTION__, sizeof(__FUNCTION__)-1) == -UNW_ENOMEM,
                  "unexpected return value for function name longer than buffer\n");
}

static void NOINLINE
test_function_name_shorter_than_buffer ()
{
  UNW_TEST_ASSERT(trace_back_1 (__FUNCTION__, sizeof(__FUNCTION__)+1) == UNW_ESUCCESS,
                  "unexpected return value for function name shorter than buffer\n");
}


int
main (int argc, char **argv)
{
  is_verbose_mode = (argc > 1);

  test_function_name_shorter_than_buffer ();
  test_function_name_longer_than_buffer ();

  return UNW_TEST_EXIT_PASS;
}
