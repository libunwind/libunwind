#ifndef dwarf_find_proc_info_i_h
#define dwarf_find_proc_info_i_h

#include <stdint.h>
#include <stddef.h>

#ifndef Debug
#define Debug(level, ...)
#endif

struct table_entry
  {
    int32_t start_ip_offset;
    int32_t fde_offset;
  };

struct table_entry64
  {
    int64_t start_ip_offset;
    int64_t fde_offset;
  };

static inline const struct table_entry *
lookup (const struct table_entry *table, size_t table_size, int32_t rel_ip)
{
  unsigned long table_len = table_size / sizeof (struct table_entry);
  const struct table_entry *e = NULL;
  unsigned long lo, hi, mid;

  /* do a binary search for right entry: */
  for (lo = 0, hi = table_len; lo < hi;)
    {
      mid = (lo + hi) / 2;
      e = table + mid;
      Debug (15, "e->start_ip_offset = %lx\n", (long) e->start_ip_offset);
      if (rel_ip < e->start_ip_offset)
        hi = mid;
      else
        lo = mid + 1;
    }
  if (hi <= 0)
        return NULL;
  e = table + hi - 1;
  return e;
}

static inline const struct table_entry64 *
lookup64 (const struct table_entry64 *table, size_t table_size, int64_t rel_ip)
{
  unsigned long table_len = table_size / sizeof (struct table_entry64);
  const struct table_entry64 *e = NULL;
  unsigned long lo, hi, mid;

  /* do a binary search for right entry: */
  for (lo = 0, hi = table_len; lo < hi;)
    {
      mid = (lo + hi) / 2;
      e = table + mid;
      Debug (15, "e->start_ip_offset = %lx\n", (long) e->start_ip_offset);
      if (rel_ip < e->start_ip_offset)
        hi = mid;
      else
        lo = mid + 1;
    }
  if (hi <= 0)
        return NULL;
  e = table + hi - 1;
  return e;
}

#endif /* dwarf_find_proc_info_i_h */
