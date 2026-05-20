/* libunwind - a platform-independent unwind library

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

#if defined(HAVE_ELF_H)
# include <elf.h>
#elif defined(HAVE_SYS_ELF_H)
# include <sys/elf.h>
#endif

#include "_UCD_internal.h"

/**
 * Manually associate an ELF file with the PT_LOAD segment containing @vaddr.
 *
 * Use this when the core file lacks NT_FILE notes (e.g. QEMU user-mode cores)
 * to supply the backing file for a given load address.  @vaddr must fall
 * inside a PT_LOAD segment in the core.
 *
 * @param[in] ui    UCD info created by _UCD_create()
 * @param[in] vaddr load address of the ELF image (from /proc/self/maps or eu-unstrip)
 * @param[in] path  filesystem path to the ELF image
 * @return UNW_ESUCCESS on success, negative error code on failure
 */
int
_UCD_add_backing_file_at_vaddr(struct UCD_info *ui,
                               unsigned long    vaddr,
                               const char      *path)
{
  for (unsigned p = 0; p < ui->phdrs_count; ++p)
    {
      coredump_phdr_t *phdr = &ui->phdrs[p];
      if (phdr->p_type == PT_LOAD
          && (uoff_t)vaddr >= phdr->p_vaddr
          && (uoff_t)vaddr <  phdr->p_vaddr + phdr->p_memsz)
        {
          /* Skip phdrs already populated by NT_FILE. */
          if (phdr->p_backing_file_index != ucd_file_no_index)
            return UNW_ESUCCESS;
          ucd_file_index_t idx = ucd_file_table_insert (&ui->ucd_file_table, path);
          if (idx == ucd_file_no_index)
            return -UNW_ENOMEM;
          phdr->p_backing_file_index = idx;
          return UNW_ESUCCESS;
        }
    }
  return -UNW_ENOINFO;
}
