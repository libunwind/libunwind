/* libunwind - a platform-independent unwind library

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
NONINFRINGEMENT.  IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.  */

/* Verify that unw_get_proc_name() resolves frames that live in an executable
   PT_LOAD segment other than the first one.  Such objects are produced, for
   example, by llvm-bolt, which appends a second text segment to the binary.

   The test is linked so that the functions placed in the ".second_text"
   section end up far away from ".text", in a PT_LOAD segment of their own.  */

#define UNW_LOCAL_ONLY
#define _GNU_SOURCE
#include <libunwind.h>
#include "compiler.h"

#include <link.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SKIP 77
#define SECOND_TEXT __attribute__((section(".second_text")))

int verbose;
static int nerrors;

static int
count_exec_segments (struct dl_phdr_info *info, size_t size UNUSED, void *data)
{
  int *count = data;
  int i;

  /* The first callback is for the main program.  */
  for (i = 0; i < info->dlpi_phnum; ++i)
    if (info->dlpi_phdr[i].p_type == PT_LOAD
        && (info->dlpi_phdr[i].p_flags & PF_X))
      ++*count;
  return 1;
}

static void
check_frame (unw_cursor_t *cursor, const char *expected)
{
  char name[128];
  unw_word_t ip = 0, off = 0;
  int ret;

  unw_get_reg (cursor, UNW_REG_IP, &ip);
  ret = unw_get_proc_name (cursor, name, sizeof (name), &off);
  if (verbose)
    printf ("ip=%#lx expected=%s got=%s (ret=%d)\n", (unsigned long) ip,
            expected, ret == 0 ? name : "<none>", ret);
  if (ret != 0 || strcmp (name, expected) != 0)
    {
      fprintf (stderr, "FAILURE: ip %#lx: expected \"%s\", got \"%s\" (ret=%d)\n",
               (unsigned long) ip, expected, ret == 0 ? name : "<none>", ret);
      ++nerrors;
    }
}

int second_segment_leaf (void);
int second_segment_caller (void);

/* Both of these live in the second executable segment.  */
SECOND_TEXT NOINLINE int
second_segment_leaf (void)
{
  unw_context_t uc;
  unw_cursor_t cursor;

  unw_getcontext (&uc);
  if (unw_init_local (&cursor, &uc) < 0)
    {
      fprintf (stderr, "FAILURE: unw_init_local failed\n");
      return ++nerrors;
    }

  check_frame (&cursor, "second_segment_leaf");
  if (unw_step (&cursor) <= 0)
    {
      fprintf (stderr, "FAILURE: unw_step failed\n");
      return ++nerrors;
    }
  check_frame (&cursor, "second_segment_caller");
  if (unw_step (&cursor) <= 0)
    {
      fprintf (stderr, "FAILURE: unw_step failed\n");
      return ++nerrors;
    }
  /* ... and its caller lives in the first one.  */
  check_frame (&cursor, "main");
  return nerrors;
}

SECOND_TEXT NOINLINE int
second_segment_caller (void)
{
  return second_segment_leaf () + 1;
}

int
main (int argc, char **argv UNUSED)
{
  int exec_segments = 0;

  if (argc > 1)
    verbose = 1;

  dl_iterate_phdr (count_exec_segments, &exec_segments);
  if (exec_segments < 2)
    {
      printf ("SKIP: linker did not produce a second executable segment\n");
      return SKIP;
    }

  second_segment_caller ();

  if (nerrors > 0)
    {
      fprintf (stderr, "FAILURE: detected %d errors\n", nerrors);
      exit (-1);
    }
  if (verbose)
    printf ("SUCCESS.\n");
  return 0;
}
