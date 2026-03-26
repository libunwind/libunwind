#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "unw_test.h"

/* Pull in the inline lookup functions directly. */
#include "dwarf/Gfind_proc_info_i.h"

int
main (void)
{
  /* --- sdata4 (32-bit) lookup tests --- */
  {
    /* Basic sdata4 lookup: find the correct entry. */
    struct table_entry table[] = {
      { .start_ip_offset = 0x100, .fde_offset = 0xA00 },
      { .start_ip_offset = 0x200, .fde_offset = 0xB00 },
      { .start_ip_offset = 0x300, .fde_offset = 0xC00 },
    };
    const struct table_entry *e;

    e = lookup (table, sizeof (table), 0x250);
    UNW_TEST_ASSERT (e != NULL, "sdata4 lookup returned NULL");
    UNW_TEST_ASSERT (e->start_ip_offset == 0x200,
                     "sdata4 expected start_ip_offset 0x200, got 0x%x",
                     e->start_ip_offset);
    UNW_TEST_ASSERT (e->fde_offset == 0xB00,
                     "sdata4 expected fde_offset 0xB00, got 0x%x",
                     e->fde_offset);
  }

  {
    /* sdata4 lookup: IP before first entry returns NULL. */
    struct table_entry table[] = {
      { .start_ip_offset = 0x100, .fde_offset = 0xA00 },
    };
    const struct table_entry *e;

    e = lookup (table, sizeof (table), 0x50);
    UNW_TEST_ASSERT (e == NULL,
                     "sdata4 lookup before first entry should return NULL");
  }

  {
    /* sdata4 lookup: IP exactly at an entry boundary. */
    struct table_entry table[] = {
      { .start_ip_offset = 0x100, .fde_offset = 0xA00 },
      { .start_ip_offset = 0x200, .fde_offset = 0xB00 },
    };
    const struct table_entry *e;

    e = lookup (table, sizeof (table), 0x200);
    UNW_TEST_ASSERT (e != NULL, "sdata4 exact match returned NULL");
    UNW_TEST_ASSERT (e->start_ip_offset == 0x200,
                     "sdata4 exact match expected 0x200, got 0x%x",
                     e->start_ip_offset);
  }

  {
    /* sdata4 lookup: IP in last entry's range. */
    struct table_entry table[] = {
      { .start_ip_offset = 0x100, .fde_offset = 0xA00 },
      { .start_ip_offset = 0x200, .fde_offset = 0xB00 },
      { .start_ip_offset = 0x300, .fde_offset = 0xC00 },
    };
    const struct table_entry *e;

    e = lookup (table, sizeof (table), 0x999);
    UNW_TEST_ASSERT (e != NULL, "sdata4 last entry returned NULL");
    UNW_TEST_ASSERT (e->start_ip_offset == 0x300,
                     "sdata4 last entry expected 0x300, got 0x%x",
                     e->start_ip_offset);
  }

  /* --- sdata8 (64-bit) lookup tests --- */
  {
    /* Basic sdata8 lookup: find the correct entry. */
    struct table_entry64 table[] = {
      { .start_ip_offset = 0x100000000LL, .fde_offset = 0xA00000000LL },
      { .start_ip_offset = 0x200000000LL, .fde_offset = 0xB00000000LL },
      { .start_ip_offset = 0x300000000LL, .fde_offset = 0xC00000000LL },
    };
    const struct table_entry64 *e;

    e = lookup64 (table, sizeof (table), 0x250000000LL);
    UNW_TEST_ASSERT (e != NULL, "sdata8 lookup returned NULL");
    UNW_TEST_ASSERT (e->start_ip_offset == 0x200000000LL,
                     "sdata8 expected start_ip_offset 0x200000000, got 0x%llx",
                     (long long) e->start_ip_offset);
    UNW_TEST_ASSERT (e->fde_offset == 0xB00000000LL,
                     "sdata8 expected fde_offset 0xB00000000, got 0x%llx",
                     (long long) e->fde_offset);
  }

  {
    /* sdata8 lookup: IP before first entry returns NULL. */
    struct table_entry64 table[] = {
      { .start_ip_offset = 0x100000000LL, .fde_offset = 0xA00000000LL },
    };
    const struct table_entry64 *e;

    e = lookup64 (table, sizeof (table), 0x50);
    UNW_TEST_ASSERT (e == NULL,
                     "sdata8 lookup before first entry should return NULL");
  }

  {
    /* sdata8 lookup: IP exactly at an entry boundary. */
    struct table_entry64 table[] = {
      { .start_ip_offset = 0x100000000LL, .fde_offset = 0xA00000000LL },
      { .start_ip_offset = 0x200000000LL, .fde_offset = 0xB00000000LL },
    };
    const struct table_entry64 *e;

    e = lookup64 (table, sizeof (table), 0x200000000LL);
    UNW_TEST_ASSERT (e != NULL, "sdata8 exact match returned NULL");
    UNW_TEST_ASSERT (e->start_ip_offset == 0x200000000LL,
                     "sdata8 exact match expected 0x200000000, got 0x%llx",
                     (long long) e->start_ip_offset);
  }

  {
    /* sdata8 lookup: addresses spanning > 2 GiB (the whole point of sdata8). */
    struct table_entry64 table[] = {
      { .start_ip_offset = -0x100000000LL,  .fde_offset = 0xA00 },
      { .start_ip_offset =  0x0,            .fde_offset = 0xB00 },
      { .start_ip_offset =  0x100000000LL,  .fde_offset = 0xC00 },
    };
    const struct table_entry64 *e;

    e = lookup64 (table, sizeof (table), -0x80000000LL);
    UNW_TEST_ASSERT (e != NULL, "sdata8 negative offset returned NULL");
    UNW_TEST_ASSERT (e->start_ip_offset == -0x100000000LL,
                     "sdata8 negative offset mismatch, got 0x%llx",
                     (long long) e->start_ip_offset);
  }

  {
    /* Single-entry table. */
    struct table_entry64 table[] = {
      { .start_ip_offset = 0x42, .fde_offset = 0xFF },
    };
    const struct table_entry64 *e;

    e = lookup64 (table, sizeof (table), 0x42);
    UNW_TEST_ASSERT (e != NULL, "sdata8 single entry returned NULL");
    UNW_TEST_ASSERT (e->start_ip_offset == 0x42,
                     "sdata8 single entry expected 0x42, got 0x%llx",
                     (long long) e->start_ip_offset);
  }

  {
    /* Empty table. */
    const struct table_entry64 *e;

    e = lookup64 (NULL, 0, 0x100);
    UNW_TEST_ASSERT (e == NULL, "sdata8 empty table should return NULL");
  }

  return UNW_TEST_EXIT_PASS;
}
