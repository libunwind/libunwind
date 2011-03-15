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

static inline int
arm_exidx_frame_reg(void *pc)
{
  return ((unsigned)pc & 1) ? FP_thumb : FP_arm;
}

HIDDEN void
arm_exidx_frame_to_vrs(struct arm_stackframe *f, struct arm_exidx_vrs *s)
{
  int fp_reg = arm_exidx_frame_reg(f->pc);
  s->vrs[fp_reg] = (uint32_t)f->fp;
  s->vrs[SP] = (uint32_t)f->sp;
  s->vrs[LR] = (uint32_t)f->lr;
  s->vrs[PC] = 0;
}

HIDDEN int
arm_exidx_vrs_to_frame(struct arm_exidx_vrs *s, struct arm_stackframe *f)
{
  if (s->vrs[PC] == 0)
    s->vrs[PC] = s->vrs[LR];

  if (f->pc == (void *)s->vrs[PC])
    return -1;

  int fp_reg = arm_exidx_frame_reg(f->pc);
  f->fp = (void *)s->vrs[fp_reg];
  f->sp = (void *)s->vrs[SP];
  f->lr = (void *)s->vrs[LR];
  f->pc = (void *)s->vrs[PC];

  return 0;
}

HIDDEN int
arm_exidx_vrs_callback (struct arm_exbuf_callback_data *aecd)
{
  struct arm_exidx_vrs *s = aecd->cb_data;
  int ret = 0, i;
  switch (aecd->cmd)
    {
      case ARM_EXIDX_CMD_FINISH:
	break;
      case ARM_EXIDX_CMD_DATA_PUSH:
	Debug (2, "vsp = vsp - %d\n", aecd->data);
	s->vrs[SP] -= aecd->data;
	break;
      case ARM_EXIDX_CMD_DATA_POP:
	Debug (2, "vsp = vsp + %d\n", aecd->data);
	s->vrs[SP] += aecd->data;
	break;
      case ARM_EXIDX_CMD_REG_POP:
	for (i = 0; i < 16; i++)
	  if (aecd->data & (1 << i))
	    {
	      s->vrs[i] = *(uint32_t*)s->vrs[SP];
	      s->vrs[SP] += 4;
	      Debug (2, "pop {r%d}\n", i);
	    }
	break;
      case ARM_EXIDX_CMD_REG_TO_SP:
	assert (aecd->data < 16);
	Debug (2, "vsp = r%d\n", aecd->data);
	s->vrs[SP] = s->vrs[aecd->data];
	break;
      case ARM_EXIDX_CMD_VFP_POP:
	/* Skip VFP registers, but be sure to adjust stack */
	for (i = ARM_EXBUF_START (aecd->data); i < ARM_EXBUF_END (aecd->data); i++)
	  s->vrs[SP] += 8;
	if (!(aecd->data & ARM_EXIDX_VFP_DOUBLE))
	  s->vrs[SP] += 4;
	break;
      case ARM_EXIDX_CMD_WREG_POP:
	for (i = ARM_EXBUF_START (aecd->data); i < ARM_EXBUF_END (aecd->data); i++)
	  s->vrs[SP] += 8;
	break;
      case ARM_EXIDX_CMD_WCGR_POP:
	for (i = 0; i < 4; i++)
	  if (aecd->data & (1 << i))
	    s->vrs[SP] += 4;
	break;
      case ARM_EXIDX_CMD_REFUSED:
      case ARM_EXIDX_CMD_RESERVED:
	ret = -1;
	break;
    }
  return ret;
}

