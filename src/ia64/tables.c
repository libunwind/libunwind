/* libunwind - a platform-independent unwind library
   Copyright (C) 2001-2002 Hewlett-Packard Co
	Contributed by David Mosberger-Tang <davidm@hpl.hp.com>

This file is part of libunwind.  */

#include <assert.h>
#include <stdlib.h>

#include "unwind_i.h"

struct ia64_table_entry
  {
    uint64_t start_offset;
    uint64_t end_offset;
    uint64_t info_offset;
  };

static inline const struct ia64_table_entry *
lookup (struct ia64_table_entry *table, size_t table_size, unw_word_t rel_ip)
{
  const struct ia64_table_entry *e = 0;
  unsigned long lo, hi, mid;

  /* do a binary search for right entry: */
  for (lo = 0, hi = table_size / sizeof (struct ia64_table_entry); lo < hi;)
    {
      mid = (lo + hi) / 2;
      e = table + mid;
      if (rel_ip < e->start_offset)
	hi = mid;
      else if (rel_ip >= e->end_offset)
	lo = mid + 1;
      else
	break;
    }
  if (rel_ip < e->start_offset || rel_ip >= e->end_offset)
    return NULL;
  return e;
}

static inline int
is_local_addr_space (unw_addr_space_t as)
{
  extern unw_addr_space_t _ULia64_local_addr_space;

  return (as == _Uia64_local_addr_space
#ifndef UNW_REMOTE_ONLY
	  || as == _ULia64_local_addr_space
#endif
	  );
}

HIDDEN int
tdep_search_unwind_table (unw_addr_space_t as, unw_word_t ip,
			  unw_dyn_info_t *di,
			  unw_proc_info_t *pi, int need_unwind_info, void *arg)
{
  unw_word_t addr, hdr_addr, info_addr, info_end_addr, hdr, *wp;
  unw_word_t handler_offset;
  const struct ia64_table_entry *e;
  unw_accessors_t *a = unw_get_accessors (as);
  int ret;

  assert (di->format == UNW_INFO_FORMAT_TABLE
	  && (ip >= di->start_ip && ip < di->end_ip));

  e = lookup ((struct ia64_table_entry *) di->u.ti.table_data,
	      di->u.ti.table_len * sizeof (unw_word_t), ip - di->u.ti.segbase);
  if (!e)
    {
      /* IP is inside this table's range, but there is no explicit
	 unwind info => use default conventions (i.e., this is NOT an
	 error).  */
      memset (pi, 0, sizeof (*pi));
      return 0;
    }

  pi->start_ip = e->start_offset + di->u.ti.segbase;
  pi->end_ip = e->end_offset + di->u.ti.segbase;

  hdr_addr = e->info_offset + di->u.ti.segbase;
  info_addr = hdr_addr + 8;

  /* read the header word: */
  ret = (*a->access_mem) (as, hdr_addr, &hdr, 0, arg);
  if (ret < 0)
    return ret;

  if (IA64_UNW_VER (hdr) != 1)
    return -UNW_EBADVERSION;

  info_end_addr = info_addr + 8 * IA64_UNW_LENGTH (hdr);

  pi->unwind_info = 0;
  if (need_unwind_info)
    {
      pi->unwind_info_size = 8 * IA64_UNW_LENGTH (hdr);

      if (is_local_addr_space (as))
	pi->unwind_info = (void *) info_addr;
      else
	{
	  /* Internalize unwind info.  Note: since we're doing this
	     only for non-local address spaces, there is no
	     signal-safety issue and it is OK to use malloc()/free().  */
	  pi->unwind_info = malloc (8 * IA64_UNW_LENGTH (hdr));
	  if (!pi->unwind_info)
	    return -UNW_ENOMEM;

	  wp = (unw_word_t *) pi->unwind_info;
	  for (addr = info_addr; addr < info_end_addr; addr += 8, ++wp)
	    {
	      ret = (*a->access_mem) (as, addr, wp, 0, arg);
	      if (ret < 0)
		{
		  free (pi->unwind_info);
		  return ret;
		}
	    }
	}
    }

  pi->handler = 0;
  if (IA64_UNW_FLAG_EHANDLER (hdr) || IA64_UNW_FLAG_UHANDLER (hdr))
    {
      /* read the personality routine address (address is gp-relative): */
      ret = (*a->access_mem) (as, info_end_addr + 8, &handler_offset, 0, arg);
      if (ret < 0)
	return ret;
      pi->handler = handler_offset + di->gp;
    }
  pi->lsda = info_end_addr + 16;
  pi->gp = di->gp;
  pi->flags = 0;
  pi->format = di->format;
  return 0;
}

