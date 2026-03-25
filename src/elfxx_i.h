/* libunwind - a platform-independent unwind library
   Copyright (C) 2003, 2005 Hewlett-Packard Co
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

#ifndef elfxx_i_h
#define elfxx_i_h

static inline int
elf_w (dynamic_symtab_count) (Elf_W (Word) *hash, uint32_t *gnu_hash,
                              Elf_W (Word) *sym_num)
{

  if (!sym_num || (!hash && !gnu_hash))
    return -UNW_ENOINFO;

  /* Prefer the SYSV hash table when present since, unlike the GNU
     hash, it directly encodes the symbol count. */
  if (hash)
    {
      *sym_num = hash[1];
    }
  else
    {
      uint32_t nbuckets   = gnu_hash[0];
      uint32_t symoffset  = gnu_hash[1];
      uint32_t bloom_size = gnu_hash[2];
      Elf_W (Addr) *bloom = (Elf_W (Addr) *) &gnu_hash[4];
      uint32_t *buckets   = (uint32_t *) (bloom + bloom_size);
      uint32_t i;
      Elf_W (Word) count;

      for (i = 0, count = 0; i < nbuckets; ++i)
	if (buckets[i] != 0)
	  {
	    if (buckets[i] < count)
	      return -UNW_ENOINFO;
	    count = buckets[i];
	  }

      if (count)
	{
	  uint32_t *hashval;

	  if (count < symoffset)
	    return -UNW_ENOINFO;

	  hashval = buckets + nbuckets + (count - symoffset);
	  do
	    ++count;
	  while (!(*hashval++ & 1));
	}

      *sym_num = count;
    }

  return 0;
}

#endif /* elfxx_i_h */
