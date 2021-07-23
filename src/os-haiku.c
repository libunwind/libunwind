/* libunwind - a platform-independent unwind library
   Copyright (C) 2010 Konstantin Belousov <kib@freebsd.org>

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

#include <kernel/image.h>
#include <kernel/OS.h>
#include <runtime_loader.h>

#include <sys/param.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <stdio.h>
#include <errno.h>

#include "libunwind_i.h"

static void *
get_mem(size_t sz)
{
  void *res;

  res = mmap(NULL, sz, PROT_READ | PROT_WRITE, MAP_ANON | MAP_PRIVATE, -1, 0);
  if (res == MAP_FAILED)
    return (NULL);
  return (res);
}

static void
free_mem(void *ptr, size_t sz)
{
  munmap(ptr, sz);
}

static int
get_pid_by_tid(int tid)
{
  // might be a kernel interface for this
  return -1;
}

int
tdep_get_elf_image (struct elf_image *ei, pid_t pid, unw_word_t ip,
                    unsigned long *segbase, unsigned long *mapoff, char *path, size_t pathlen)
{
  int32 cookie;
  char *buf;
  size_t len;
  
  team_info teamInfo;
  area_info areaInfo;
  
  if (get_team_info(B_CURRENT_TEAM, &teamInfo) != B_OK)
    return (-UNW_EUNSPEC);

  while (get_next_area_info(teamInfo.team, &cookie, &areaInfo) == B_OK) {
  	buf = get_mem(areaInfo.size);
  	if (buf == NULL)
  	  return (-UNW_EUNSPEC);
  }
  
  return -UNW_EUNSPEC;
  #if 0
  for (bp = buf, eb = buf + len; bp < eb; bp += kv->kve_structsize) {
     kv = (struct kinfo_vmentry *)(uintptr_t)bp;
     if (ip < kv->kve_start || ip >= kv->kve_end)
       continue;
     if (kv->kve_type != KVME_TYPE_VNODE)
       continue;
     *segbase = kv->kve_start;
     *mapoff = kv->kve_offset;
     if (path)
       {
         strncpy(path, kv->kve_path, pathlen);
       }
     ret = elf_map_image (ei, kv->kve_path);
     break;
  }
  free_mem(buf, len1);
  return (ret);
  #endif
}

#ifndef UNW_REMOTE_ONLY

void
tdep_get_exe_image_path (char *path)
{
  image_id image;

  if (__gRuntimeLoader->get_nearest_symbol_at_address(B_CURRENT_IMAGE_SYMBOL,
      &image, &path, NULL, NULL, NULL, NULL, NULL) != B_OK) {
	  path[0] = 0;
  }
}

#endif
