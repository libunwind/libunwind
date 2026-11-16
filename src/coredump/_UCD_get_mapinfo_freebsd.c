/**
 * Extract filemap info from a coredump (FreeBSD)
 */
/*
 This file is part of libunwind.

 Copyright (c) 2026, Piotr Kubaj. All rights reserved.

 Permission is hereby granted, free of charge, to any person obtaining a copy of
 this software and associated documentation files (the "Software"), to deal in
 the Software without restriction, including without limitation the rights to
 use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
 of the Software, and to permit persons to whom the Software is furnished to do
 so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in all
 copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 SOFTWARE.
*/
#include "_UCD_internal.h"

#include <stddef.h>
#include <sys/types.h>
#include <sys/user.h>

/**
 * Program headers added for file mappings that are not in the core file.
 */
struct vmmap_phdrs
{
  struct UCD_info *ui;
  coredump_phdr_t *phdrs;
  unsigned         count;
};

static uint32_t
_kve_prot_to_pf (int prot)
{
  uint32_t flags = 0;

  if (prot & KVME_PROT_READ)
    flags |= PF_R;
  if (prot & KVME_PROT_WRITE)
    flags |= PF_W;
  if (prot & KVME_PROT_EXEC)
    flags |= PF_X;
  return flags;
}

/**
 * Handle one file-backed mapping from the FreeBSD/NT_PROCSTAT_VMMAP note.
 *
 * FreeBSD does not dump read-only file-backed mappings (text, rodata), so
 * unlike on Linux there is no PT_LOAD in the core file to attach the backing
 * file to.  Describe such a mapping the way a Linux core does: a PT_LOAD with
 * no file contents, backed by the mapped file.  Mappings that are present in
 * the core file are left alone and keep being read from it.
 */
static int
_add_mapping (struct vmmap_phdrs *new, const struct kinfo_vmentry *kve)
{
  struct UCD_info *ui = new->ui;
  uoff_t start = kve->kve_start;
  uoff_t end = kve->kve_end;

  for (unsigned p = 0; p < ui->phdrs_count; ++p)
    {
      coredump_phdr_t *phdr = &ui->phdrs[p];

      if (phdr->p_type == PT_LOAD
          && start < phdr->p_vaddr + phdr->p_memsz
          && end > phdr->p_vaddr)
        return UNW_ESUCCESS;
    }

  ucd_file_index_t idx = ucd_file_table_insert (&ui->ucd_file_table, kve->kve_path);
  if (idx < 0)
    return idx;

  coredump_phdr_t *phdrs = realloc (new->phdrs, (new->count + 1) * sizeof (*phdrs));
  if (phdrs == NULL)
    return -UNW_ENOMEM;
  new->phdrs = phdrs;

  coredump_phdr_t *phdr = &phdrs[new->count++];
  memset (phdr, 0, sizeof (*phdr));
  phdr->p_type = PT_LOAD;
  phdr->p_flags = _kve_prot_to_pf (kve->kve_protection);
  phdr->p_vaddr = start;
  phdr->p_memsz = end - start;
  phdr->p_backing_file_index = idx;
  phdr->p_mapoff = kve->kve_offset;
  Debug (3, "adding '%s' at index %d (va=0x%llx-0x%llx mapoff=0x%llx)\n",
         kve->kve_path, idx, (unsigned long long) start,
         (unsigned long long) end, (unsigned long long) phdr->p_mapoff);

  return UNW_ESUCCESS;
}

/**
 * Handle the FreeBSD/NT_PROCSTAT_VMMAP note type.
 * @param[in] desc    The note-specific data
 * @param[in] descsz  The size of the note-specific data
 * @param[in] new     Program headers to add
 *
 * The note holds the size of struct kinfo_vmentry followed by the process's
 * kinfo_vmentry records.  The records may be packed, so each one is
 * kve_structsize bytes long and its kve_path is truncated to fit.
 */
