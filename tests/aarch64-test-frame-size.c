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
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.  */

/* Unit test for AArch64 get_frame_state SP and frame-record tracking.  */

#include "dwarf.h"
#include "libunwind_i.h"
#include "compiler.h"
#include "unw_test.h"
#include <stdio.h>

int unw_is_signal_frame (unw_cursor_t *cursor UNUSED) { return 0; }
int dwarf_step (struct dwarf_cursor *c UNUSED) { return 0; }

static int procedure_size;
int verbose;

static int
access_mem (unw_addr_space_t as UNUSED, unw_word_t addr, unw_word_t *val,
            int write, void *arg)
{
  if (write != 0)
    return -1;

  if ((void *) addr < arg ||
      (char *) addr > (const char *) arg + procedure_size * sizeof(uint32_t))
    return -1;

  *val = *(const unw_word_t *) addr;
  return 0;
}

static int
get_proc_name (unw_addr_space_t as UNUSED, unw_word_t ip, char *buf UNUSED,
               size_t len UNUSED, unw_word_t *offp, void *arg)
{
  *offp = ip - (unw_word_t) arg;
  return 0;
}

/* Stub that always fails, simulating dladdr lock contention. */
static int
get_proc_name_fail (unw_addr_space_t as UNUSED, unw_word_t ip UNUSED,
                    char *buf UNUSED, size_t len UNUSED,
                    unw_word_t *offp UNUSED, void *arg UNUSED)
{
  return -UNW_ENOINFO;
}

