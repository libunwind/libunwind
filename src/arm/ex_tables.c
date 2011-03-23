/* libunwind - a platform-independent unwind library
   Copyright 2011 Linaro Limited

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

#include "libunwind_i.h"

#define ARM_EXIDX_CANT_UNWIND	0x00000001
#define ARM_EXIDX_COMPACT	0x80000000

#define ARM_EXTBL_OP_FINISH	0xb0

#ifdef ARM_EXIDX_TABLE_MALLOC
static struct arm_exidx_table *arm_exidx_table_list;
#else
#define ARM_EXIDX_TABLE_LIMIT 32
static struct arm_exidx_table arm_exidx_tables[ARM_EXIDX_TABLE_LIMIT];
static unsigned arm_exidx_table_count = 0;
#endif
static const char *arm_exidx_appname;

static void
arm_exidx_table_reset_all (void)
{
#ifdef ARM_EXIDX_TABLE_MALLOC
  while (NULL != arm_exidx_table_list)
    {
      struct arm_exidx_table *next = arm_exidx_table_list->next;
      free (arm_exidx_table_list);
      arm_exidx_table_list = next;
    }
#else
  arm_exidx_table_count = 0;
#endif
}

HIDDEN int
arm_exidx_table_add (const char *name,
    struct arm_exidx_entry *start, struct arm_exidx_entry *end)
{
#ifdef ARM_EXIDX_TABLE_MALLOC
  struct arm_exidx_table *table = malloc (sizeof (*table));
  if (NULL == table)
    return -1;
  table->next = arm_exidx_table_list;
  arm_exidx_table_list = table;
#else
  if (arm_exidx_table_count >= ARM_EXIDX_TABLE_LIMIT)
    return -1;
  struct arm_exidx_table *table = &arm_exidx_tables[arm_exidx_table_count++];
#endif
  table->name = name;
  table->start = start;
  table->end = end;
  table->start_addr = prel31_to_addr (&start->addr);
  table->end_addr = prel31_to_addr (&(end - 1)->addr);
  Debug (2, "name=%s, range=%p-%p, addr=%p-%p\n",
      name, start, end, table->start_addr, table->end_addr);
  return 0;
}

/**
 * Located the appropriate unwind table
 */
