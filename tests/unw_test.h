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

/**
 * Exit values for test programs.
 * Based on https://www.gnu.org/software/automake/manual/html_node/Scripts_002dbased-Testsuites.html
 *
 * These are used to interact with the test harness (eg. a TAP-based harness,
 * CTest, or automake).
 */
enum {
  UNW_TEST_EXIT_PASS        =  0,  /* Item under test is a PASS */
  UNW_TEST_EXIT_FAIL        =  1,  /* Item under test is a FAIL */
  UNW_TEST_EXIT_BAD_COMMAND =  2,  /* Test program is invoked with invalid arguments */
  UNW_TEST_EXIT_SKIP        = 77,  /* Test should be skipped */
  UNW_TEST_EXIT_HARD_ERROR  = 99   /* Test program itself has failed */
};

#endif /* LIBUNWIND_UNW_TEST_H */

