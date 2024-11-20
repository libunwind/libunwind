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

#include <limits.h>
#include <stdio.h>
#include <sys/stat.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

#include "libunwind_i.h"
#include "os-linux.h"

#ifndef MAX_VDSO_SIZE
/*Dont know what is the Max Size in my system it was 8192 or twice the page size*/
# define MAX_VDSO_SIZE ((size_t) 2 * sysconf (_SC_PAGESIZE))
#endif

#ifndef MAP_32BIT
# define MAP_32BIT 0
#endif


int
tdep_get_elf_image (unw_addr_space_t as, struct elf_image *ei, pid_t pid, unw_word_t ip,
                    unsigned long *segbase, unsigned long *mapoff,
                    char *path, size_t pathlen,
                    void *arg)
{
  struct map_iterator mi;
  int found = 0, rc = UNW_ESUCCESS;
  unsigned long hi;
  char root[sizeof ("/proc/0123456789/root")], *cp;
  char *full_path;
  struct stat st;
  unw_accessors_t *a;
  unw_word_t magic;


  if (maps_init (&mi, pid) < 0)
    return -1;

  while (maps_next (&mi, segbase, &hi, mapoff, NULL))
    if (ip >= *segbase && ip < hi)
      {
        found = 1;
        break;
      }

  if (!found)
    {
      maps_close (&mi);
      return -1;
    }

  // get path only, no need to map elf image
  if (!ei && path)
    {
      strncpy(path, mi.path, pathlen);
      path[pathlen - 1] = '\0';
      if (strlen(mi.path) >= pathlen)
        rc = -UNW_ENOMEM;

      maps_close (&mi);
      return rc;
    }

  full_path = mi.path;

  /* Get process root */
  memcpy (root, "/proc/", 6);
  cp = unw_ltoa (root + 6, pid);
  assert (cp + 6 < root + sizeof (root));
  memcpy (cp, "/root", 6);

  size_t _len = strlen (mi.path) + 1;
  if (!stat(root, &st) && S_ISDIR(st.st_mode))
    _len += strlen (root);
  else
    root[0] = '\0';

  full_path = path;
  if(!path)
    full_path = (char*) malloc (_len);
  else if(_len >= pathlen) // passed buffer is too small, fail
    {
      maps_close (&mi);
      return -1;
    }

  strcpy (full_path, root);
  strcat (full_path, mi.path);

  if (stat(full_path, &st) || !S_ISREG(st.st_mode))
    strcpy(full_path, mi.path);

  rc = elf_map_image (ei, full_path);

  /*Follwing code is adapted from 
    https://sourceware.org/pipermail/frysk-cvs/2008q1/007245.html,
    For VDSO there is no file in the file system, so read the ELF , 
    and create mmaped file for the content of the VDSO 
  */
  if (rc != -1)
  {
    maps_close (&mi);
    goto err_exit;
  }

  /* If the above failed, try to bring in page-sized segments directly
     from process memory.  This enables us to locate VDSO unwind
     tables.  */
  ei->size = hi - *segbase;
  if (ei->size > MAX_VDSO_SIZE) 
  {
    maps_close (&mi);
    goto err_exit;
  }

  a = unw_get_accessors (as);
  if (! a->access_mem) 
  {
    maps_close (&mi);
    goto err_exit;
  }

  /* Try to decide whether it's an ELF image before bringing it all
     in.  */
  if (ei->size <= EI_CLASS || ei->size <= sizeof (magic))
  {
    maps_close (&mi);
    goto err_exit;
  }

  if (sizeof (magic) >= SELFMAG)
    {
      int ret = (*a->access_mem) (as, *segbase, &magic, 0, arg);
      if (ret < 0)
      {
        rc = ret;
        maps_close (&mi);
        goto err_exit;
      }

      if (memcmp (&magic, ELFMAG, SELFMAG) != 0)
      {
        maps_close (&mi);
        goto err_exit;
      }
    }

  ei->image = mmap (0, ei->size, PROT_READ | PROT_WRITE,
           MAP_PRIVATE | MAP_ANONYMOUS | MAP_32BIT, -1, 0);
  if (ei->image == MAP_FAILED)
    {
      maps_close (&mi);
      goto err_exit;
    }

  if (sizeof (magic) >= SELFMAG)
    {
      *(unw_word_t *)ei->image = magic;
      hi = sizeof (magic);
    }
  else
    hi = 0;

  for (; hi < ei->size; hi += sizeof (unw_word_t))
    {
      rc = (*a->access_mem) (as, *segbase + hi, ei->image + hi,
               0, arg);
      if (rc < 0)
   {
     munmap (ei->image, ei->size);
     maps_close (&mi);
     goto err_exit;
   }
    }

  if (*segbase == *mapoff
      && (*path == 0 || strcmp (path, "[vdso]") == 0))
    *mapoff = 0;

err_exit:

  if (!path)
    free (full_path);

  maps_close (&mi);
  return rc;
}

#ifndef UNW_REMOTE_ONLY

void
tdep_get_exe_image_path (char *path)
{
  strcpy(path, "/proc/self/exe");
}

#endif /* !UNW_REMOTE_ONLY */
