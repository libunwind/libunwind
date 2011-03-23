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

#ifndef ARM_EX_TABLES_H
#define ARM_EX_TABLES_H

struct arm_exidx_entry {
  uint32_t addr;
  uint32_t data;
};

struct arm_exidx_table {
  const char *name;
  struct arm_exidx_entry *start;
  struct arm_exidx_entry *end;
  void *start_addr;
  void *end_addr;
};

typedef enum arm_exbuf_cmd {
  ARM_EXIDX_CMD_FINISH,
  ARM_EXIDX_CMD_DATA_PUSH,
  ARM_EXIDX_CMD_DATA_POP,
  ARM_EXIDX_CMD_REG_POP,
  ARM_EXIDX_CMD_REG_TO_SP,
  ARM_EXIDX_CMD_VFP_POP,
  ARM_EXIDX_CMD_WREG_POP,
  ARM_EXIDX_CMD_WCGR_POP,
  ARM_EXIDX_CMD_RESERVED,
  ARM_EXIDX_CMD_REFUSED,
} arm_exbuf_cmd_t;

struct arm_exbuf_data
{
  arm_exbuf_cmd_t cmd;
  uint32_t data;
};


int arm_exidx_init_local (void);
int arm_exidx_table_add (const char *name, struct arm_exidx_entry *start,
			 struct arm_exidx_entry *end);
struct arm_exidx_table *arm_exidx_table_find (void *pc);
struct arm_exidx_entry *arm_exidx_table_lookup (struct arm_exidx_table *table,
						void *pc);
int arm_exidx_extract (struct arm_exidx_entry *entry, uint8_t *buf);
int arm_exidx_decode (const uint8_t *buf, uint8_t len, struct dwarf_cursor *c);
int arm_exidx_apply_cmd (struct arm_exbuf_data *edata, struct dwarf_cursor *c);

#endif // ARM_EX_TABLES_H