HIDDEN int
arm_exidx_decode (const uint8_t *buf, uint8_t len,
		arm_exbuf_callback_t cb, void *cb_data)
{
#define READ_OP() aecb.ops[aecb.n_ops++] = *buf++
  const uint8_t *end = buf + len;
  int ret;

  assert(buf != NULL);
  assert(cb != NULL);
  while (buf < end)
    {
      struct arm_exbuf_callback_data aecb = { .cb_data = cb_data };
      uint8_t op = READ_OP ();
      if ((op & 0xc0) == 0x00)
	{
	  aecb.cmd = ARM_EXIDX_CMD_DATA_POP;
	  aecb.data = (((int)op & 0x3f) << 2) + 4;
	}
      else if ((op & 0xc0) == 0x40)
	{
	  aecb.cmd = ARM_EXIDX_CMD_DATA_PUSH;
	  aecb.data = (((int)op & 0x3f) << 2) + 4;
	}
      else if ((op & 0xf0) == 0x80)
	{
	  uint8_t op2 = READ_OP ();
	  if (op == 0x80 && op2 == 0x00)
	    aecb.cmd = ARM_EXIDX_CMD_REFUSED;
	  else
	    {
	      aecb.cmd = ARM_EXIDX_CMD_REG_POP;
	      aecb.data = ((op & 0xf) << 8) | op2;
	      aecb.data = aecb.data << 4;
	    }
	}
      else if ((op & 0xf0) == 0x90)
	{
	  if (op == 0x9d || op == 0x9f)
	    aecb.cmd = ARM_EXIDX_CMD_RESERVED;
	  else
	    {
	      aecb.cmd = ARM_EXIDX_CMD_REG_TO_SP;
	      aecb.data = op & 0x0f;
	    }
	}
      else if ((op & 0xf0) == 0xa0)
	{
	  unsigned end = (op & 0x07);
	  aecb.data = (1 << (end + 1)) - 1;
	  aecb.data = aecb.data << 4;
	  if (op & 0x08)
	    aecb.data |= 1 << 14;
	  aecb.cmd = ARM_EXIDX_CMD_REG_POP;
	}
      else if (op == ARM_EXTBL_OP_FINISH)
	{
	  aecb.cmd = ARM_EXIDX_CMD_FINISH;
	  buf = end;
	}
      else if (op == 0xb1)
	{
	  uint8_t op2 = READ_OP ();
	  if (op2 == 0 || (op2 & 0xf0))
	    aecb.cmd = ARM_EXIDX_CMD_RESERVED;
	  else
	    {
	      aecb.cmd = ARM_EXIDX_CMD_REG_POP;
	      aecb.data = op2 & 0x0f;
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
	  aecb.data = offset * 4 + 0x204;
	  aecb.cmd = ARM_EXIDX_CMD_DATA_POP;
	}
      else if (op == 0xb3 || op == 0xc8 || op == 0xc9)
	{
	  aecb.cmd = ARM_EXIDX_CMD_VFP_POP;
	  aecb.data = READ_OP ();
	  if (op == 0xc8)
	    aecb.data |= ARM_EXIDX_VFP_SHIFT_16;
	  if (op != 0xb3)
	    aecb.data |= ARM_EXIDX_VFP_DOUBLE;
	}
      else if ((op & 0xf8) == 0xb8 || (op & 0xf8) == 0xd0)
	{
	  aecb.cmd = ARM_EXIDX_CMD_VFP_POP;
	  aecb.data = 0x80 | (op & 0x07);
	  if ((op & 0xf8) == 0xd0)
	    aecb.data |= ARM_EXIDX_VFP_DOUBLE;
	}
      else if (op >= 0xc0 && op <= 0xc5)
	{
	  aecb.cmd = ARM_EXIDX_CMD_WREG_POP;
	  aecb.data = 0xa0 | (op & 0x07);
	}
      else if (op == 0xc6)
	{
	  aecb.cmd = ARM_EXIDX_CMD_WREG_POP;
	  aecb.data = READ_OP ();
	}
      else if (op == 0xc7)
	{
	  uint8_t op2 = READ_OP ();
	  if (op2 == 0 || (op2 & 0xf0))
	    aecb.cmd = ARM_EXIDX_CMD_RESERVED;
	  else
	    {
	      aecb.cmd = ARM_EXIDX_CMD_WCGR_POP;
	      aecb.data = op2 & 0x0f;
	    }
	}
      else
	aecb.cmd = ARM_EXIDX_CMD_RESERVED;

      ret = (*cb) (&aecb);
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