HIDDEN void
tdep_put_unwind_info (unw_addr_space_t as, unw_proc_info_t *pi, void *arg)
{
  if (!pi->unwind_info)
    return;

  if (!is_local_addr_space (as))
    {
      free (pi->unwind_info);
      pi->unwind_info = NULL;
    }
}

#ifndef UNW_REMOTE_ONLY

#include <link.h>
#include <stddef.h>
#include <stdlib.h>

#if __GLIBC__ < 2 || (__GLIBC__ == 2 && __GLIBC_MINOR__ < 2) \
    || (__GLIBC__ == 2 && __GLIBC_MINOR__ == 2 && !defined(DT_CONFIG))
# error You need GLIBC 2.2.4 or later on IA-64 Linux
#endif

#ifdef HAVE_GETUNWIND

extern unsigned long getunwind (void *buf, size_t len);

#else /* HAVE_GETUNWIND */

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

#endif /* HAVE_GETUNWIND */

static unw_dyn_info_t kernel_table;

static int
get_kernel_table (void *ptr)
{
  struct ia64_table_entry *ktab, *etab;
  unw_dyn_info_t *di = ptr;
  size_t size;

  debug (100, "unwind: checking kernel unwind table");

  size = getunwind (NULL, 0);
  ktab = sos_alloc (size);
  if (!ktab)
    {
      dprintf (__FILE__".%s: failed to allocate %Zu bytes",
	       __FUNCTION__, size);
      return -UNW_ENOMEM;
    }
  getunwind (ktab, size);

  /* Determine length of kernel's unwind table & relocate its entries.  */
  for (etab = ktab; etab->start_offset; ++etab)
    etab->info_offset += (uint64_t) ktab;

  di->format = UNW_INFO_FORMAT_TABLE;
  di->gp = 0;
  di->start_ip = ktab[0].start_offset;
  di->end_ip = etab[-1].end_offset;
  di->u.ti.name_ptr = (unw_word_t) "<kernel>";
  di->u.ti.segbase = 0;
  di->u.ti.table_len = ((char *) etab - (char *) ktab) / sizeof (unw_word_t);
  di->u.ti.table_data = (unw_word_t *) ktab;

  debug (100, "unwind: found table `%s': segbase=%lx, len=%lu, gp=%lx\n",
	 (char *) di->u.ti.name_ptr, di->u.ti.segbase, di->u.ti.table_len,
	 di->gp);
  return 0;
}

static int
callback (struct dl_phdr_info *info, size_t size, void *ptr)
{
  unw_dyn_info_t *di = ptr;
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
	  if (di->u.ti.segbase >= vaddr
	      && di->u.ti.segbase < vaddr + phdr->p_memsz)
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
	    di->gp = dyn->d_un.d_ptr;
	    break;
	  }
    }
  else
    {
      /* Otherwise this is a static executable with no _DYNAMIC.
	 The gp is constant program-wide.  */
      register unsigned long gp __asm__("gp");
      di->gp = gp;
    }
  di->format = UNW_INFO_FORMAT_TABLE;
  di->start_ip = p_text->p_vaddr + load_base;
  di->end_ip = p_text->p_vaddr + load_base + p_text->p_memsz;
  di->u.ti.name_ptr = (unw_word_t) info->dlpi_name;
  di->u.ti.table_data = (void *) (p_unwind->p_vaddr + load_base);
  di->u.ti.table_len = p_unwind->p_memsz / sizeof (unw_word_t);
  di->u.ti.segbase = p_text->p_vaddr + load_base;

  debug (100, "unwind: found table `%s': segbase=%lx, len=%lu, gp=%lx, "
	 "table_data=%p\n", (char *) di->u.ti.name_ptr, di->u.ti.segbase,
	 di->u.ti.table_len, di->gp, di->u.ti.table_data);
  return 1;
}

HIDDEN int
_Uia64_find_proc_info (unw_addr_space_t as, unw_word_t ip,
		       unw_proc_info_t *pi, int need_unwind_info, void *arg)
{
  unw_dyn_info_t di, *dip = &di;
  int ret;

  di.u.ti.segbase = ip;	/* this is cheap... */

  if (dl_iterate_phdr (callback, &di) <= 0)
    {
      if (!kernel_table.u.ti.table_data)
	{
	  ret = get_kernel_table (&kernel_table);
	  if (ret < 0)
	    return ret;
	}
      if (ip < kernel_table.start_ip || ip >= kernel_table.end_ip)
	return -UNW_ENOINFO;
      dip = &kernel_table;
    }

  /* now search the table: */
  return tdep_search_unwind_table (as, ip, dip, pi, need_unwind_info, arg);
}

#endif /* !UNW_REMOTE_ONLY */
