/* libunwind - a platform-independent unwind library
   Copyright (c) 2003 Hewlett-Packard Development Company, L.P.
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

/* Locate an FDE via the ELF data-structures defined by LSB v1.3
   (http://www.linuxbase.org/spec/).  */

#include <link.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>

#include "dwarf-eh.h"
#include "tdep.h"

#ifndef UNW_REMOTE_ONLY

/* Info is a pointer to a unw_dyn_info_t structure and, on entry,
   member u.rti.segbase contains the instruction-pointer we're looking
   for.  */
static int
callback (struct dl_phdr_info *info, size_t size, void *ptr)
{
  unw_dyn_info_t *di = ptr;
  const Elf_W(Phdr) *phdr, *p_eh_hdr, *p_dynamic, *p_text;
  unw_word_t addr, eh_frame_ptr, fde_count;
  Elf_W(Addr) load_base, segbase = 0;
  struct dwarf_eh_frame_hdr *hdr;
  unw_proc_info_t pi;
  unw_accessors_t *a;
  int ret;
  long n;

  /* Make sure struct dl_phdr_info is at least as big as we need.  */
  if (size < offsetof (struct dl_phdr_info, dlpi_phnum)
	     + sizeof (info->dlpi_phnum))
    return -1;

  Debug (16, "checking %s, base=0x%lx)\n",
	 info->dlpi_name, (long) info->dlpi_addr);

  phdr = info->dlpi_phdr;
  load_base = info->dlpi_addr;
  p_text = NULL;
  p_eh_hdr = NULL;
  p_dynamic = NULL;

  /* See if PC falls into one of the loaded segments.  Find the
     eh-header segment at the same time.  */
  for (n = info->dlpi_phnum; --n >= 0; phdr++)
    {
      if (phdr->p_type == PT_LOAD)
	{
	  Elf_W(Addr) vaddr = phdr->p_vaddr + load_base;
	  if (di->u.rti.segbase >= vaddr
	      && di->u.rti.segbase < vaddr + phdr->p_memsz)
	    p_text = phdr;
	}
      else if (phdr->p_type == PT_GNU_EH_FRAME)
	p_eh_hdr = phdr;
      else if (phdr->p_type == PT_DYNAMIC)
	p_dynamic = phdr;
    }
  if (!p_text || !p_eh_hdr)
    return 0;

  if (likely (p_eh_hdr->p_vaddr >= p_text->p_vaddr
	      && p_eh_hdr->p_vaddr < p_text->p_vaddr + p_text->p_memsz))
    /* normal case: eh-hdr is inside text segment */
    segbase = p_text->p_vaddr + load_base;
  else
    {
      /* Special case: eh-hdr is in some other segment; this may
	 happen, e.g., for the Linux kernel's gate DSO, for
	 example.  */
      phdr = info->dlpi_phdr;
      for (n = info->dlpi_phnum; --n >= 0; phdr++)
	{
	  if (phdr->p_type == PT_LOAD && p_eh_hdr->p_vaddr >= phdr->p_vaddr
	      && p_eh_hdr->p_vaddr < phdr->p_vaddr + phdr->p_memsz)
	    {
	      segbase = phdr->p_vaddr + load_base;
	      break;
	    }
	}
    }

  if (p_dynamic)
    {
      /* For dynamicly linked executables and shared libraries,
	 DT_PLTGOT is the value that data-relative addresses are
	 relative to for that object.  We call this the "gp".  */
      Elf_W(Dyn) *dyn = (Elf_W(Dyn) *)(p_dynamic->p_vaddr + load_base);
      for (; dyn->d_tag != DT_NULL; ++dyn)
	if (dyn->d_tag == DT_PLTGOT)
	  {
	    /* Assume that _DYNAMIC is writable and GLIBC has
	       relocated it (true for x86 at least).  */
	    di->gp = dyn->d_un.d_ptr;
	    break;
	  }
    }
  else
    /* Otherwise this is a static executable with no _DYNAMIC.  Assume
       that data-relative addresses are relative to 0, i.e.,
       absolute.  */
    di->gp = 0;

  hdr = (struct dwarf_eh_frame_hdr *) (p_eh_hdr->p_vaddr + load_base);
  if (hdr->version != DW_EH_VERSION)
    {
      Debug (1, "table `%s' has unexpected version %d\n",
	     info->dlpi_name, hdr->version);
      return 0;
    }

  if (hdr->table_enc == DW_EH_PE_omit)
    {
      Debug (1, "table `%s' doesn't have a binary search table\n",
	     info->dlpi_name);
      return 0;
    }
  /* For now, only support binary-search tables which are
     data-relative and whose entries have the size of a pointer.  */
  if (!(hdr->table_enc == (DW_EH_PE_datarel | DW_EH_PE_ptr)
	|| ((sizeof (unw_word_t) == 4
	     && hdr->table_enc == (DW_EH_PE_datarel | DW_EH_PE_sdata4)))
	|| ((sizeof (unw_word_t) == 8
	     && hdr->table_enc != (DW_EH_PE_datarel | DW_EH_PE_sdata8)))))
    {
      Debug (1, "search table in `%s' has unexpected encoding 0x%x\n",
	     info->dlpi_name, hdr->table_enc);
      return 0;
    }

  addr = (unw_word_t) (hdr + 1);
  a = unw_get_accessors (unw_local_addr_space);
  pi.gp = di->gp;

  /* Read eh_frame_ptr: */
  if ((ret = dwarf_read_encoded_pointer (unw_local_addr_space, a,
					 &addr, hdr->eh_frame_ptr_enc, &pi,
					 &eh_frame_ptr, NULL)) < 0)
    return ret;

  /* Read fde_count: */
  if ((ret = dwarf_read_encoded_pointer (unw_local_addr_space, a,
					 &addr, hdr->fde_count_enc, &pi,
					 &fde_count, NULL)) < 0)
    return ret;

  di->format = UNW_INFO_FORMAT_REMOTE_TABLE;
  di->start_ip = p_text->p_vaddr + load_base;
  di->end_ip = p_text->p_vaddr + load_base + p_text->p_memsz;
  di->u.rti.name_ptr = (unw_word_t) info->dlpi_name;
  di->u.rti.table_data = addr;
  di->u.rti.table_len = fde_count + 2 * sizeof (unw_word_t);
  /* For the binary-search table in the eh_frame_hdr, data-relative
     means relative to the start of that section... */
  di->u.rti.segbase = (unw_word_t) hdr;

  Debug (15, "found table `%s': segbase=0x%lx, len=%lu, gp=0x%lx, "
	 "table_data=0x%lx\n", (char *) di->u.rti.name_ptr,
	 (long) di->u.rti.segbase, (long) di->u.rti.table_len,
	 (long) di->gp, (long) di->u.rti.table_data);
  return 1;
}

