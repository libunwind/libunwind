/**
 * Unittest PPC64 is_plt_entry function by inspecting output at
 * different points in a mock PLT address space.
 */
/*
 * This file is part of libunwind.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
#include "dwarf.h"
#include "libunwind_i.h"

#undef unw_get_accessors_int
unw_accessors_t *unw_get_accessors_int (unw_addr_space_t unused) { return NULL; }
int dwarf_step (struct dwarf_cursor* unused) { return 0; }

/* Bounds for the mock memory buffer.  Set by each test before invoking
   unw_is_plt_entry().  */
static const void *mock_mem_start;
static const void *mock_mem_end;

/* Mock access_mem implementation */
static int
access_mem (unw_addr_space_t as, unw_word_t addr, unw_word_t *val, int write,
            void *arg)
{
  if (write != 0)
    return -1;

  const unw_word_t *paddr = (const unw_word_t*) addr;

  if ((void*) paddr < mock_mem_start || (void*) paddr > mock_mem_end)
    {
      return -1;
    }

  *val = *paddr;
  return 0;
}

static int
test_elfv2_le (void)
{
  enum
  {
    ip_guard0,
    ip_std,
    ip_ld,
    ip_mtctr,
    ip_bctr,
    ip_guard1,

    ip_program_end
  };

  const uint32_t plt_instructions[ip_program_end] =
    {
      0xdeadbeef,
      0xf8410018, // std     r2,24(r1)
      0xe9828730, // ld      r12,-30928(r2)
      0x7d8903a6, // mtctr   r12
      0x4e800420, // bctr
      0xdeadbeef,
    };
  uint32_t test_instructions[ip_program_end];
  memcpy(test_instructions, plt_instructions, sizeof(test_instructions));

  mock_mem_start = test_instructions;
  mock_mem_end = (const char*) test_instructions + sizeof(test_instructions);

  struct unw_addr_space mock_address_space;
  mock_address_space.big_endian = 0;
  mock_address_space.acc.access_mem = &access_mem;

  struct cursor cursor;
  struct dwarf_cursor *dwarf = &cursor.dwarf;
  struct unw_cursor *c = (struct unw_cursor *)(&cursor);
  dwarf->as = &mock_address_space;
  dwarf->as_arg = &test_instructions;

  /* ip at std r2,24(r1) */
  dwarf->ip = (unw_word_t) (test_instructions + ip_std);
  if (!unw_is_plt_entry(c)) return -1;

  /* ld uses a different offset */
  test_instructions[ip_ld] = 0xe9820000;
  if (!unw_is_plt_entry(c)) return -1;
  memcpy(test_instructions, plt_instructions, sizeof(test_instructions));

  /* ip_ld is not a ld instruction */
  test_instructions[ip_ld] = 0xf154f00d;
  if (unw_is_plt_entry(c)) return -1;
  memcpy(test_instructions, plt_instructions, sizeof(test_instructions));

  /* ip_mtctr is not a mtctr instruction */
  test_instructions[ip_mtctr] = 0xf154f00d;
  if (unw_is_plt_entry(c)) return -1;
  memcpy(test_instructions, plt_instructions, sizeof(test_instructions));

  /* ip_bctr is not a bctr instruction */
  test_instructions[ip_bctr] = 0xf154f00d;
  if (unw_is_plt_entry(c)) return -1;
  memcpy(test_instructions, plt_instructions, sizeof(test_instructions));

  /* ip at ld r12,-30928(r2) */
  dwarf->ip = (unw_word_t) (test_instructions + ip_ld);
  if (!unw_is_plt_entry(c)) return -1;

  /* ip_std is not a std instruction */
  test_instructions[ip_std] = 0xf154f00d;
  if (unw_is_plt_entry(c)) return -1;
  memcpy(test_instructions, plt_instructions, sizeof(test_instructions));

  /* ip_mtctr is not a mtctr instruction */
  test_instructions[ip_mtctr] = 0xf154f00d;
  if (unw_is_plt_entry(c)) return -1;
  memcpy(test_instructions, plt_instructions, sizeof(test_instructions));

  /* ip_bctr is not a bctr instruction */
  test_instructions[ip_bctr] = 0xf154f00d;
  if (unw_is_plt_entry(c)) return -1;
  memcpy(test_instructions, plt_instructions, sizeof(test_instructions));

  /* ip at mtctr r12 */
  dwarf->ip = (unw_word_t) (test_instructions + ip_mtctr);
  if (!unw_is_plt_entry(c)) return -1;

  /* ip_std is not a std instruction */
  test_instructions[ip_std] = 0xf154f00d;
  if (unw_is_plt_entry(c)) return -1;
  memcpy(test_instructions, plt_instructions, sizeof(test_instructions));

  /* ip_ld is not a ld instruction */
  test_instructions[ip_ld] = 0xf154f00d;
  if (unw_is_plt_entry(c)) return -1;
  memcpy(test_instructions, plt_instructions, sizeof(test_instructions));

  /* ip_bctr is not a bctr instruction */
  test_instructions[ip_bctr] = 0xf154f00d;
  if (unw_is_plt_entry(c)) return -1;
  memcpy(test_instructions, plt_instructions, sizeof(test_instructions));

  /* ip at bctr */
  dwarf->ip = (unw_word_t) (test_instructions + ip_bctr);
  if (!unw_is_plt_entry(c)) return -1;

  /* ip_std is not a std instruction */
  test_instructions[ip_std] = 0xf154f00d;
  if (unw_is_plt_entry(c)) return -1;
  memcpy(test_instructions, plt_instructions, sizeof(test_instructions));

  /* ip_ld is not a ld instruction */
  test_instructions[ip_ld] = 0xf154f00d;
  if (unw_is_plt_entry(c)) return -1;
  memcpy(test_instructions, plt_instructions, sizeof(test_instructions));

  /* ip_mtctr is not a mtctr instruction */
  test_instructions[ip_mtctr] = 0xf154f00d;
  if (unw_is_plt_entry(c)) return -1;
  memcpy(test_instructions, plt_instructions, sizeof(test_instructions));

  /* ip at non-PLT instruction */
  dwarf->ip = (unw_word_t) (test_instructions + ip_guard0);
  if (unw_is_plt_entry(c)) return -1;

  /* ip at another non-PLT instruction */
  dwarf->ip = (unw_word_t) (test_instructions + ip_guard1);
  if (unw_is_plt_entry(c)) return -1;

  return 0;
}

