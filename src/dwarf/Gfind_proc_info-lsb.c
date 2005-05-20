/* libunwind - a platform-independent unwind library
   Copyright (c) 2003-2005 Hewlett-Packard Development Company, L.P.
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
#include <stddef.h>

#include "dwarf_i.h"
#include "dwarf-eh.h"
#include "libunwind_i.h"

struct table_entry
  {
    int32_t start_ip_offset;
    int32_t fde_offset;
  };

#ifndef UNW_REMOTE_ONLY

struct callback_data
  {
    /* in: */
    unw_word_t ip;		/* instruction-pointer we're looking for */
    unw_proc_info_t *pi;	/* proc-info pointer */
    int need_unwind_info;
    /* out: */
    int single_fde;		/* did we find a single FDE? (vs. a table) */
    unw_dyn_info_t di;		/* table info (if single_fde is false) */
  };

static int
linear_search (unw_addr_space_t as, unw_word_t ip,
	       unw_word_t eh_frame_start, unw_word_t eh_frame_end,
	       unw_word_t fde_count,
	       unw_proc_info_t *pi, int need_unwind_info, void *arg)
{
  unw_accessors_t *a = unw_get_accessors (unw_local_addr_space);
  unw_word_t i = 0, fde_addr, addr = eh_frame_start;
  int ret;

  while (i++ < fde_count && addr < eh_frame_end)
    {
      fde_addr = addr;
      if ((ret = dwarf_extract_proc_info_from_fde (as, a, &addr, pi, 0, arg))
	  < 0)
	return ret;

      if (ip >= pi->start_ip && ip < pi->end_ip)
	{
	  if (!need_unwind_info)
	    return 1;
	  addr = fde_addr;
	  if ((ret = dwarf_extract_proc_info_from_fde (as, a, &addr, pi,
						       need_unwind_info, arg))
	      < 0)
	    return ret;
	  return 1;
	}
    }
  return -UNW_ENOINFO;
}

/* Info is a pointer to a unw_dyn_info_t structure and, on entry,
   member u.rti.segbase contains the instruction-pointer we're looking
   for.  */
