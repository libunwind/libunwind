#ifndef _UPT_internal_h
#define _UPT_internal_h

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

#include <errno.h>
#include <libunwind.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>

#include <sys/ptrace.h>

#include "internal.h"

struct UPT_info
  {
    pid_t pid;		/* the process-id of the child we're unwinding */
    void *image;	/* mmap'ped image of the child's executable */
    size_t image_size;	/* size of the image */
    unw_dyn_info_t di_cache;
    unw_word_t dyn_info_list_addr;
    unsigned int checked_dyn_info_list_addr : 1;
#if UNW_TARGET_IA64
    unw_dyn_info_t ktab;
#endif
  };

struct map_iterator
  {
    FILE *fp;
  };

HIDDEN int _UPT_reg_offset[UNW_REG_LAST];

extern HIDDEN unw_dyn_info_t *_UPTi_find_unwind_table (struct UPT_info *ui,
						       unw_addr_space_t as,
						       char *path,
						       unw_word_t segbase,
						       unw_word_t mapoff);

static inline void
maps_init (struct map_iterator *mi, pid_t pid)
{
  char path[PATH_MAX];

  snprintf (path, sizeof (path), "/proc/%d/maps", pid);
  mi->fp = fopen (path, "r");
}

static inline int
maps_next (struct map_iterator *mi,
	   unsigned long *low, unsigned long *high, unsigned long *offset,
	   char *path)
{
  char line[256+PATH_MAX];

  if (!mi->fp)
    return 0;

  while (fgets (line, sizeof (line), mi->fp))
    {
      if (sscanf (line, "%lx-%lx %*4c %lx %*x:%*x %*d %s\n",
		  low, high, offset, path) == 4)
	return 1;
    }
  return 0;
}

static inline void
maps_close (struct map_iterator *mi)
{
  fclose (mi->fp);
  mi->fp = NULL;
}

#endif /* _UPT_internal_h */