HIDDEN int
dwarf_find_proc_info (unw_addr_space_t as, unw_word_t ip,
		      unw_proc_info_t *pi, int need_unwind_info, void *arg)
{
  sigset_t saved_sigmask;
  unw_dyn_info_t di;
  int ret;

  Debug (14, "looking for IP=0x%lx\n", (long) ip);
  di.u.rti.segbase = ip;	/* this is cheap... */

  sigprocmask (SIG_SETMASK, &unwi_full_sigmask, &saved_sigmask);
  ret = dl_iterate_phdr (callback, &di);
  sigprocmask (SIG_SETMASK, &saved_sigmask, NULL);

  if (ret <= 0)
    {
      Debug (14, "IP=0x%lx not found\n", (long) ip);
      return -UNW_ENOINFO;
    }

  /* now search the table: */
  return dwarf_search_unwind_table (as, ip, &di, pi, need_unwind_info, arg);
}

struct table_entry
  {
    unw_word_t start_ip_offset;
    unw_word_t fde_offset;
  };

static inline const struct table_entry *
lookup (struct table_entry *table, size_t table_size, unw_word_t rel_ip)
{
  unsigned long table_len = table_size / sizeof (struct table_entry);
  const struct table_entry *e = 0;
  unsigned long lo, hi, mid;
  unw_word_t end = 0;

  /* do a binary search for right entry: */
  for (lo = 0, hi = table_len; lo < hi;)
    {
      mid = (lo + hi) / 2;
      e = table + mid;
      if (rel_ip < e->start_ip_offset)
	hi = mid;
      else
	{
	  if (mid + 1 >= table_len)
	    break;

	  end = table[mid + 1].start_ip_offset;

	  if (rel_ip >= end)
	    lo = mid + 1;
	  else
	    break;
	}
    }
  if (rel_ip < e->start_ip_offset || rel_ip >= end)
    return NULL;
  return e;
}

