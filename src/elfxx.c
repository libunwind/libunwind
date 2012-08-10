/* libunwind - a platform-independent unwind library
   Copyright (C) 2003-2005 Hewlett-Packard Co
   Copyright (C) 2007 David Mosberger-Tang
	Contributed by David Mosberger-Tang <dmosberger@gmail.com>

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

#include <stdio.h>
#include <sys/param.h>

#include "libunwind_i.h"

static Elf_W (Shdr)*
elf_w (section_table) (struct elf_image *ei)
{
  Elf_W (Ehdr) *ehdr = ei->image;
  Elf_W (Off) soff;

  soff = ehdr->e_shoff;
  if (soff + ehdr->e_shnum * ehdr->e_shentsize > ei->size)
    {
      Debug (1, "section table outside of image? (%lu > %lu)\n",
	     (unsigned long) (soff + ehdr->e_shnum * ehdr->e_shentsize),
	     (unsigned long) ei->size);
      return NULL;
    }

  return (Elf_W (Shdr) *) ((char *) ei->image + soff);
}

static char*
elf_w (string_table) (struct elf_image *ei, int section)
{
  Elf_W (Ehdr) *ehdr = ei->image;
  Elf_W (Off) soff, str_soff;
  Elf_W (Shdr) *str_shdr;

  /* this offset is assumed to be OK */
  soff = ehdr->e_shoff;

  str_soff = soff + (section * ehdr->e_shentsize);
  if (str_soff + ehdr->e_shentsize > ei->size)
    {
      Debug (1, "string shdr table outside of image? (%lu > %lu)\n",
	     (unsigned long) (str_soff + ehdr->e_shentsize),
	     (unsigned long) ei->size);
      return NULL;
    }
  str_shdr = (Elf_W (Shdr) *) ((char *) ei->image + str_soff);

  if (str_shdr->sh_offset + str_shdr->sh_size > ei->size)
    {
      Debug (1, "string table outside of image? (%lu > %lu)\n",
	     (unsigned long) (str_shdr->sh_offset + str_shdr->sh_size),
	     (unsigned long) ei->size);
      return NULL;
    }

  Debug (16, "strtab=0x%lx\n", (long) str_shdr->sh_offset);
  return ei->image + str_shdr->sh_offset;
}

static int
elf_w (lookup_symbol) (unw_addr_space_t as,
		       unw_word_t ip, struct elf_image *ei,
		       Elf_W (Addr) load_offset,
		       char *buf, size_t buf_len, unw_word_t *offp)
{
  size_t syment_size;
  Elf_W (Ehdr) *ehdr = ei->image;
  Elf_W (Sym) *sym, *symtab, *symtab_end;
  Elf_W (Shdr) *shdr;
  Elf_W (Addr) val, min_dist = ~(Elf_W (Addr))0;
  int i, ret = -UNW_ENOINFO;
  char *strtab;

  if (!elf_w (valid_object) (ei))
    return -UNW_ENOINFO;

  shdr = elf_w (section_table) (ei);
  if (!shdr)
    return -UNW_ENOINFO;

  for (i = 0; i < ehdr->e_shnum; ++i)
    {
      switch (shdr->sh_type)
	{
	case SHT_SYMTAB:
	case SHT_DYNSYM:
	  symtab = (Elf_W (Sym) *) ((char *) ei->image + shdr->sh_offset);
	  symtab_end = (Elf_W (Sym) *) ((char *) symtab + shdr->sh_size);
	  syment_size = shdr->sh_entsize;

	  strtab = elf_w (string_table) (ei, shdr->sh_link);
	  if (!strtab)
	    break;

	  Debug (16, "symtab=0x%lx[%d]\n",
		 (long) shdr->sh_offset, shdr->sh_type);

	  for (sym = symtab;
	       sym < symtab_end;
	       sym = (Elf_W (Sym) *) ((char *) sym + syment_size))
	    {
	      if (ELF_W (ST_TYPE) (sym->st_info) == STT_FUNC
		  && sym->st_shndx != SHN_UNDEF)
		{
		  if (tdep_get_func_addr (as, sym->st_value, &val) < 0)
		    continue;
		  if (sym->st_shndx != SHN_ABS)
		    val += load_offset;
		  Debug (16, "0x%016lx info=0x%02x %s\n",
			 (long) val, sym->st_info, strtab + sym->st_name);

		  if ((Elf_W (Addr)) (ip - val) < min_dist)
		    {
		      min_dist = (Elf_W (Addr)) (ip - val);
		      strncpy (buf, strtab + sym->st_name, buf_len);
		      buf[buf_len - 1] = '\0';
		      ret = (strlen (strtab + sym->st_name) >= buf_len
			     ? -UNW_ENOMEM : 0);
		    }
		}
	    }
	  break;

	default:
	  break;
	}
      shdr = (Elf_W (Shdr) *) (((char *) shdr) + ehdr->e_shentsize);
    }
  if (min_dist >= ei->size)
    return -UNW_ENOINFO;		/* not found */
  if (offp)
    *offp = min_dist;
  return ret;
}

static Elf_W (Addr)
elf_w (get_load_offset) (struct elf_image *ei, unsigned long segbase,
			 unsigned long mapoff)
{
  Elf_W (Addr) offset = 0;
  Elf_W (Ehdr) *ehdr;
  Elf_W (Phdr) *phdr;
  int i;

  ehdr = ei->image;
  phdr = (Elf_W (Phdr) *) ((char *) ei->image + ehdr->e_phoff);

  for (i = 0; i < ehdr->e_phnum; ++i)
    if (phdr[i].p_type == PT_LOAD && phdr[i].p_offset == mapoff)
      {
	offset = segbase - phdr[i].p_vaddr;
	break;
      }

  return offset;
}

/* Find the ELF image that contains IP and return the "closest"
   procedure name, if there is one.  With some caching, this could be
   sped up greatly, but until an application materializes that's
   sensitive to the performance of this routine, why bother...  */

HIDDEN int
elf_w (get_proc_name_in_image) (unw_addr_space_t as, struct elf_image *ei,
		       unsigned long segbase,
		       unsigned long mapoff,
		       unw_word_t ip,
		       char *buf, size_t buf_len, unw_word_t *offp)
{
  Elf_W (Addr) load_offset;
  int ret;

  load_offset = elf_w (get_load_offset) (ei, segbase, mapoff);
  ret = elf_w (lookup_symbol) (as, ip, ei, load_offset, buf, buf_len, offp);



  return ret;
}

HIDDEN int
elf_w (get_proc_name) (unw_addr_space_t as, pid_t pid, unw_word_t ip,
		       char *buf, size_t buf_len, unw_word_t *offp)
{
  unsigned long segbase, mapoff;
  struct elf_image ei;
  int ret;

  ret = tdep_get_elf_image (&ei, pid, ip, &segbase, &mapoff, NULL, 0);
  if (ret < 0)
    return ret;

  ret = elf_w (get_proc_name_in_image) (as, &ei, segbase, mapoff, ip, buf, buf_len, offp);

  munmap (ei.image, ei.size);
  ei.image = NULL;

  return ret;
}
