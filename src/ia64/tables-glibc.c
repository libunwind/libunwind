/* libunwind - a platform-independent unwind library
   Copyright (C) 2001-2002 Hewlett-Packard Co
	Contributed by David Mosberger-Tang <davidm@hpl.hp.com>

This file is part of libunwind.

libunwind is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2, or (at your option)
any later version.

libunwind is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

As a special exception, if you link this library with other files to
produce an executable, this library does not by itself cause the
resulting executable to be covered by the GNU General Public License.
This exception does not however invalidate any other reasons why the
executable file might be covered by the GNU General Public
License.  */

/* This file is bsed on gcc/config/ia64/fde-glibc.c, which is copyright
   by:

    Copyright (C) 2000, 2001 Free Software Foundation, Inc.
	Contributed by Richard Henderson <rth@cygnus.com>.  */

#ifndef UNW_REMOTE_ONLY

#include <link.h>
#include <stddef.h>
#include <stdlib.h>

#include "unwind_i.h"

#if __GLIBC__ < 2 || (__GLIBC__ == 2 && __GLIBC_MINOR__ < 2) \
    || (__GLIBC__ == 2 && __GLIBC_MINOR__ == 2 && !defined(DT_CONFIG))
# error You need GLIBC 2.2.4 or later on IA-64 Linux
#endif

#if 0

extern unsigned long getunwind (void *buf, size_t len);

#else

/* XXX fix me */

#include <unistd.h>
#include <sys/syscall.h>

# ifndef __NR_getunwind
#  define __NR_getunwind	1215
# endif

static unsigned long
getunwind (void *buf, size_t len)
{
  return syscall (SYS_getunwind, buf, len);
}

#endif

static int
get_kernel_table (void *ptr)
{
  struct ia64_unwind_table_entry *ktab, *etab;
  unw_ia64_table_t *info = ptr;
  size_t size;

  debug (100, "unwind: checking kernel unwind table");

  size = getunwind (NULL, 0);
  ktab = malloc (size);
  if (!ktab)
    {
      dprintf (__FILE__".%s: failed to allocate %Zu bytes",
	       __FUNCTION__, size);
      return -UNW_ENOMEM;
    }
  getunwind (ktab, size);

  /* Determine length of kernel's unwind table.  */
  for (etab = ktab; etab->start_offset; ++etab);

  if (info->segbase < ktab[0].start_offset
      || info->segbase >= etab[-1].end_offset)
    {
      free (ktab);
      return -1;
    }

  info->name = "<kernel>";
  info->gp = 0;
  info->segbase = 0;
  info->start = ktab[0].start_offset;
  info->end = etab[-1].end_offset;
  info->length = etab - ktab;
  info->array = ktab;
  info->unwind_info_base = (const u_int8_t *) ktab;

  debug (100, "unwind: found table `%s': segbase=%lx, length=%lu, gp=%lx\n",
	 info->name, info->segbase, info->length, info->gp);

  return 0;
}

static int
callback (struct dl_phdr_info *info, size_t size, void *ptr)
{
  unw_ia64_table_t *data = ptr;
  const Elf64_Phdr *phdr, *p_unwind, *p_dynamic, *p_text;
  long n;
  Elf64_Addr load_base;

  /* Make sure struct dl_phdr_info is at least as big as we need.  */
  if (size < offsetof (struct dl_phdr_info, dlpi_phnum)
	     + sizeof (info->dlpi_phnum))
    return -1;

  debug (100, "unwind: checking `%s'\n", info->dlpi_name);

  phdr = info->dlpi_phdr;
  load_base = info->dlpi_addr;
  p_text = NULL;
  p_unwind = NULL;
  p_dynamic = NULL;

  /* See if PC falls into one of the loaded segments.  Find the unwind
     segment at the same time.  */
  for (n = info->dlpi_phnum; --n >= 0; phdr++)
    {
      if (phdr->p_type == PT_LOAD)
	{
	  Elf64_Addr vaddr = phdr->p_vaddr + load_base;
	  if (data->segbase >= vaddr && data->segbase < vaddr + phdr->p_memsz)
	    p_text = phdr;
	}
      else if (phdr->p_type == PT_IA_64_UNWIND)
	p_unwind = phdr;
      else if (phdr->p_type == PT_DYNAMIC)
	p_dynamic = phdr;
    }
  if (!p_text || !p_unwind)
    return 0;

  if (p_dynamic)
    {
      /* For dynamicly linked executables and shared libraries,
	 DT_PLTGOT is the gp value for that object.  */
      Elf64_Dyn *dyn = (Elf64_Dyn *)(p_dynamic->p_vaddr + load_base);
      for (; dyn->d_tag != DT_NULL ; dyn++)
	if (dyn->d_tag == DT_PLTGOT)
	  {
	    /* On IA-64, _DYNAMIC is writable and GLIBC has relocated it.  */
	    data->gp = dyn->d_un.d_ptr;
	    break;
	  }
    }
  else
    {
      /* Otherwise this is a static executable with no _DYNAMIC.
	 The gp is constant program-wide.  */
      register unsigned long gp __asm__("gp");
      data->gp = gp;
    }
  data->name = info->dlpi_name;
  data->array
    = (struct ia64_unwind_table_entry *) (p_unwind->p_vaddr + load_base);
  data->length = p_unwind->p_memsz / sizeof (struct ia64_unwind_table_entry);
  data->segbase = p_text->p_vaddr + load_base;
  data->start = p_text->p_vaddr + load_base;
  data->end = p_text->p_vaddr + load_base + p_text->p_memsz;
  data->unwind_info_base = (const u_int8_t *) data->segbase;

  debug (100, "unwind: found table `%s': segbase=%lx, length=%lu, gp=%lx, "
	 "array=%p\n", data->name, data->segbase, data->length, data->gp,
	 data->array);

  return 1;
}

int
_Uia64_glibc_acquire_unwind_info (unw_word_t ip, void *info, void *arg)
{
  ((unw_ia64_table_t *) info)->segbase = ip;	/* this is cheap... */

  if (dl_iterate_phdr (callback, info) >= 0)
    return 0;

  return get_kernel_table (info);
}

int
_Uia64_glibc_release_unwind_info (void *info, void *arg)
{
  /* nothing to do */
  return 0;
}

#endif /* !UNW_REMOTE_ONLY */
