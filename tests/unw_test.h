/**
 * @file tests/unw_test.h
 *
 * Common unit test API for libunwind.
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
#ifndef LIBUNWIND_UNW_TEST_H
#define LIBUNWIND_UNW_TEST_H 1

#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>

/**
 * Exit values for test programs.
 * Based on https://www.gnu.org/software/automake/manual/html_node/Scripts_002dbased-Testsuites.html
 *
 * These are used to interact with the test harness (eg. a TAP-based harness,
 * CTest, or automake).
 */
enum {
  UNW_TEST_EXIT_PASS        =  0,  /**< Item under test is a PASS */
  UNW_TEST_EXIT_FAIL        =  1,  /**< Item under test is a FAIL */
  UNW_TEST_EXIT_BAD_COMMAND =  2,  /**< Test program is invoked with invalid arguments */
  UNW_TEST_EXIT_SKIP        = 77,  /**< Test should be skipped */
  UNW_TEST_EXIT_HARD_ERROR  = 99   /**< Test program itself has failed */
};

/**
 * libunwind test assertion macro
 *
 * Use for testing a condition and printing out a useful error message when that
 * condition does not hold.
 *
 * Prints out the error message and exits the prohram with UNW_TEST_EXIT_FAIL.
 *
 * Example:
 * ```
 * UNW_TEST_ASSERT(retval > 0, "unw_step() failed at frame %d\n", frame_number);
 * ```
 */
#define UNW_TEST_ASSERT(cond, fmt, ...) cond \
    ? (void)0 \
    : _unw_test_fail_assertion(#cond, __FILE__, __LINE__, fmt __VA_OPT__(,) __VA_ARGS__)

/**
 * libunwind test check macro
 *
 * Use for testing a condition and printing out a useful error message when that
 * condition does not hold.
 *
 * Prints out the error message and continues.
 *
 * Example:
 * ```
 * UNW_TEST_CHECK(retval > 0, "unw_step() failed at frame %d\n", frame_number);
 * ```
 */
#define UNW_TEST_CHECK(cond, fmt, ...) cond \
    ? (void)0 \
    : _unw_test_fail_check(#cond, __FILE__, __LINE__, fmt __VA_OPT__(,) __VA_ARGS__)

/**
 * Maximum length of the formatted error string printed by UNW_TEST_ASSERT.
 */
#define UNW_TEST_MAX_ERRSTRING_LEN 1024

static inline void
_unw_test_fail_assertion(char const * const cond_str,
                         char const * const file,
                         int                line,
                         char const * const fmt, ...)
{
  char err_str[UNW_TEST_MAX_ERRSTRING_LEN];

  va_list args;
  va_start(args, fmt);
  vsnprintf(err_str, UNW_TEST_MAX_ERRSTRING_LEN, fmt, args);
  va_end(args);
  fprintf(stderr, "%s:%d ASSERTION FAIL '%s': %s\n", file, line, cond_str, err_str);
  exit(UNW_TEST_EXIT_FAIL);
}

static inline void
_unw_test_fail_check(char const * const cond_str,
                         char const * const file,
                         int                line,
                         char const * const fmt, ...)
{
  char err_str[UNW_TEST_MAX_ERRSTRING_LEN];

  va_list args;
  va_start(args, fmt);
  vsnprintf(err_str, UNW_TEST_MAX_ERRSTRING_LEN, fmt, args);
  va_end(args);
  fprintf(stderr, "%s:%d CHECK FAIL '%s': %s\n", file, line, cond_str, err_str);
}

#endif /* LIBUNWIND_UNW_TEST_H */