#endif /* !UNW_REMOTE_ONLY */

/* Helper macro for reading an table_entry from remote memory.  */
#define remote_read(addr, member)					   \
	(*a->access_mem) (as, (addr) + offsetof (struct table_entry,	   \
						 member), &member, 0, arg)

#ifndef UNW_LOCAL_ONLY

/* Lookup an unwind-table entry in remote memory.  Returns 1 if an
   entry is found, 0 if no entry is found, negative if an error
   occurred reading remote memory.  */
static int
remote_lookup (unw_addr_space_t as,
	       unw_word_t table, size_t table_size, unw_word_t rel_ip,
	       struct table_entry *e, void *arg)
{
  unsigned long table_len = table_size / sizeof (struct table_entry);
  unw_word_t e_addr = 0, start_ip_offset, fde_offset;
  unw_word_t start = ~(unw_word_t) 0, end = 0;
  unw_accessors_t *a = unw_get_accessors (as);
  unsigned long lo, hi, mid;
  int ret;

  /* do a binary search for right entry: */
  for (lo = 0, hi = table_len; lo < hi;)
    {
      mid = (lo + hi) / 2;
      e_addr = table + mid * sizeof (struct table_entry);
      if ((ret = remote_read (e_addr, start_ip_offset)) < 0)
	return ret;

      start = start_ip_offset;
      if (rel_ip < start)
	hi = mid;
      else
	{
	  if (mid + 1 >= table_len)
	    break;

	  if ((ret = remote_read (e_addr + sizeof (struct table_entry),
				  start_ip_offset)) < 0)
	    return ret;
	  end = start_ip_offset;

	  if (rel_ip >= end)
	    lo = mid + 1;
	  else
	    break;
	}
    }
  if (rel_ip < start || rel_ip >= end)
    return 0;
  e->start_ip_offset = start;

  if ((ret = remote_read (e_addr, fde_offset)) < 0)
    return ret;
  e->fde_offset = fde_offset;
  return 1;
}

#endif /* UNW_LOCAL_ONLY */

PROTECTED int
dwarf_search_unwind_table (unw_addr_space_t as, unw_word_t ip,
			   unw_dyn_info_t *di, unw_proc_info_t *pi,
			   int need_unwind_info, void *arg)
{
  const struct table_entry *e = NULL;
  unw_word_t segbase = 0, fde_addr;
  unw_accessors_t *a;
#ifndef UNW_LOCAL_ONLY
  struct table_entry ent;
  int ret;
#endif

  assert (di->format == UNW_INFO_FORMAT_REMOTE_TABLE
	  && (ip >= di->start_ip && ip < di->end_ip));

  a = unw_get_accessors (as);

  pi->flags = 0;
  pi->unwind_info = 0;
  pi->handler = 0;
  pi->gp = 0;
  memset (&pi->extra, 0, sizeof (pi->extra));

#ifndef UNW_REMOTE_ONLY
  if (as == unw_local_addr_space)
    {
      segbase = di->u.rti.segbase;
      e = lookup ((struct table_entry *) di->u.rti.table_data,
		  di->u.rti.table_len * sizeof (unw_word_t), ip - segbase);
    }
  else
#endif
    {
#ifndef UNW_LOCAL_ONLY
      segbase = di->u.rti.segbase;
      if ((ret = remote_lookup (as, di->u.rti.table_data,
				di->u.rti.table_len * sizeof (unw_word_t),
				ip - segbase, &ent, arg)) < 0)
	return ret;
      if (ret)
	e = &ent;
      else
	e = NULL;	/* no info found */
#endif
    }
  if (!e)
    {
      /* IP is inside this table's range, but there is no explicit
	 unwind info.  */
      return -UNW_ENOINFO;
    }
  Debug (16, "ip=0x%lx, start_ip=0x%lx\n",
	 (long) ip, (long) (e->start_ip_offset + segbase));
  fde_addr = e->fde_offset + segbase;
  return dwarf_parse_fde (as, a, &fde_addr, pi, &pi->extra.dwarf_info, arg);
}

HIDDEN void
dwarf_put_unwind_info (unw_addr_space_t as, unw_proc_info_t *pi, void *arg)
{
  return;	/* always a nop */
}