int
main (int argc, char **argv UNUSED)
{
  struct unw_addr_space mock_address_space;
  mock_address_space.acc.access_mem = &access_mem;
  mock_address_space.acc.get_proc_name = &get_proc_name;

  verbose = argc > 1;

  frame_state_t fs;
  unw_cursor_t cursor;
  struct cursor *c = (struct cursor *) &cursor;
  c->dwarf.as = &mock_address_space;

  /* SUB SP, SP, #imm */
  /*
   * Matches vdso __kernel_clock_gettime prologue:
   *   sub sp, sp, #0x20
   *   stp x20, x19, [sp, #16]
   *   ...
   */
  {
    uint32_t instructions[] = {
      0xd10083ff, /* sub sp, sp, #0x20 */
      0xa9014ff4, /* stp x20, x19, [sp, #16] */
      0x71003c1f, /* cmp w0, #0xf */
      0xd503201f, /* nop */
    };
    procedure_size = ARRAY_SIZE (instructions);
    c->dwarf.as_arg = &instructions;

    /* Before SUB: offset should be 0 */
    c->dwarf.ip = (unw_word_t) (instructions);
    fs = get_frame_state(&cursor);
    UNW_TEST_ASSERT (fs.offset == 0, "fs.offset was %d", fs.offset);

    /* After SUB: offset should be 32 */
    c->dwarf.ip = (unw_word_t) (instructions + 1);
    fs = get_frame_state(&cursor);
    UNW_TEST_ASSERT (fs.offset == 32, "fs.offset was %d", fs.offset);

    /* At end: offset should still be 32 */
    c->dwarf.ip = (unw_word_t) (instructions + 3);
    fs = get_frame_state(&cursor);
    UNW_TEST_ASSERT (fs.offset == 32, "fs.offset was %d", fs.offset);
  }

  /* SUB SP, SP, #imm with LSL #12 shift */
  {
    uint32_t instructions[] = {
      0xd14007ff, /* sub sp, sp, #0x1, lsl #12 */
      0xd503201f, /* nop */
    };
    procedure_size = ARRAY_SIZE (instructions);
    c->dwarf.as_arg = &instructions;

    c->dwarf.ip = (unw_word_t) (instructions + 1);
    fs = get_frame_state(&cursor);
    UNW_TEST_ASSERT (fs.offset == 1 << 12, "fs.offset was %d", fs.offset);
  }

  /* STP Xt1, Xt2, [SP, #-imm]! */
  /*
   * Matches a common prologue pattern:
   *   stp x29, x30, [sp, #-48]!
   */
  {
    uint32_t instructions[] = {
      0xa9bd7bfd, /* stp x29, x30, [sp, #-48]! */
      0x910003fd, /* mov x29, sp */
      0xd503201f, /* nop */
    };
    procedure_size = ARRAY_SIZE (instructions);
    c->dwarf.as_arg = &instructions;

    /* Before STP: offset should be 0 */
    c->dwarf.ip = (unw_word_t) (instructions);
    fs = get_frame_state(&cursor);
    UNW_TEST_ASSERT (fs.offset == 0,
		     "before STP, fs.offset was %d", fs.offset);

    /* After STP x29,x30: frame record detected at SP+0. Pre-index
       decrements SP before storing, so the frame record is at the new SP. */
    c->dwarf.ip = (unw_word_t) (instructions + 1);
    fs = get_frame_state(&cursor);
    UNW_TEST_ASSERT (fs.loc == AT_SP_OFFSET, "fs.loc was %d", fs.loc);
    UNW_TEST_ASSERT (fs.offset == 0, "fs.offset was %d", fs.offset);
  }

  /* STP x29, x30, [SP, #-16]! */
  {
    uint32_t instructions[] = {
      0xa9bf7bfd, /* stp x29, x30, [sp, #-16]! */
      0xd503201f, /* nop */
    };
    procedure_size = ARRAY_SIZE (instructions);
    c->dwarf.as_arg = &instructions;

    c->dwarf.ip = (unw_word_t) (instructions + 1);
    fs = get_frame_state(&cursor);
    UNW_TEST_ASSERT (fs.loc == AT_SP_OFFSET, "fs.loc was %d", fs.loc);
  }

  /* STP x20, x19, [SP, #-32]! */
  {
    uint32_t instructions[] = {
      0xa9be4ff4, /* stp x20, x19, [sp, #-32]! */
      0xd503201f, /* nop */
    };
    procedure_size = ARRAY_SIZE (instructions);
    c->dwarf.as_arg = &instructions;

    c->dwarf.ip = (unw_word_t) (instructions + 1);
    fs = get_frame_state(&cursor);
    UNW_TEST_ASSERT (fs.offset == 32, "fs.offset was %d", fs.offset);
  }

  /* STR Xt, [SP, #-imm]! */
  /*
   * Matches vdso __kernel_clock_gettime on some kernels:
   *   str x19, [sp, #-16]!
   */
  {
    uint32_t instructions[] = {
      0xf81f0ff3, /* str x19, [sp, #-16]! */
      0xd503201f, /* nop */
      0xd503201f, /* nop */
    };
    procedure_size = ARRAY_SIZE (instructions);
    c->dwarf.as_arg = &instructions;

    /* Before STR: offset should be 0 */
    c->dwarf.ip = (unw_word_t) (instructions);
    fs = get_frame_state(&cursor);
    UNW_TEST_ASSERT (fs.offset == 0, "fs.offset was %d", fs.offset);

    /* After STR: offset should be 16 */
    c->dwarf.ip = (unw_word_t) (instructions + 1);
    fs = get_frame_state(&cursor);
    UNW_TEST_ASSERT (fs.offset == 16, "fs.offset was %d", fs.offset);
  }

  /* STR x20, [SP, #-32]! */
  {
    uint32_t instructions[] = {
      0xf81e0ff4, /* str x20, [sp, #-32]! */
      0xd503201f, /* nop */
    };
    procedure_size = ARRAY_SIZE (instructions);
    c->dwarf.as_arg = &instructions;

    c->dwarf.ip = (unw_word_t) (instructions + 1);
    fs = get_frame_state(&cursor);
    UNW_TEST_ASSERT (fs.offset == 32, "fs.offset was %d", fs.offset);
  }

  /* SUB SP, SP, #imm
     STP x29, x30, [SP, #-imm]! */
  {
    uint32_t instructions[] = {
      0xd10083ff, /* sub sp, sp, #0x20 */
      0xa9bf7bfd, /* stp x29, x30, [sp, #-16]! */
      0xd503201f, /* nop */
    };
    procedure_size = ARRAY_SIZE (instructions);
    c->dwarf.as_arg = &instructions;

    c->dwarf.ip = (unw_word_t) (instructions + 2);
    fs = get_frame_state(&cursor);
    UNW_TEST_ASSERT (fs.loc == AT_SP_OFFSET, "fs.loc was %d", fs.loc);
    UNW_TEST_ASSERT (fs.offset == 0, "fs.offset was %d", fs.offset);
  }

  /* ADD SP, SP, #imm */
  {
    uint32_t instructions[] = {
      0xd10083ff, /* sub sp, sp, #0x20 */
      0xd503201f, /* nop */
      0x910083ff, /* add sp, sp, #0x20 */
      0xd65f03c0, /* ret */
    };
    procedure_size = ARRAY_SIZE (instructions);
    c->dwarf.as_arg = &instructions;

    c->dwarf.ip = (unw_word_t) (instructions + 2);
    fs = get_frame_state(&cursor);
    UNW_TEST_ASSERT (fs.offset == 32, "fs.offset was %d", fs.offset);

    c->dwarf.ip = (unw_word_t) (instructions + 3);
    fs = get_frame_state(&cursor);
    UNW_TEST_ASSERT (fs.offset == 0, "fs.offset was %d", fs.offset);
  }

  /* LDP x29, x30, [SP], #imm */
  {
    uint32_t instructions[] = {
      0xa9bd7bfd, /* stp x29, x30, [sp, #-48]! */
      0x910003fd, /* mov x29, sp */
      0xd503201f, /* nop */
      0xa8c37bfd, /* ldp x29, x30, [sp], #48 */
      0xd65f03c0, /* ret */
    };
    procedure_size = ARRAY_SIZE (instructions);

    c->dwarf.as_arg = &instructions;

    /* Before LDP: STP+MOV detected, so loc=AT_FP.  SP tracking offset
       is not observable via offset when the frame-record path is active. */
    c->dwarf.ip = (unw_word_t) (instructions + 3);
    fs = get_frame_state(&cursor);
    UNW_TEST_ASSERT (fs.loc == AT_FP, "fs.loc was %d", fs.loc);

    c->dwarf.ip = (unw_word_t) (instructions + 4);
    fs = get_frame_state(&cursor);
    UNW_TEST_ASSERT (fs.offset == 0, "fs.offset was %d", fs.offset);
  }

  /* SUB SP, SP, #imm
     B.EQ */
  {
    uint32_t instructions[] = {
      0xd10083ff, /* sub sp, sp, #0x20 */
      0x54000000, /* b.eq */
      0xd503201f, /* nop */
    };
    procedure_size = ARRAY_SIZE (instructions);
    c->dwarf.as_arg = &instructions;

    c->dwarf.ip = (unw_word_t) (instructions + 2);
    fs = get_frame_state(&cursor);
    /* sp adjustment should not be affected by the branch */
    UNW_TEST_ASSERT (fs.offset == 32, "fs.offset was %d", fs.offset);
  }

  /* MOV SP, x29 */
  {
    uint32_t instructions[] = {
      0xd10083ff, /* sub sp, sp, #0x20 */
      0x910003bf, /* mov sp, x29 */
      0xd503201f, /* nop */
    };
    procedure_size = ARRAY_SIZE (instructions);
    c->dwarf.as_arg = &instructions;

    c->dwarf.ip = (unw_word_t) (instructions + 2);
    fs = get_frame_state(&cursor);
    /* sp adjustment should be invalidated by mov sp, x29 */
    UNW_TEST_ASSERT (fs.loc == NONE, "fs.loc was %d", fs.loc);
    UNW_TEST_ASSERT (fs.offset == 0, "fs.offset was %d", fs.offset);
  }

  /* ADD SP, x29, #imm */
  {
    uint32_t instructions[] = {
      0xd10083ff, /* sub sp, sp, #0x20 */
      0x910043bf, /* add sp, x29, #0x10 */
      0xd503201f, /* nop */
    };
    procedure_size = ARRAY_SIZE (instructions);
    c->dwarf.as_arg = &instructions;

    c->dwarf.ip = (unw_word_t) (instructions + 2);
    fs = get_frame_state(&cursor);
    UNW_TEST_ASSERT (fs.loc == NONE, "fs.loc was %d", fs.loc);
    UNW_TEST_ASSERT (fs.offset == 0, "fs.offset was %d", fs.offset);
  }

  /* SUB SP, x29, #imm */
  {
    uint32_t instructions[] = {
      0xd10083ff, /* sub sp, sp, #0x20 */
      0xd14043bf, /* sub sp, x29, #0x10 */
      0xd503201f, /* nop */
    };
    procedure_size = ARRAY_SIZE (instructions);
    c->dwarf.as_arg = &instructions;

    c->dwarf.ip = (unw_word_t) (instructions + 2);
    fs = get_frame_state(&cursor);
    UNW_TEST_ASSERT (fs.loc == NONE, "fs.loc was %d", fs.loc);
    UNW_TEST_ASSERT (fs.offset == 0, "fs.offset was %d", fs.offset);
  }

  /* get_proc_name failure */
  {
    mock_address_space.acc.get_proc_name = &get_proc_name_fail;

    uint32_t instructions[] = {
      0xd10083ff, /* sub sp, sp, #0x20 */
      0xd503201f, /* nop */
    };
    procedure_size = ARRAY_SIZE (instructions);
    c->dwarf.as_arg = &instructions;
    c->dwarf.ip = (unw_word_t) (instructions + 1);
    fs = get_frame_state(&cursor);
    UNW_TEST_ASSERT (fs.offset == 0, "fs.offset was %d", fs.offset);

    /* Restore for subsequent tests */
    mock_address_space.acc.get_proc_name = &get_proc_name;
  }

  /* Leaf function */
  {
    uint32_t instructions[] = {
      0xb0001641, /* adrp x1, 0x2c9000 */
      0x90000d00, /* adrp x0, 0x1a0000 */
      0xd65f03c0, /* ret */
    };
    procedure_size = ARRAY_SIZE (instructions);
    c->dwarf.as_arg = &instructions;

    c->dwarf.ip = (unw_word_t) (instructions + 2);
    fs = get_frame_state(&cursor);
    /* Nothing affects SP or FP */
    UNW_TEST_ASSERT (fs.offset == 0, "fs.offset was %d", fs.offset);
  }

  /* BL after SUB SP, SP, #imm */
  {
    uint32_t instructions[] = {
      0xd10083ff, /* sub sp, sp, #0x20 */
      0x94000000, /* bl <label> */
      0xd503201f, /* nop */
    };
    procedure_size = ARRAY_SIZE (instructions);
    c->dwarf.as_arg = &instructions;

    c->dwarf.ip = (unw_word_t) (instructions + 2);
    fs = get_frame_state(&cursor);
    /* should be unaffected by BL */
    UNW_TEST_ASSERT (fs.offset == 32, "fs.offset was %d", fs.offset);
  }

  /* CBZ after SUB SP, SP, #imm */
  {
    uint32_t instructions[] = {
      0xd10083ff, /* sub sp, sp, #0x20 */
      0xb4000000, /* cbz x0, <label> */
      0xd503201f, /* nop */
    };
    procedure_size = ARRAY_SIZE (instructions);
    c->dwarf.as_arg = &instructions;

    c->dwarf.ip = (unw_word_t) (instructions + 2);
    fs = get_frame_state(&cursor);
    UNW_TEST_ASSERT (fs.offset == 32, "fs.offset was %d", fs.offset);
  }

  /* TBNZ after SUB SP, SP, #imm */
  {
    uint32_t instructions[] = {
      0xd10083ff, /* sub sp, sp, #0x20 */
      0x37000000, /* tbnz w0, #0, <label> */
      0xd503201f, /* nop */
    };
    procedure_size = ARRAY_SIZE (instructions);
    c->dwarf.as_arg = &instructions;

    c->dwarf.ip = (unw_word_t) (instructions + 2);
    fs = get_frame_state(&cursor);
    UNW_TEST_ASSERT (fs.offset == 32, "fs.offset was %d", fs.offset);
  }

  /* B.cond before SUB SP, SP, #imm */
  {
    uint32_t instructions[] = {
      0x54000000, /* b.eq <label> */
      0xd10083ff, /* sub sp, sp, #0x20 */
      0xd503201f, /* nop */
    };
    procedure_size = ARRAY_SIZE (instructions);
    c->dwarf.as_arg = &instructions;

    c->dwarf.ip = (unw_word_t) (instructions + 2);
    fs = get_frame_state(&cursor);
    UNW_TEST_ASSERT (fs.offset == 0, "fs.offset was %d", fs.offset);
  }

  /* SUB SP, SP, Xm */
  {
    uint32_t instructions[] = {
      0xcb2263ff, /* sub sp, sp, x2, uxtx */
      0xd503201f, /* nop */
    };
    procedure_size = ARRAY_SIZE (instructions);
    c->dwarf.as_arg = &instructions;

    c->dwarf.ip = (unw_word_t) (instructions + 1);
    fs = get_frame_state(&cursor);
    UNW_TEST_ASSERT (fs.offset == 0, "fs.offset was %d", fs.offset);
  }

  /* ADD SP, SP, Xm */
  {
    uint32_t instructions[] = {
      0xd10083ff, /* sub sp, sp, #0x20 */
      0x8b2263ff, /* add sp, sp, x2, uxtx */
      0xd503201f, /* nop */
    };
    procedure_size = ARRAY_SIZE (instructions);
    c->dwarf.as_arg = &instructions;

    c->dwarf.ip = (unw_word_t) (instructions + 2);
    fs = get_frame_state(&cursor);
    UNW_TEST_ASSERT (fs.offset == 0, "fs.offset was %d", fs.offset);
  }

  /* SUB SP, SP, #imm followed by STP x29, x30, [SP, #-imm]! and LDP */
  {
    uint32_t instructions[] = {
      0xd10083ff, /* sub sp, sp, #0x20 */
      0xa9bf7bfd, /* stp x29, x30, [sp, #-16]! */
      0xa8c37bfd, /* ldp x29, x30, [sp], #48 */
      0xd503201f, /* nop */
    };
    procedure_size = ARRAY_SIZE (instructions);
    c->dwarf.as_arg = &instructions;

    c->dwarf.ip = (unw_word_t) (instructions + 3);
    fs = get_frame_state(&cursor);
    UNW_TEST_ASSERT (fs.loc == AT_SP_OFFSET, "fs.loc was %d", fs.loc);
    UNW_TEST_ASSERT (fs.offset == 0, "fs.offset was %d", fs.offset);
  }

  if (verbose)
    printf ("SUCCESS\n");

  return UNW_TEST_EXIT_PASS;
}
