/* libunwind - a platform-independent unwind library
   Copyright (C) 2003-2005 Hewlett-Packard Co
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

#ifndef UNW_REMOTE_ONLY

#include <dlfcn.h>
#include <unistd.h>

#include "libunwind_i.h"

#include "elf64.h"

HIDDEN int
tdep_get_elf_image (struct elf_image *ei, pid_t pid, unw_word_t ip,
		    unsigned long *segbase, unsigned long *mapoff)
{
  struct load_module_desc lmd;
  const char *path;

  if (pid != getpid ())
    {
      printf ("%s: remote case not implemented yet\n", __FUNCTION__);
      return -UNW_ENOINFO;
    }

  if (!dlmodinfo (ip, &lmd, sizeof (lmd), NULL, 0, 0))
    return -UNW_ENOINFO;

  *segbase = lmd.text_base;
  *mapoff = 0;			/* XXX fix me? */

  path = dlgetname (&lmd, sizeof (lmd), NULL, 0, 0);
  if (!path)
    return -UNW_ENOINFO;

  Debug(1, "segbase=%lx, mapoff=%lx, path=%s\n", *segbase, *mapoff, path);

  return elf_map_image (ei, path);
}

#endif /* UNW_REMOTE_ONLY */
