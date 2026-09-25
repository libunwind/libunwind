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
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/
#include "libunwind_i.h"

#ifndef UNW_REMOTE_ONLY

#if defined(HAVE__DL_FIND_OBJECT) && defined(HAVE_DL_ITERATE_PHDR)
#include <dlfcn.h>
#include <sys/auxv.h>

/* Describe the loaded object containing ip in *info, the way
   dl_iterate_phdr would, using _dl_find_object. Unlike dl_iterate_phdr,
   _dl_find_object takes no locks, so it is async-signal-safe.

   _dl_find_object does not report the program headers. For the main
   program take them from the auxiliary vector, like the dynamic linker
   does; in static executables the reported mapping need not start at the
   ELF header. For other objects find them through the ELF header at the
   start of the object's mapping, and check that it describes this object
   at this load address.

   Returns 1 on success, 0 if no loaded object contains ip, or -1 if the
   program headers could not be located. */
static int
find_object_phdr_info (unw_word_t ip, struct dl_phdr_info *info)
{
  struct dl_find_object dlfo, main_dlfo;
  const ElfW(Ehdr) *ehdr;
  const ElfW(Phdr) *phdr;
  uintptr_t bias, start, size;
  size_t phnum, n;

  if (_dl_find_object ((void *) (uintptr_t) ip, &dlfo) != 0)
    return 0;

  bias = dlfo.dlfo_link_map->l_addr;

  if (_dl_find_object ((void *) getauxval (AT_ENTRY), &main_dlfo) == 0
      && main_dlfo.dlfo_link_map == dlfo.dlfo_link_map
      && getauxval (AT_PHENT) == sizeof (ElfW(Phdr)))
    {
      phdr = (const ElfW(Phdr) *) getauxval (AT_PHDR);
      phnum = getauxval (AT_PHNUM);
      goto found;
    }

  start = (uintptr_t) dlfo.dlfo_map_start;
  size = (uintptr_t) dlfo.dlfo_map_end - start;
  ehdr = (const ElfW(Ehdr) *) start;

  if (size < sizeof (*ehdr)
      || memcmp (ehdr->e_ident, ELFMAG, SELFMAG) != 0
      || ehdr->e_phentsize != sizeof (ElfW(Phdr))
      || ehdr->e_phoff > size
      || ehdr->e_phnum > (size - ehdr->e_phoff) / sizeof (ElfW(Phdr)))
    return -1;

  phdr = (const ElfW(Phdr) *) (start + ehdr->e_phoff);
  phnum = ehdr->e_phnum;

  for (n = 0; n < phnum; n++)
    if (phdr[n].p_type == PT_PHDR && bias + phdr[n].p_vaddr != (uintptr_t) phdr)
      return -1;

  /* The segment mapping file offset 0 holds the ELF header. */
  for (n = 0; n < phnum; n++)
    if (phdr[n].p_type == PT_LOAD && phdr[n].p_offset == 0)
      break;
  if (n == phnum || bias + phdr[n].p_vaddr != start)
    return -1;

found:
  memset (info, 0, sizeof (*info));
  info->dlpi_addr = bias;
  info->dlpi_name = dlfo.dlfo_link_map->l_name;
  info->dlpi_phdr = phdr;
  info->dlpi_phnum = phnum;
  return 1;
}
#endif /* HAVE__DL_FIND_OBJECT && HAVE_DL_ITERATE_PHDR */

/* Call callback on the loaded object containing ip, the way
   as->iterate_phdr_function would. Callers' callbacks ignore objects
   that do not contain ip, so this is equivalent to iterating over all of
   them, but it avoids dl_iterate_phdr when _dl_find_object can be used.
   An application-supplied iterate_phdr function is always honored, since
   it may know about objects the dynamic linker does not. */
HIDDEN int
unwi_iterate_phdr_for_ip (unw_addr_space_t as, unw_word_t ip,
                          unw_iterate_phdr_callback_t callback, void *data)
{
  intrmask_t saved_mask;
  int ret;

  SIGPROCMASK (SIG_SETMASK, &unwi_full_mask, &saved_mask);
#if defined(HAVE__DL_FIND_OBJECT) && defined(HAVE_DL_ITERATE_PHDR)
  if (as->iterate_phdr_function == dl_iterate_phdr)
    {
      struct dl_phdr_info info;

      ret = find_object_phdr_info (ip, &info);
      if (ret >= 0)
        {
          if (ret > 0)
            ret = callback (&info, sizeof (info), data);
          SIGPROCMASK (SIG_SETMASK, &saved_mask, NULL);
          return ret;
        }
      Debug (3, "cannot locate program headers for IP=0x%lx, "
             "falling back to dl_iterate_phdr\n", (long) ip);
    }
#endif
  ret = as->iterate_phdr_function (callback, data);
  SIGPROCMASK (SIG_SETMASK, &saved_mask, NULL);
  return ret;
}

#endif /* !UNW_REMOTE_ONLY */
