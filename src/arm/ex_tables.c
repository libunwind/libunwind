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

#define ARM_EXBUF_START(x)	(((x) >> 4) & 0x0f)
#define ARM_EXBUF_COUNT(x)	((x) & 0x0f)
#define ARM_EXBUF_END(x)	(ARM_EXBUF_START(x) + ARM_EXBUF_COUNT(x))

#define ARM_EXIDX_CANT_UNWIND	0x00000001
#define ARM_EXIDX_COMPACT	0x80000000

#define ARM_EXTBL_OP_FINISH	0xb0

enum arm_exbuf_cmd_flags {
  ARM_EXIDX_VFP_SHIFT_16 = 1 << 16,
  ARM_EXIDX_VFP_DOUBLE = 1 << 17,
};

#ifdef ARM_EXIDX_TABLE_MALLOC
static struct arm_exidx_table *arm_exidx_table_list;
#else
#define ARM_EXIDX_TABLE_LIMIT 32
static struct arm_exidx_table arm_exidx_tables[ARM_EXIDX_TABLE_LIMIT];
static unsigned arm_exidx_table_count = 0;
#endif

static inline uint32_t
prel31_read (uint32_t prel31)
{
  return ((int32_t)prel31 << 1) >> 1;
}

static inline void *
prel31_to_addr (void *addr)
{
  uint32_t offset = ((long)*(uint32_t *)addr << 1) >> 1;
  return (char *)addr + offset;
}

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
 * Locate the appropriate unwind table from the given PC.
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

/**
 * Finds the corresponding arm_exidx_entry from a given index table and PC.
 */
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

/**
 * Decodes the given unwind instructions into arm_exbuf_data and calls
 * arm_exidx_apply_cmd that applies the command onto the dwarf_cursor.
 */
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

/**
 * Reads the given entry and extracts the unwind instructions into buf.
 * Returns the number of the extracted unwind insns or -UNW_ESTOPUNWIND
 * if the special bit pattern ARM_EXIDX_CANT_UNWIND (0x1) was found.
 */
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
	  Debug (2, "%p Personality routine: %8p\n", addr, pers);
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

/**
 * Callback to dl_iterate_phdr to find the unwind tables.
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

      ElfW (Addr) addr = info->dlpi_addr + phdr->p_vaddr;
      ElfW (Word) size = phdr->p_filesz;

      arm_exidx_table_add (info->dlpi_name,
	  (struct arm_exidx_entry *)addr,
	  (struct arm_exidx_entry *)(addr + size));
      break;
    }
  return 0;
}

/**
 * Traverse the program headers of the executable and its loaded
 * shared objects to collect the unwind tables.
 */
HIDDEN int
arm_exidx_init_local (void)
{
  arm_exidx_table_reset_all();
  return dl_iterate_phdr (&arm_exidx_init_local_cb, NULL);
}