static int
test_elfv1_be (void)
{
  enum
  {
    ip_guard0,
    ip_std,
    ip_addis,
    ip_ld12,
    ip_mtctr,
    ip_ld2,
    ip_bctr,
    ip_guard1,

    ip_program_end
  };

  /* The new BE ELFv1 detector reads via _read_instruction(), which assumes
     8-byte aligned access via the accessor.  Force the buffers to 8-byte
     alignment so the mock access_mem() can dereference unw_word_t* directly.  */
  const uint32_t plt_instructions[ip_program_end] __attribute__((aligned(8))) =
    {
      0xdeadbeef,
      0xf8410028, // std   r2,40(r1)
      0x3d624000, // addis r11,r2,0x4000
      0xe98b8000, // ld    r12,-32768(r11)
      0x7d8903a6, // mtctr r12
      0xe84b8008, // ld    r2,-32760(r11)
      0x4e800420, // bctr
      0xdeadbeef,
    };
  uint32_t test_instructions[ip_program_end] __attribute__((aligned(8)));
  memcpy(test_instructions, plt_instructions, sizeof(test_instructions));

  mock_mem_start = test_instructions;
  mock_mem_end = (const char*) test_instructions + sizeof(test_instructions);

  struct unw_addr_space mock_address_space;
  mock_address_space.big_endian = 1;
  mock_address_space.acc.access_mem = &access_mem;

  struct cursor cursor;
  struct dwarf_cursor *dwarf = &cursor.dwarf;
  struct unw_cursor *c = (struct unw_cursor *)(&cursor);
  dwarf->as = &mock_address_space;
  dwarf->as_arg = &test_instructions;

  /* IP at every position within the 6-instruction stub must be detected. */
  for (int ip = ip_std; ip <= ip_bctr; ++ip)
    {
      dwarf->ip = (unw_word_t) (test_instructions + ip);
      if (!unw_is_plt_entry(c)) return -1;
    }

  /* Breaking any one of the six pattern slots must defeat detection,
     regardless of which slot the IP currently points at. */
  const int stub_slots[] =
    { ip_std, ip_addis, ip_ld12, ip_mtctr, ip_ld2, ip_bctr };
  const size_t n_slots = sizeof(stub_slots) / sizeof(stub_slots[0]);
  for (size_t k = 0; k < n_slots; ++k)
    {
      for (int ip = ip_std; ip <= ip_bctr; ++ip)
        {
          memcpy(test_instructions, plt_instructions, sizeof(test_instructions));
          test_instructions[stub_slots[k]] = 0xf154f00d;
          dwarf->ip = (unw_word_t) (test_instructions + ip);
          if (unw_is_plt_entry(c)) return -1;
        }
    }
  memcpy(test_instructions, plt_instructions, sizeof(test_instructions));

  /* IPs at the surrounding guard words must not be detected as PLT. */
  dwarf->ip = (unw_word_t) (test_instructions + ip_guard0);
  if (unw_is_plt_entry(c)) return -1;

  dwarf->ip = (unw_word_t) (test_instructions + ip_guard1);
  if (unw_is_plt_entry(c)) return -1;

  return 0;
}

int
main ()
{
  /* The mock access_mem() returns *(unw_word_t *)addr, i.e. a host-endian
     read.  This naturally matches a target whose endianness equals the
     host's, but mismatches the other endianness.  So pick the test that
     matches the host: BE ELFv1 on BE hosts, LE ELFv2 on LE hosts. */
  if (target_is_big_endian())
    return test_elfv1_be();
  return test_elfv2_le();
}
