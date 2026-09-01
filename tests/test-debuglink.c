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

/* Check that a procedure name is resolved through a separate debug file
   located via a .gnu_debuglink section.

   The run-debuglink script strips this program and gives it a debug link
   naming a file with the same basename as the program itself, which is the
   first location libunwind looks in.  Only the CRC-32 recorded in the
   .gnu_debuglink section distinguishes the stripped program from the real
   debug file, so a failure here means the checksum is not being verified.  */

#define UNW_LOCAL_ONLY
#include <libunwind.h>

#include "compiler.h"

#include <stdio.h>
#include <string.h>

/* Deliberately static: the name is in the symbol table of the separate debug
   file only, never in the dynamic symbol table of the stripped program.  */
static int NOINLINE
debuglink_find_caller (void)
{
  unw_cursor_t cursor;
  unw_context_t uc;
  char name[256];
  unw_word_t off;
  int ret;

  unw_getcontext (&uc);
  if (unw_init_local (&cursor, &uc) < 0)
    {
      fprintf (stderr, "FAILURE: unw_init_local failed\n");
      return -1;
    }

  if (unw_step (&cursor) <= 0)
    {
      fprintf (stderr, "FAILURE: unw_step failed\n");
      return -1;
    }

  ret = unw_get_proc_name (&cursor, name, sizeof (name), &off);
  if (ret < 0)
    {
      fprintf (stderr, "FAILURE: unw_get_proc_name failed: %d\n", ret);
      return -1;
    }

  /* A prefix match: the compiler may append a suffix such as ".isra.0" to
     the name of a local function it has cloned.  */
  if (strncmp (name, "debuglink_caller", sizeof ("debuglink_caller") - 1) != 0)
    {
      fprintf (stderr, "FAILURE: expected \"debuglink_caller\", got \"%s\"\n",
               name);
      return -1;
    }

  return 0;
}

static int NOINLINE
debuglink_caller (void)
{
  /* Volatile keeps this frame from being tail-call optimized away.  */
  volatile int ret = debuglink_find_caller ();

  return ret;
}

int
main (void)
{
  return debuglink_caller () == 0 ? 0 : -1;
}
