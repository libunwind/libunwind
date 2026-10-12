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

#include "_UCD_internal.h"

#if UNW_TARGET_IA64 && defined(__linux__)

static inline int
get_list_addr (unw_addr_space_t as, unw_word_t *dil_addr, void *arg,
               int *countp)
{
  struct UCD_info *ui = arg;
  unw_word_t res;
  int count = 0;
  unsigned i;

  for (i = 0; i < ui->phdrs_count; ++i)
    {
      coredump_phdr_t *phdr = &ui->phdrs[i];
      ucd_file_t *file;

      if (phdr->p_mapoff
          || !(file = ucd_file_table_at (&ui->ucd_file_table,
                                         phdr->p_backing_file_index)))
        continue;

      /* The image belongs to the file table; see _UCD_find_proc_info().  */
      ui->edi.ei.image = NULL;
      invalidate_edi (&ui->edi);

      if (!_UCD_get_elf_image (ui, phdr->p_vaddr))
        continue;

      Debug (16, "checking object %s\n", file->filename);

      if (tdep_find_unwind_table (&ui->edi, as, file->filename,
                                  phdr->p_vaddr, 0, 0) > 0)
        {
          res = _Uia64_find_dyn_list (as, &ui->edi.di_cache, arg);
          if (res && count++ == 0)
            {
              Debug (12, "dyn_info_list_addr = 0x%lx\n", (long) res);
              *dil_addr = res;
            }
        }
    }
  ui->edi.ei.image = NULL;
  invalidate_edi (&ui->edi);
  *countp = count;
  return 0;
}

#else

/* XXX fix me: there is currently no way to locate the dyn-info list
       by a remote unwinder.  On ia64, this is done via a special
       unwind-table entry.  Perhaps something similar can be done with
       DWARF2 unwind info.  */

static inline int
get_list_addr (unw_addr_space_t  as UNUSED,
               unw_word_t       *dil_addr UNUSED,
               void             *arg UNUSED,
               int              *countp)
{
# warning Implement get_list_addr(), please.
  *countp = 0;
  return 0;
}

#endif

int
_UCD_get_dyn_info_list_addr (unw_addr_space_t as, unw_word_t *dil_addr,
                             void *arg)
{
  int count, ret;

  Debug (12, "looking for dyn_info list\n");

  if ((ret = get_list_addr (as, dil_addr, arg, &count)) < 0)
    return ret;

  /* If multiple dynamic-info list addresses are found, we would have
     to determine which was is the one actually in use (since the
     dynamic name resolution algorithm will pick one "winner").
     Perhaps we'd have to track them all until we find one that's
     non-empty.  Hopefully, this case simply will never arise, since
     only libunwind defines the dynamic info list head. */
  assert (count <= 1);

  return (count > 0) ? 0 : -UNW_ENOINFO;
}
