/* libunwind - a platform-independent unwind library
   Copyright (C) 2003 Hewlett-Packard Co
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

#include <elf.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>

#include <sys/mman.h>

#include "_UPT_internal.h"

static unw_word_t
find_gp (struct UPT_info *ui, Elf64_Phdr *pdyn, Elf64_Addr load_base)
{
  Elf64_Off soff, str_soff;
  Elf64_Ehdr *ehdr = ui->image;
  Elf64_Shdr *shdr;
  Elf64_Shdr *str_shdr;
  Elf64_Addr gp = 0;
  char *strtab;
  int i;

  if (pdyn)
    {
      /* If we have a PT_DYNAMIC program header, fetch the gp-value
	 from the DT_PLTGOT entry.  */
      Elf64_Dyn *dyn = (Elf64_Dyn *) (pdyn->p_offset + ui->image);
      for (; dyn->d_tag != DT_NULL; ++dyn)
	if (dyn->d_tag == DT_PLTGOT)
	  {
	    gp = (Elf64_Addr) dyn->d_un.d_ptr + load_base;
	    goto done;
	  }
    }

  /* Without a PT_DYAMIC header, lets try to look for a non-empty .opd
     section.  If there is such a section, we know it's full of
     function descriptors, and we can simply pick up the gp from the
     second word of the first entry in this table.  */

  soff = ehdr->e_shoff;
  str_soff = soff + (ehdr->e_shstrndx * ehdr->e_shentsize);

  if (soff + ehdr->e_shnum * ehdr->e_shentsize > ui->image_size)
    {
      debug (1, "%s: section table outside of image? (%lu > %lu)", __FUNCTION__,
	     soff + ehdr->e_shnum * ehdr->e_shentsize, ui->image_size);
      goto done;
    }

  shdr = (Elf64_Shdr *) ((char *) ui->image + soff);
  str_shdr = (Elf64_Shdr *) ((char *) ui->image + str_soff);
  strtab = (char *) ui->image + str_shdr->sh_offset;
  for (i = 0; i < ehdr->e_shnum; ++i)
    {
      if (strcmp (strtab + shdr->sh_name, ".opd") == 0
	  && shdr->sh_size >= 16)
	{
	  gp = ((Elf64_Addr *) (ui->image + shdr->sh_offset))[1];
	  goto done;
	}
      shdr = (Elf64_Shdr *) (((char *) shdr) + ehdr->e_shentsize);
    }

 done:
  debug (100, "%s: image at %lx, gp = %lx\n", ui->image, gp);
  return gp;
}

static inline int
elf64_valid_object (struct UPT_info *ui)
{
  return memcmp (ui->image, ELFMAG, SELFMAG) == 0;
}

HIDDEN unw_dyn_info_t *
_UPTi_find_unwind_table (struct UPT_info *ui, unw_addr_space_t as,
			 char *path, unw_word_t segbase, unw_word_t mapoff)
{
  Elf64_Phdr *phdr, *ptxt = NULL, *punw = NULL, *pdyn = NULL;
  struct stat stat;
  Elf64_Ehdr *ehdr;
  int fd, i;

  if (ui->image)
    {
      munmap (ui->image, ui->image_size);
      ui->image = NULL;
      ui->image_size = 0;

      /* invalidate the cache: */
      ui->di_cache.start_ip = ui->di_cache.end_ip = 0;
    }

  fd = open (path, O_RDONLY);
  if (fd < 0)
    return NULL;

  if (fstat (fd, &stat) < 0)
    {
      close (fd);
      return NULL;
    }

  ui->image_size = stat.st_size;
  ui->image = mmap (NULL, ui->image_size, PROT_READ, MAP_PRIVATE, fd, 0);
  close (fd);
  if (ui->image == MAP_FAILED)
    return NULL;

  if (!elf64_valid_object (ui))
    return NULL;

  ehdr = ui->image;
  phdr = (Elf64_Phdr *) ((char *) ui->image + ehdr->e_phoff);

  for (i = 0; i < ehdr->e_phnum; ++i)
    {
      switch (phdr[i].p_type)
	{
	case PT_LOAD:
	  if (phdr[i].p_offset == mapoff)
	    ptxt = phdr + i;
	  break;

	case PT_IA_64_UNWIND:
	  punw = phdr + i;
	  break;

	case PT_DYNAMIC:
	  pdyn = phdr + i;
	  break;

	default:
	  break;
	}
    }
  if (!ptxt || !punw)
    return NULL;

  ui->di_cache.start_ip = segbase;
  ui->di_cache.end_ip = ui->di_cache.start_ip + ptxt->p_memsz;
  ui->di_cache.gp = find_gp (ui, pdyn, segbase - ptxt->p_vaddr);
  ui->di_cache.format = UNW_INFO_FORMAT_TABLE;
  ui->di_cache.u.ti.name_ptr = 0;
  ui->di_cache.u.ti.segbase = segbase;
  ui->di_cache.u.ti.table_len = punw->p_memsz / sizeof (unw_word_t);
  ui->di_cache.u.ti.table_data = (unw_word_t *)
    ((char *) ui->image + (punw->p_vaddr - ptxt->p_vaddr));
  return &ui->di_cache;
}

static unw_dyn_info_t *
get_unwind_info (struct UPT_info *ui, unw_addr_space_t as,
		 unw_word_t ip, unw_proc_info_t *pi,
		 int need_unwind_info)
{
  unsigned long segbase, hi, mapoff;
  struct map_iterator mi;
  char path[PATH_MAX];

#if UNW_TARGET_IA64
  if (!ui->ktab.start_ip)
    {
      int ret = _Uia64_get_kernel_table (&ui->ktab);
      if (ret < 0)
	return NULL;
    }
  if (ip >= ui->ktab.start_ip && ip < ui->ktab.end_ip)
    return &ui->ktab;
#endif

  if (ip >= ui->di_cache.start_ip && ip < ui->di_cache.end_ip)
    return &ui->di_cache;

  maps_init (&mi, ui->pid);
  while (maps_next (&mi, &segbase, &hi, &mapoff, path))
    {
      if (ip >= segbase && ip < hi)
	break;
    }
  maps_close (&mi);

  if (ip < segbase || ip >= hi)
    return NULL;

  return _UPTi_find_unwind_table (ui, as, path, segbase, mapoff);
}

int
_UPT_find_proc_info (unw_addr_space_t as, unw_word_t ip, unw_proc_info_t *pi,
		     int need_unwind_info, void *arg)
{
  struct UPT_info *ui = arg;
  unw_dyn_info_t *di;

  di = get_unwind_info (ui, as, ip, pi, need_unwind_info);
  if (!di)
    return -UNW_ENOINFO;

  return _Uia64_search_unwind_table (as, ip, di, pi, need_unwind_info, arg);
}