static int
callback (struct dl_phdr_info *info, size_t size, void *ptr)
{
  struct callback_data *cb_data = ptr;
  unw_dyn_info_t *di = &cb_data->di;
  const Elf_W(Phdr) *phdr, *p_eh_hdr, *p_dynamic, *p_text;
  unw_word_t addr, eh_frame_start, eh_frame_end, fde_count, ip;
  Elf_W(Addr) load_base, segbase = 0, max_load_addr = 0;
  int ret, need_unwind_info = cb_data->need_unwind_info;
  unw_proc_info_t *pi = cb_data->pi;
  struct dwarf_eh_frame_hdr *hdr;
  unw_accessors_t *a;
  long n;

  ip = cb_data->ip;

  /* Make sure struct dl_phdr_info is at least as big as we need.  */
  if (size < offsetof (struct dl_phdr_info, dlpi_phnum)
	     + sizeof (info->dlpi_phnum))
    return -1;

  Debug (15, "checking %s, base=0x%lx)\n",
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

	  if (ip >= vaddr && ip < vaddr + phdr->p_memsz)
	    p_text = phdr;

	  if (vaddr + phdr->p_filesz > max_load_addr)
	    max_load_addr = vaddr + phdr->p_filesz;
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
  pi->gp = di->gp;

  hdr = (struct dwarf_eh_frame_hdr *) (p_eh_hdr->p_vaddr + load_base);
  if (hdr->version != DW_EH_VERSION)
    {
      Debug (1, "table `%s' has unexpected version %d\n",
	     info->dlpi_name, hdr->version);
      return 0;
    }

  a = unw_get_accessors (unw_local_addr_space);
  addr = (unw_word_t) (hdr + 1);

  /* (Optionally) read eh_frame_ptr: */
  if ((ret = dwarf_read_encoded_pointer (unw_local_addr_space, a,
					 &addr, hdr->eh_frame_ptr_enc, pi,
					 &eh_frame_start, NULL)) < 0)
    return ret;

  /* (Optionally) read fde_count: */
  if ((ret = dwarf_read_encoded_pointer (unw_local_addr_space, a,
					 &addr, hdr->fde_count_enc, pi,
					 &fde_count, NULL)) < 0)
    return ret;

  if (hdr->table_enc != (DW_EH_PE_datarel | DW_EH_PE_sdata4))
    {
      /* If there is no search table or it has an unsupported
	 encoding, fall back on linear search.  */
      if (hdr->table_enc == DW_EH_PE_omit)
	Debug (4, "table `%s' lacks search table; doing linear search\n",
	       info->dlpi_name);
      else
	Debug (4, "table `%s' has encoding 0x%x; doing linear search\n",
	       info->dlpi_name, hdr->table_enc);

      eh_frame_end = max_load_addr;	/* XXX can we do better? */

      if (hdr->fde_count_enc == DW_EH_PE_omit)
	fde_count = ~0UL;
      if (hdr->eh_frame_ptr_enc == DW_EH_PE_omit)
	abort ();

      cb_data->single_fde = 1;
      return linear_search (unw_local_addr_space, ip,
			    eh_frame_start, eh_frame_end, fde_count,
			    pi, need_unwind_info, NULL);
    }

  cb_data->single_fde = 0;
  di->format = UNW_INFO_FORMAT_REMOTE_TABLE;
  di->start_ip = p_text->p_vaddr + load_base;
  di->end_ip = p_text->p_vaddr + load_base + p_text->p_memsz;
  di->u.rti.name_ptr = (unw_word_t) info->dlpi_name;
  di->u.rti.table_data = addr;
  assert (sizeof (struct table_entry) % sizeof (unw_word_t) == 0);
  di->u.rti.table_len = (fde_count * sizeof (struct table_entry)
			 / sizeof (unw_word_t));
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
  struct callback_data cb_data;
  intrmask_t saved_mask;
  int ret;

  Debug (14, "looking for IP=0x%lx\n", (long) ip);

  cb_data.ip = ip;
  cb_data.pi = pi;
  cb_data.need_unwind_info = need_unwind_info;

  sigprocmask (SIG_SETMASK, &unwi_full_mask, &saved_mask);
  ret = dl_iterate_phdr (callback, &cb_data);
  sigprocmask (SIG_SETMASK, &saved_mask, NULL);

  if (ret <= 0)
    {
      Debug (14, "IP=0x%lx not found\n", (long) ip);
      return -UNW_ENOINFO;
    }

  if (cb_data.single_fde)
    /* already got the result in *pi */
    return 0;
  else
    /* search the table: */
    return dwarf_search_unwind_table (as, ip, &cb_data.di,
				      pi, need_unwind_info, arg);
}

static inline const struct table_entry *
lookup (struct table_entry *table, size_t table_size, int32_t rel_ip)
{
  unsigned long table_len = table_size / sizeof (struct table_entry);
  const struct table_entry *e = 0;
  unsigned long lo, hi, mid;

  /* do a binary search for right entry: */
  for (lo = 0, hi = table_len; lo < hi;)
    {
      mid = (lo + hi) / 2;
      e = table + mid;
      if (rel_ip < e->start_ip_offset)
	hi = mid;
      else
	lo = mid + 1;
    }
  if (hi <= 0)
	return NULL;
  e = table + hi - 1;
  return e;
}

#endif /* !UNW_REMOTE_ONLY */

#ifndef UNW_LOCAL_ONLY

/* Lookup an unwind-table entry in remote memory.  Returns 1 if an
   entry is found, 0 if no entry is found, negative if an error
   occurred reading remote memory.  */
static int
remote_lookup (unw_addr_space_t as,
	       unw_word_t table, size_t table_size, int32_t rel_ip,
	       struct table_entry *e, void *arg)
{
  unsigned long table_len = table_size / sizeof (struct table_entry);
  unw_accessors_t *a = unw_get_accessors (as);
  unsigned long lo, hi, mid;
  unw_word_t e_addr = 0;
  int32_t start;
  int ret;

  /* do a binary search for right entry: */
  for (lo = 0, hi = table_len; lo < hi;)
    {
      mid = (lo + hi) / 2;
      e_addr = table + mid * sizeof (struct table_entry);
      if ((ret = dwarf_reads32 (as, a, &e_addr, &start, arg)) < 0)
	return ret;

      if (rel_ip < start)
	hi = mid;
      else
	lo = mid + 1;
    }
  if (hi <= 0)
    return 0;
  e_addr = table + (hi - 1) * sizeof (struct table_entry);
  if ((ret = dwarf_reads32 (as, a, &e_addr, &e->start_ip_offset, arg)) < 0
   || (ret = dwarf_reads32 (as, a, &e_addr, &e->fde_offset, arg)) < 0)
    return ret;
  return 1;
}

#endif /* !UNW_LOCAL_ONLY */

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
#endif
  int ret;

  assert (di->format == UNW_INFO_FORMAT_REMOTE_TABLE
	  && (ip >= di->start_ip && ip < di->end_ip));

  a = unw_get_accessors (as);

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
  Debug (15, "ip=0x%lx, start_ip=0x%lx\n",
	 (long) ip, (long) (e->start_ip_offset + segbase));
  fde_addr = e->fde_offset + segbase;
  if ((ret = dwarf_extract_proc_info_from_fde (as, a, &fde_addr, pi,
					       need_unwind_info, arg)) < 0)
    return ret;

  if (ip < pi->start_ip || ip >= pi->end_ip)
    return -UNW_ENOINFO;

  return 0;
}

HIDDEN void
dwarf_put_unwind_info (unw_addr_space_t as, unw_proc_info_t *pi, void *arg)
{
  return;	/* always a nop */
}
