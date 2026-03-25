#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "unw_test.h"

#include "elf32.h"
#include "elfxx_i.h"

int
main (void)
{
  {
    Elf32_Word hash[] = { 17, 42 };
    Elf32_Word sym_num = 0;
    int ret = _Uelf32_dynamic_symtab_count (hash, NULL, &sym_num);

    UNW_TEST_ASSERT (ret == 0,
		     "Symbol count with SYSV only failed with return %d",
		     ret);
    UNW_TEST_ASSERT (sym_num == 42, "sym_num expected 42 got %u", sym_num);
  }

  {
    uint32_t gnu_hash[] = {
      2,  /* nbuckets */
      9,  /* symndx */
      1,  /* maskwords */
      6,  /* shift2 */
      0x00003000,  /* filter word */
      0, 9,  /* buckets */
      0xf783da5b,  /* end */
    };
    Elf32_Word sym_num = 0;
    int ret = _Uelf32_dynamic_symtab_count (NULL, gnu_hash, &sym_num);

    UNW_TEST_ASSERT (ret == 0,
		     "Symbol count with GNU only failed with return %d",
		     ret);
    UNW_TEST_ASSERT (sym_num == 10, "sym_num changed, expected 10 got %u", sym_num);
  }

  {
    Elf32_Word hash[] = { 1, 7 };
    uint32_t gnu_hash[] = {
      2,  /* nbuckets */
      9,  /* symndx */
      1,  /* maskwords */
      6,  /* shift2 */
      0x00003000,  /* filter word */
      0, 9,  /* buckets */
      0xf783da5b,  /* end */
    };
    Elf32_Word sym_num = 0;
    int ret = _Uelf32_dynamic_symtab_count (hash, gnu_hash, &sym_num);

    UNW_TEST_ASSERT (ret == 0,
		     "Symbol count with both SYSV and GNU failed with return %d",
		     ret);
    UNW_TEST_ASSERT (sym_num == 7, "prefer_sysv expected 7 got %u", sym_num);
  }

  {
    /* On ELF32, bloom words are 32 bits, so maskwords=1 means one
       uint32_t.  Buckets start right after that single word. */
    uint32_t gnu_hash[] = {
      1,  /* nbuckets */
      5,  /* symndx */
      1,  /* maskwords */
      6,  /* shift2 */
      0x00000000,  /* filter word */
      5,  /* buckets */
      0x00000001,  /* end */
    };
    Elf32_Word sym_num = 0;
    int ret = _Uelf32_dynamic_symtab_count (NULL, gnu_hash, &sym_num);

    UNW_TEST_ASSERT (ret == 0,
		     "Symbol count with ELF32 bloom layout failed with return %d",
		     ret);
    UNW_TEST_ASSERT (sym_num == 6, "sym_num expected 6 got %u", sym_num);
  }

  {
    /* Non-empty buckets must be non-decreasing in a well-formed GNU hash
       table.  Reject descending bucket values as corrupt. */
    uint32_t gnu_hash[] = {
      2,  /* nbuckets */
      5,  /* symndx */
      1,  /* maskwords */
      6,  /* shift2 */
      0x00000000,  /* filter word */
      8, 5,  /* buckets (descending) */
      0x00000001, 0x00000001, 0x00000001, 0x00000001,
    };
    Elf32_Word sym_num = 1234;
    int ret = _Uelf32_dynamic_symtab_count (NULL, gnu_hash, &sym_num);

    UNW_TEST_ASSERT (ret == -UNW_ENOINFO,
		     "Symbol count with descending buckets should have failed, returned %d",
		     ret);
    UNW_TEST_ASSERT (sym_num == 1234,
		     "sym_num should be unchanged, expected 1234 got %u",
		     sym_num);
  }

  {
    uint32_t gnu_hash[] = {
      1,  /* nbuckets */
      5,  /* symndx */
      1,  /* maskwords */
      6,  /* shift2 */
      0x00000000,  /* filter word */
      4,  /* buckets */
      0x00000001,  /* end */
    };
    Elf32_Word sym_num = 1234;
    int ret = _Uelf32_dynamic_symtab_count (NULL, gnu_hash, &sym_num);

    UNW_TEST_ASSERT (ret == -UNW_ENOINFO,
		     "Symbol count with bucket before symndx should have failed, returned %d",
		     ret);
    UNW_TEST_ASSERT (sym_num == 1234,
		     "sym_num should be unchanged, expected 1234 got %u",
		     sym_num);
  }

  {
    Elf32_Word sym_num = 1234;
    int ret = _Uelf32_dynamic_symtab_count (NULL, NULL, &sym_num);

    UNW_TEST_ASSERT (ret == -UNW_ENOINFO,
		     "Symbol count with missing hash tables should have failed, returned %d",
		     ret);
    UNW_TEST_ASSERT (sym_num == 1234,
		     "sym_num should be unchanged, expected 1234 got %u",
		     sym_num);
  }

  return UNW_TEST_EXIT_PASS;
}