static int
_handle_vmmap_note (uint8_t *desc, uint32_t descsz, struct vmmap_phdrs *new)
{
  const size_t path_offset = offsetof (struct kinfo_vmentry, kve_path);
  int structsize;

  if (descsz < sizeof (structsize))
    return UNW_ESUCCESS;

  /* desc is only 4-byte aligned, so copy the records out before reading
   * their 64-bit fields. */
  memcpy (&structsize, desc, sizeof (structsize));
  if (structsize != sizeof (struct kinfo_vmentry))
    {
      Debug (0, "unexpected kinfo_vmentry size %d\n", structsize);
      return UNW_ESUCCESS;
    }

  size_t off = sizeof (structsize);
  while (off + path_offset <= descsz)
    {
      struct kinfo_vmentry kve;
      int kve_structsize;

      memcpy (&kve_structsize, desc + off, sizeof (kve_structsize));
      if (kve_structsize < (int) path_offset
          || (size_t) kve_structsize > descsz - off)
        break;

      memset (&kve, 0, sizeof (kve));
      memcpy (&kve, desc + off,
              (size_t) kve_structsize < sizeof (kve) ? (size_t) kve_structsize : sizeof (kve));
      kve.kve_path[sizeof (kve.kve_path) - 1] = '\0';

      if (kve.kve_type == KVME_TYPE_VNODE
          && (kve.kve_protection & KVME_PROT_READ) != 0
          && kve.kve_path[0] != '\0')
        {
          int ret = _add_mapping (new, &kve);
          if (ret != UNW_ESUCCESS)
            return ret;
        }

      off += kve_structsize;
    }

  return UNW_ESUCCESS;
}

/**
 * Callback to handle notes.
 * @param[in]  n_namesz size of name data
 * @param[in]  n_descsz size of desc data
 * @param[in]  n_type type of note
 * @param[in]  name zero-terminated string, n_namesz bytes plus alignment padding
 * @param[in]  desc note-specific data, n_descsz bytes plus alignment padding
 * @param[in]  arg user-supplied callback argument
 */
static int
_handle_pt_note_segment (uint32_t  n_namesz UNUSED,
                         uint32_t  n_descsz,
                         uint32_t  n_type,
                         char     *name,
                         uint8_t  *desc,
                         void     *arg)
{
  if (n_type == NT_PROCSTAT_VMMAP && strcmp (name, "FreeBSD") == 0)
    {
      return _handle_vmmap_note (desc, n_descsz, arg);
    }
  return UNW_ESUCCESS;
}

/**
 * Get filemap info from core file (FreeBSD)
 *
 * Attach the files named in the FreeBSD/NT_PROCSTAT_VMMAP note to the
 * mappings they back, adding program headers for the mappings that are
 * not in the core file.  ui->phdrs may be reallocated.
 *
 * Since there may or may not be a vmmap note it's OK for this function to
 * find nothing.
 */
int
_UCD_get_mapinfo (struct UCD_info *ui, coredump_phdr_t *phdrs, unsigned phdr_size)
{
  struct vmmap_phdrs new = { ui, NULL, 0 };
  int ret = UNW_ESUCCESS;

  for (unsigned i = 0; i < phdr_size && ret == UNW_ESUCCESS; ++i)
    {
      if (phdrs[i].p_type == PT_NOTE)
        {
          uint8_t *segment;
          size_t segment_size;

          if (_UCD_elf_read_segment (ui, &phdrs[i], &segment, &segment_size) == UNW_ESUCCESS)
            {
              ret = _UCD_elf_visit_notes (segment, segment_size, _handle_pt_note_segment, &new);
              free (segment);
            }
        }
    }

  if (ret == UNW_ESUCCESS && new.count > 0)
    {
      coredump_phdr_t *all = realloc (ui->phdrs, (ui->phdrs_count + new.count) * sizeof (*all));
      if (all == NULL)
        ret = -UNW_ENOMEM;
      else
        {
          memcpy (all + ui->phdrs_count, new.phdrs, new.count * sizeof (*all));
          ui->phdrs = all;
          ui->phdrs_count += new.count;
        }
    }

  free (new.phdrs);
  return ret;
}