HIDDEN struct arm_exidx_table *
arm_exidx_table_find (void *pc)
{
  struct arm_exidx_table *table;
#ifdef ARM_EXIDX_TABLE_MALLOC
  for (table = arm_exidx_table_list; table != NULL; table = table->next)
  {
#else
  unsigned i;
  for (i = 0; i < arm_exidx_table_count; i++)
  {
    table = &arm_exidx_tables[i];
#endif
    if (pc >= table->start_addr && pc < table->end_addr)
      return table;
  }
  return NULL;
}

/* Based on Linux arm/arm/kernel/unwind.c:search_index() */
HIDDEN struct arm_exidx_entry *
arm_exidx_table_lookup (struct arm_exidx_table *table, void *pc)
{
  struct arm_exidx_entry *first = table->start, *last = table->end - 1;
  if (pc < prel31_to_addr (&first->addr))
    return NULL;
  else if (pc >= prel31_to_addr (&last->addr))
    return last;
  while (first < last - 1)
    {
      struct arm_exidx_entry *mid = first + ((last - first + 1) >> 1);
      if (pc < prel31_to_addr (&mid->addr))
	last = mid;
      else
	first = mid;
    }
  return first;
}

/**
 * Applies the given command onto the new state to the given dwarf_cursor.
 */
HIDDEN int
arm_exidx_apply_cmd (struct arm_exbuf_data *edata, struct dwarf_cursor *c)
{
  int ret = 0;
  unsigned i;

  switch (edata->cmd)
    {
    case ARM_EXIDX_CMD_FINISH:
      /* Set LR to PC if not set already.  */
      if (DWARF_IS_NULL_LOC (c->loc[UNW_ARM_R15]))
	c->loc[UNW_ARM_R15] = c->loc[UNW_ARM_R14];
      /* Set IP.  */
      dwarf_get (c, c->loc[UNW_ARM_R15], &c->ip);
      break;
    case ARM_EXIDX_CMD_DATA_PUSH:
      Debug (2, "vsp = vsp - %d\n", edata->data);
      c->cfa -= edata->data;
      break;
    case ARM_EXIDX_CMD_DATA_POP:
      Debug (2, "vsp = vsp + %d\n", edata->data);
      c->cfa += edata->data;
      break;
    case ARM_EXIDX_CMD_REG_POP:
      for (i = 0; i < 16; i++)
	if (edata->data & (1 << i))
	  {
	    Debug (2, "pop {r%d}\n", i);
	    c->loc[UNW_ARM_R0 + i] = DWARF_LOC (c->cfa, 0);
	    c->cfa += 4;
	  }
      /* Set cfa in case the SP got popped. */
      if (edata->data & (1 << 13))
	dwarf_get (c, c->loc[UNW_ARM_R13], &c->cfa);
      break;
    case ARM_EXIDX_CMD_REG_TO_SP:
      assert (edata->data < 16);
      Debug (2, "vsp = r%d\n", edata->data);
      c->loc[UNW_ARM_R13] = c->loc[UNW_ARM_R0 + edata->data];
      dwarf_get (c, c->loc[UNW_ARM_R13], &c->cfa);
      break;
    case ARM_EXIDX_CMD_VFP_POP:
      /* Skip VFP registers, but be sure to adjust stack */
      for (i = ARM_EXBUF_START (edata->data); i < ARM_EXBUF_END (edata->data);
	   i++)
	c->cfa += 8;
      if (!(edata->data & ARM_EXIDX_VFP_DOUBLE))
	c->cfa += 4;
      break;
    case ARM_EXIDX_CMD_WREG_POP:
      for (i = ARM_EXBUF_START (edata->data); i < ARM_EXBUF_END (edata->data);
	   i++)
	c->cfa += 8;
      break;
    case ARM_EXIDX_CMD_WCGR_POP:
      for (i = 0; i < 4; i++)
	if (edata->data & (1 << i))
	  c->cfa += 4;
      break;
    case ARM_EXIDX_CMD_REFUSED:
    case ARM_EXIDX_CMD_RESERVED:
      ret = -1;
      break;
    }
  return ret;
}


HIDDEN int
arm_exidx_decode (const uint8_t *buf, uint8_t len, struct dwarf_cursor *c)
{
#define READ_OP() *buf++
  const uint8_t *end = buf + len;
  int ret;
  struct arm_exbuf_data edata;

  assert(buf != NULL);
  assert(len > 0);

  while (buf < end)
    {
      uint8_t op = READ_OP ();
      if ((op & 0xc0) == 0x00)
	{
	  edata.cmd = ARM_EXIDX_CMD_DATA_POP;
	  edata.data = (((int)op & 0x3f) << 2) + 4;
	}
      else if ((op & 0xc0) == 0x40)
	{
	  edata.cmd = ARM_EXIDX_CMD_DATA_PUSH;
	  edata.data = (((int)op & 0x3f) << 2) + 4;
	}
      else if ((op & 0xf0) == 0x80)
	{
	  uint8_t op2 = READ_OP ();
	  if (op == 0x80 && op2 == 0x00)
	    edata.cmd = ARM_EXIDX_CMD_REFUSED;
	  else
	    {
	      edata.cmd = ARM_EXIDX_CMD_REG_POP;
	      edata.data = ((op & 0xf) << 8) | op2;
	      edata.data = edata.data << 4;
	    }
	}
      else if ((op & 0xf0) == 0x90)
	{
	  if (op == 0x9d || op == 0x9f)
	    edata.cmd = ARM_EXIDX_CMD_RESERVED;
	  else
	    {
	      edata.cmd = ARM_EXIDX_CMD_REG_TO_SP;
	      edata.data = op & 0x0f;
	    }
	}
      else if ((op & 0xf0) == 0xa0)
	{
	  unsigned end = (op & 0x07);
	  edata.data = (1 << (end + 1)) - 1;
	  edata.data = edata.data << 4;
	  if (op & 0x08)
	    edata.data |= 1 << 14;
	  edata.cmd = ARM_EXIDX_CMD_REG_POP;
	}
      else if (op == ARM_EXTBL_OP_FINISH)
	{
	  edata.cmd = ARM_EXIDX_CMD_FINISH;
	  buf = end;
	}
      else if (op == 0xb1)
	{
	  uint8_t op2 = READ_OP ();
	  if (op2 == 0 || (op2 & 0xf0))
	    edata.cmd = ARM_EXIDX_CMD_RESERVED;
	  else
	    {
	      edata.cmd = ARM_EXIDX_CMD_REG_POP;
	      edata.data = op2 & 0x0f;
	    }
	}
      else if (op == 0xb2)
	{
	  uint32_t offset = 0;
	  uint8_t byte, shift = 0;
	  do
	    {
	      byte = READ_OP ();
	      offset |= (byte & 0x7f) << shift;
	      shift += 7;
	    }
	  while (byte & 0x80);
	  edata.data = offset * 4 + 0x204;
	  edata.cmd = ARM_EXIDX_CMD_DATA_POP;
	}
      else if (op == 0xb3 || op == 0xc8 || op == 0xc9)
	{
	  edata.cmd = ARM_EXIDX_CMD_VFP_POP;
	  edata.data = READ_OP ();
	  if (op == 0xc8)
	    edata.data |= ARM_EXIDX_VFP_SHIFT_16;
	  if (op != 0xb3)
	    edata.data |= ARM_EXIDX_VFP_DOUBLE;
	}
      else if ((op & 0xf8) == 0xb8 || (op & 0xf8) == 0xd0)
	{
	  edata.cmd = ARM_EXIDX_CMD_VFP_POP;
	  edata.data = 0x80 | (op & 0x07);
	  if ((op & 0xf8) == 0xd0)
	    edata.data |= ARM_EXIDX_VFP_DOUBLE;
	}
      else if (op >= 0xc0 && op <= 0xc5)
	{
	  edata.cmd = ARM_EXIDX_CMD_WREG_POP;
	  edata.data = 0xa0 | (op & 0x07);
	}
      else if (op == 0xc6)
	{
	  edata.cmd = ARM_EXIDX_CMD_WREG_POP;
	  edata.data = READ_OP ();
	}
      else if (op == 0xc7)
	{
	  uint8_t op2 = READ_OP ();
	  if (op2 == 0 || (op2 & 0xf0))
	    edata.cmd = ARM_EXIDX_CMD_RESERVED;
	  else
	    {
	      edata.cmd = ARM_EXIDX_CMD_WCGR_POP;
	      edata.data = op2 & 0x0f;
	    }
	}
      else
	edata.cmd = ARM_EXIDX_CMD_RESERVED;

      ret = arm_exidx_apply_cmd (&edata, c);
      if (ret < 0)
	return ret;
    }
  return 0;
}

static inline uint32_t
prel31_read (uint32_t prel31)
{
  return ((int32_t)prel31 << 1) >> 1;
}

/**
 * Finds the index of the table entry for the given address
 * using a binary search.
 * @returns The table index (or -1 if not found).
 */
HIDDEN int
arm_exidx_lookup (uint32_t *exidx_data, uint32_t exidx_size, uint32_t ip)
{
  unsigned lo = 0, hi = exidx_size / 8;
  while (lo < hi)
    {
      unsigned mid = (lo + hi) / 2;
      uint32_t base = (uint32_t)(exidx_data + mid * 2);
      uint32_t addr = base + prel31_read (exidx_data[mid * 2]);
      if (ip < addr)
	hi = mid;
      else
	lo = mid + 1;
    }
  return hi - 1;
}

HIDDEN int
arm_exidx_entry_lookup (struct elf_image *ei,
		Elf_W (Shdr) *exidx, uint32_t ip)
{
#if 1
  return arm_exidx_lookup (ei->image + exidx->sh_offset, exidx->sh_size, ip);
#else
  unsigned n_entries = exidx->sh_size / 8;
  uint32_t *exidx_data = ei->image + exidx->sh_offset;
  unsigned lo = 0, hi = n_entries;

  while (lo < hi)
    {
      unsigned mid = (lo + hi) / 2;
      uint32_t base = exidx->sh_addr + mid * 8;
      uint32_t addr = base + prel31_read (exidx_data[mid * 2]);
      if (ip < addr)
	hi = mid;
      else
	lo = mid + 1;
    }
  return hi - 1;
#endif
}

HIDDEN int
arm_exidx_extract (struct arm_exidx_entry *entry, uint8_t *buf)
{
  int nbuf = 0;
  uint32_t *addr = prel31_to_addr (&entry->addr);

  uint32_t data = entry->data;
  if (data == ARM_EXIDX_CANT_UNWIND)
    {
      Debug (2, "0x1 [can't unwind]\n");
      nbuf = -UNW_ESTOPUNWIND;
    }
  else if (data & ARM_EXIDX_COMPACT)
    {
      Debug (2, "%p compact model %d [%8.8x]\n", addr, (data >> 24) & 0x7f, data);
      buf[nbuf++] = data >> 16;
      buf[nbuf++] = data >> 8;
      buf[nbuf++] = data;
    }
  else
    {
      uint32_t *extbl_data = prel31_to_addr (&entry->data);
      data = extbl_data[0];
      unsigned int n_table_words = 0;
      if (data & ARM_EXIDX_COMPACT)
	{
	  int pers = (data >> 24) & 0x0f;
	  Debug (2, "%p compact model %d [%8.8x]\n", addr, pers, data);
	  if (pers == 1 || pers == 2)
	    {
	      n_table_words = (data >> 16) & 0xff;
	      extbl_data += 1;
	    }
	  else
	    buf[nbuf++] = data >> 16;
	  buf[nbuf++] = data >> 8;
	  buf[nbuf++] = data;
	}
      else
	{
	  void *pers = prel31_to_addr (extbl_data);
	  Debug (2, "%p Personality routine: %8.8x\n", addr, pers);
	  n_table_words = extbl_data[1] >> 24;
	  buf[nbuf++] = extbl_data[1] >> 16;
	  buf[nbuf++] = extbl_data[1] >> 8;
	  buf[nbuf++] = extbl_data[1];
	  extbl_data += 2;
	}
      assert (n_table_words <= 5);
      unsigned j;
      for (j = 0; j < n_table_words; j++)
	{
	  data = *extbl_data++;
	  buf[nbuf++] = data >> 24;
	  buf[nbuf++] = data >> 16;
	  buf[nbuf++] = data >> 8;
	  buf[nbuf++] = data >> 0;
	}
    }

  if (nbuf > 0 && buf[nbuf - 1] != ARM_EXTBL_OP_FINISH)
    buf[nbuf++] = ARM_EXTBL_OP_FINISH;

  return nbuf;
}

HIDDEN int
arm_exidx_entry_extract (struct elf_image *ei,
		Elf_W (Shdr) *exidx, Elf_W (Shdr) *extbl,
		unsigned i, uint8_t *buf)
{
  int nbuf = 0;
  uint32_t *exidx_data = ei->image + exidx->sh_offset;
  uint32_t base = exidx->sh_addr + i * 8;
  uint32_t one = base + prel31_read (exidx_data[i * 2]);

  uint32_t two = exidx_data[i * 2 + 1];
  if (two == ARM_EXIDX_CANT_UNWIND)
    printf ("0x1 [can't unwind]\n");
  else if (two & ARM_EXIDX_COMPACT)
    {
      printf ("compact model %d [%8.8x]\n", (two >> 24) & 0x7f, two);
      buf[nbuf++] = two >> 16;
      buf[nbuf++] = two >> 8;
      buf[nbuf++] = two;
    }
  else
    {
      assert (NULL != extbl);
      two = base + prel31_read (two) + 4;
      uint32_t *extbl_data = ei->image + extbl->sh_offset
	      + (two - extbl->sh_addr);
      two = extbl_data[0];
      unsigned int n_table_words = 0;
      if (two & ARM_EXIDX_COMPACT)
	{
	  int pers = (two >> 24) & 0x0f;
	  printf ("compact model %d [%8.8x]\n", pers, two);
	  if (pers == 1 || pers == 2)
	    {
	      n_table_words = (two >> 16) & 0xff;
	      extbl_data += 1;
	    }
	  else
	    buf[nbuf++] = two >> 16;
	  buf[nbuf++] = two >> 8;
	  buf[nbuf++] = two;
	}
      else
	{
	  uint32_t pers = prel31_read (extbl_data[0]);
	  pers += extbl->sh_addr + i * 8 + 4;
	  printf ("Personality routine: %8.8x\n", pers);
	  n_table_words = extbl_data[1] >> 24;
	  buf[nbuf++] = extbl_data[1] >> 16;
	  buf[nbuf++] = extbl_data[1] >> 8;
	  buf[nbuf++] = extbl_data[1];
	  extbl_data += 2;
	}
      assert (n_table_words <= 5);
      unsigned j;
      for (j = 0; j < n_table_words; j++)
	{
	  two = *extbl_data++;
	  buf[nbuf++] = two >> 24;
	  buf[nbuf++] = two >> 16;
	  buf[nbuf++] = two >> 8;
	  buf[nbuf++] = two >> 0;
	}
    }

  if (nbuf > 0 && buf[nbuf - 1] != ARM_EXTBL_OP_FINISH)
    buf[nbuf++] = ARM_EXTBL_OP_FINISH;

  return nbuf;
}

HIDDEN void
arm_exidx_section_find (struct elf_image *ei,
		Elf_W (Shdr) **exidx, Elf_W (Shdr) **extbl)
{
  Elf_W (Ehdr) *ehdr = ei->image;
  Elf_W (Shdr) *shdr;
  shdr = (Elf_W (Shdr) *)((char *)ei->image + ehdr->e_shoff);
  char *names = (char *)ei->image + shdr[ehdr->e_shstrndx].sh_offset;
  int i = 0;
  for (i = 0; i < ehdr->e_shnum; i++)
    {
      char *name = names + shdr[i].sh_name;
      switch (shdr[i].sh_type)
	{
	  case SHT_ARM_EXIDX:
	    *exidx = shdr + i;
	    break;
	  case SHT_PROGBITS:
	    if (strcmp (name, ".ARM.extab") != 0)
	      break;
	    *extbl = shdr + i;
	    break;
	}
    }
}

/**
 * Callback to dl_iterate_phr to find each library's unwind table.
 * If found, calls arm_exidx_table_add to remember it for later lookups.
 */
static int
arm_exidx_init_local_cb (struct dl_phdr_info *info, size_t size, void *data)
{
  unsigned i;

  for (i = 0; i < info->dlpi_phnum; i++)
    {
      const ElfW (Phdr) *phdr = info->dlpi_phdr + i;
      if (phdr->p_type != PT_ARM_EXIDX)
	continue;

      const char *name = info->dlpi_name;
      if (NULL == name || 0 == name[0])
	name = arm_exidx_appname;

      ElfW (Addr) addr = info->dlpi_addr + phdr->p_vaddr;
      ElfW (Word) size = phdr->p_filesz;

      arm_exidx_table_add (name,
	  (struct arm_exidx_entry *)addr,
	  (struct arm_exidx_entry *)(addr + size));
      break;
    }
  return 0;
}

/**
 * Must be called for local process lookups.
 */
HIDDEN int
arm_exidx_init_local (const char *appname)
{
  arm_exidx_appname = appname;
  // traverse all libraries and find unwind tables
  arm_exidx_table_reset_all();
  return dl_iterate_phdr (&arm_exidx_init_local_cb, NULL);
}
