/* libunwind - a platform-independent unwind library
   Copyright (C) 2003 Hewlett-Packard Co
	Contributed by David Mosberger-Tang <davidm@hpl.hp.com>

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

#include "_UPT_internal.h"

int
_UPT_access_mem (unw_addr_space_t as, unw_word_t addr, unw_word_t *val,
		 int write, void *arg)
{
  struct UPT_info *ui = arg;
  pid_t pid = ui->pid;

  errno = 0;
  if (write)
    {
      debug (100, "%s: mem[%lx] <- %lx\n", __FUNCTION__, addr, *val);
      ptrace (PTRACE_POKEDATA, pid, addr, *val);
      if (errno)
	return -UNW_EINVAL;
    }
  else
    {
      *val = ptrace (PTRACE_PEEKDATA, pid, addr, 0);
      if (errno)
	return -UNW_EINVAL;
      debug (100, "%s: mem[%lx] -> %lx\n", __FUNCTION__, addr, *val);
    }
  return 0;
}
