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

#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <sys/types.h>


extern HIDDEN int
elfW (valid_object) (struct elf_image *ei)
{
  if (ei->size <= EI_CLASS)
    return 0;

  return (memcmp (ei->image, ELFMAG, SELFMAG) == 0
	  && ((uint8_t *) ei->image)[EI_CLASS] == ELF_CLASS);
}


static int
elfW (lookup_symbol) (unw_word_t ip, struct elf_image *ei,
		      ElfW (Addr) load_offset,
		      char *buf, size_t buf_len, unw_word_t *offp)
{
  size_t syment_size, str_size;
  ElfW (Ehdr) *ehdr = ei->image;
  ElfW (Sym) *sym, *symtab, *symtab_end;
  ElfW (Off) soff, str_soff;
  ElfW (Shdr) *shdr, *str_shdr;
  ElfW (Addr) val, min_dist = ~(ElfW (Addr))0;
  char *strtab;
  int i;

  if (!elfW (valid_object) (ei))
    return -1;

  soff = ehdr->e_shoff;
  if (soff + ehdr->e_shnum * ehdr->e_shentsize > ei->size)
    {
      debug (1, "%s: section table outside of image? (%lu > %lu)\n",
	     __FUNCTION__, soff + ehdr->e_shnum * ehdr->e_shentsize,
	     ei->size);
      return -1;
    }

  shdr = (ElfW (Shdr) *) ((char *) ei->image + soff);

  for (i = 0; i < ehdr->e_shnum; ++i)
    {
      switch (shdr->sh_type)
	{
	case SHT_SYMTAB:
	case SHT_DYNSYM:
	  symtab = (ElfW (Sym) *) ((char *) ei->image + shdr->sh_offset);
	  symtab_end = (ElfW (Sym) *) ((char *) symtab + shdr->sh_size);
	  syment_size = shdr->sh_entsize;

	  str_soff = soff + (shdr->sh_link * ehdr->e_shentsize);
	  if (str_soff + ehdr->e_shentsize >= ei->size)
	    {
	      debug (1, "%s: string table outside of image? (%lu >= %lu)\n",
		     __FUNCTION__, str_soff + ehdr->e_shentsize, ei->size);
	      break;
	    }
	  str_shdr = (ElfW (Shdr) *) ((char *) ei->image + str_soff);
	  str_size = str_shdr->sh_size;
	  strtab = (char *) ei->image + str_shdr->sh_offset;

	  debug (10, "symtab=0x%lx[%d], strtab=0x%lx\n", shdr->sh_offset,
		 shdr->sh_type, str_shdr->sh_offset);

	  for (sym = symtab;
	       sym < symtab_end;
	       sym = (ElfW (Sym) *) ((char *) sym + syment_size))
	    {
	      if (ELFW (ST_TYPE) (sym->st_info) == STT_FUNC
		  && sym->st_shndx != SHN_UNDEF)
		{
		  val = sym->st_value;
		  if (sym->st_shndx != SHN_ABS)
		    val += load_offset;
		  debug (100, "0x%016lx info=0x%02x %s\n",
			 val, sym->st_info, strtab + sym->st_name);

		  if ((ElfW (Addr)) (ip - val) < min_dist)
		    {
		      min_dist = (ElfW (Addr)) (ip - val);
		      buf[buf_len - 1] = 'x';
		      strncpy (buf, strtab + sym->st_name, buf_len);
		      buf[buf_len - 1] = '\0';
		    }
		}
	    }
	  break;

	default:
	  break;
	}
      shdr = (Elf64_Shdr *) (((char *) shdr) + ehdr->e_shentsize);
    }
  if (min_dist >= ei->size)
    return -1;			/* not found */
  if (offp)
    *offp = min_dist;
  return 0;
}

/* Find the ELF image that contains IP and return the "closest"
   procedure name, if there is one.  With some caching, this could be
   sped up greatly, but until an application materializes that's
   sensitive to the performance of this routine, why bother...  */

HIDDEN int
elfW (get_proc_name) (unw_word_t ip, char *buf, size_t buf_len,
		      unw_word_t *offp)
{
  unsigned long segbase, mapoff;
  ElfW (Addr) load_offset = 0;
  struct elf_image ei;
  ElfW (Ehdr) *ehdr;
  ElfW (Phdr) *phdr;
  int i, ret;

  ret = tdep_get_elf_image (&ei, getpid (), ip, &segbase, &mapoff);
  if (ret < 0)
    return ret;

  ehdr = ei.image;
  phdr = (Elf64_Phdr *) ((char *) ei.image + ehdr->e_phoff);

  for (i = 0; i < ehdr->e_phnum; ++i)
    if (phdr[i].p_type == PT_LOAD && phdr[i].p_offset == mapoff)
      {
	load_offset = segbase - phdr[i].p_vaddr;
	break;
      }

  ret = elfW (lookup_symbol) (ip, &ei, load_offset, buf, buf_len, offp);

  munmap (ei.image, ei.size);
  ei.image = NULL;

  return ret;
}
