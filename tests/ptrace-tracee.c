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

/* The default tracee for test-ptrace.

   It is built like the rest of the test suite so that the traced
   process always has the word size the library was configured for: a
   program taken from the host system does not, in a multilib build, and
   a process of a different word size cannot be unwound.

   It makes a few system calls and exits, giving test-ptrace something
   to unwind at each syscall stop without taking long about it.  */

#include <unistd.h>

static void
level3 (void)
{
  (void) getpid ();
}

static void
level2 (void)
{
  level3 ();
  (void) getppid ();
}

static void
level1 (void)
{
  level2 ();
  (void) getpid ();
}

int
main (void)
{
  int i;

  for (i = 0; i < 4; ++i)
    level1 ();

  return 0;
}
