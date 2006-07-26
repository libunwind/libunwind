#ifndef DWARF_I_H
#define DWARF_I_H

/* This file contains definitions that cannot be used in code outside
   of libunwind.  In particular, most inline functions are here
   because otherwise they'd generate unresolved references when the
   files are compiled with inlining disabled.  */

#include "dwarf.h"
#include "tdep.h"

#define dwarf_to_unw_regnum_map		UNW_OBJ (dwarf_to_unw_regnum_map)

extern uint8_t dwarf_to_unw_regnum_map[DWARF_REGNUM_MAP_LENGTH];

static inline unw_regnum_t
dwarf_to_unw_regnum (unw_word_t regnum)
{
  if (regnum <= DWARF_REGNUM_MAP_LENGTH)
    return dwarf_to_unw_regnum_map[regnum];
  return 0;
}

#ifdef UNW_LOCAL_ONLY

/* In the local-only case, we can let the compiler directly access
   memory and don't need to worry about differing byte-order.  */

typedef union
  {
    int8_t s8;
    int16_t s16;
    int32_t s32;
    int64_t s64;
    uint8_t u8;
    uint16_t u16;
    uint32_t u32;
    uint64_t u64;
    unw_word_t w;
    void *ptr;
  }
dwarf_misaligned_value_t __attribute__ ((packed));

static inline int
dwarf_reads8 (unw_addr_space_t as, unw_accessors_t *a, unw_word_t *addr,
	      int8_t *val, void *arg)
{
  dwarf_misaligned_value_t *mvp = (void *) *addr;

  *val = mvp->s8;
  *addr += sizeof (mvp->s8);
  return 0;
}

static inline int
dwarf_reads16 (unw_addr_space_t as, unw_accessors_t *a, unw_word_t *addr,
	       int16_t *val, void *arg)
{
  dwarf_misaligned_value_t *mvp = (void *) *addr;

  *val = mvp->s16;
  *addr += sizeof (mvp->s16);
  return 0;
}

static inline int
dwarf_reads32 (unw_addr_space_t as, unw_accessors_t *a, unw_word_t *addr,
	       int32_t *val, void *arg)
{
  dwarf_misaligned_value_t *mvp = (void *) *addr;

  *val = mvp->s32;
  *addr += sizeof (mvp->s32);
  return 0;
}

static inline int
dwarf_reads64 (unw_addr_space_t as, unw_accessors_t *a, unw_word_t *addr,
	       int64_t *val, void *arg)
{
  dwarf_misaligned_value_t *mvp = (void *) *addr;

  *val = mvp->s64;
  *addr += sizeof (mvp->s64);
  return 0;
}

static inline int
dwarf_readu8 (unw_addr_space_t as, unw_accessors_t *a, unw_word_t *addr,
	      uint8_t *val, void *arg)
{
  dwarf_misaligned_value_t *mvp = (void *) *addr;

  *val = mvp->u8;
  *addr += sizeof (mvp->u8);
  return 0;
}

static inline int
dwarf_readu16 (unw_addr_space_t as, unw_accessors_t *a, unw_word_t *addr,
	       uint16_t *val, void *arg)
{
  dwarf_misaligned_value_t *mvp = (void *) *addr;

  *val = mvp->u16;
  *addr += sizeof (mvp->u16);
  return 0;
}

static inline int
dwarf_readu32 (unw_addr_space_t as, unw_accessors_t *a, unw_word_t *addr,
	       uint32_t *val, void *arg)
{
  dwarf_misaligned_value_t *mvp = (void *) *addr;

  *val = mvp->u32;
  *addr += sizeof (mvp->u32);
  return 0;
}

static inline int
dwarf_readu64 (unw_addr_space_t as, unw_accessors_t *a, unw_word_t *addr,
	       uint64_t *val, void *arg)
{
  dwarf_misaligned_value_t *mvp = (void *) *addr;

  *val = mvp->u64;
  *addr += sizeof (mvp->u64);
  return 0;
}

static inline int
dwarf_readw (unw_addr_space_t as, unw_accessors_t *a, unw_word_t *addr,
	     unw_word_t *val, void *arg)
{
  dwarf_misaligned_value_t *mvp = (void *) *addr;

  *val = mvp->w;
  *addr += sizeof (mvp->w);
  return 0;
}

#else /* !UNW_LOCAL_ONLY */

static inline int
dwarf_readu8 (unw_addr_space_t as, unw_accessors_t *a, unw_word_t *addr,
	      uint8_t *valp, void *arg)
{
  unw_word_t val, aligned_addr = *addr & -sizeof (unw_word_t);
  unw_word_t off = *addr - aligned_addr;
  int ret;

  *addr += 1;
  ret = (*a->access_mem) (as, aligned_addr, &val, 0, arg);
#if __BYTE_ORDER == __LITTLE_ENDIAN
  val >>= 8*off;
#else
  val >>= 8*(sizeof (unw_word_t) - 1 - off);
#endif
  *valp = (uint8_t) val;
  return ret;
}

static inline int
dwarf_readu16 (unw_addr_space_t as, unw_accessors_t *a, unw_word_t *addr,
	       uint16_t *val, void *arg)
{
  uint8_t v0, v1;
  int ret;

  if ((ret = dwarf_readu8 (as, a, addr, &v0, arg)) < 0
      || (ret = dwarf_readu8 (as, a, addr, &v1, arg)) < 0)
    return ret;

  if (tdep_big_endian (as))
    *val = (uint16_t) v0 << 8 | v1;
  else
    *val = (uint16_t) v1 << 8 | v0;
  return 0;
}

static inline int
dwarf_readu32 (unw_addr_space_t as, unw_accessors_t *a, unw_word_t *addr,
	       uint32_t *val, void *arg)
{
  uint16_t v0, v1;
  int ret;

  if ((ret = dwarf_readu16 (as, a, addr, &v0, arg)) < 0
      || (ret = dwarf_readu16 (as, a, addr, &v1, arg)) < 0)
    return ret;

  if (tdep_big_endian (as))
    *val = (uint32_t) v0 << 16 | v1;
  else
    *val = (uint32_t) v1 << 16 | v0;
  return 0;
}

static inline int
dwarf_readu64 (unw_addr_space_t as, unw_accessors_t *a, unw_word_t *addr,
	       uint64_t *val, void *arg)
{
  uint32_t v0, v1;
  int ret;

  if ((ret = dwarf_readu32 (as, a, addr, &v0, arg)) < 0
      || (ret = dwarf_readu32 (as, a, addr, &v1, arg)) < 0)
    return ret;

  if (tdep_big_endian (as))
    *val = (uint64_t) v0 << 32 | v1;
  else
    *val = (uint64_t) v1 << 32 | v0;
  return 0;
}

static inline int
dwarf_reads8 (unw_addr_space_t as, unw_accessors_t *a, unw_word_t *addr,
	      int8_t *val, void *arg)
{
  uint8_t uval;
  int ret;

  if ((ret = dwarf_readu8 (as, a, addr, &uval, arg)) < 0)
    return ret;
  *val = (int8_t) uval;
  return 0;
}

static inline int
dwarf_reads16 (unw_addr_space_t as, unw_accessors_t *a, unw_word_t *addr,
	       int16_t *val, void *arg)
{
  uint16_t uval;
  int ret;

  if ((ret = dwarf_readu16 (as, a, addr, &uval, arg)) < 0)
    return ret;
  *val = (int16_t) uval;
  return 0;
}

static inline int
dwarf_reads32 (unw_addr_space_t as, unw_accessors_t *a, unw_word_t *addr,
	       int32_t *val, void *arg)
{
  uint32_t uval;
  int ret;

  if ((ret = dwarf_readu32 (as, a, addr, &uval, arg)) < 0)
    return ret;
  *val = (int32_t) uval;
  return 0;
}

static inline int
dwarf_reads64 (unw_addr_space_t as, unw_accessors_t *a, unw_word_t *addr,
	       int64_t *val, void *arg)
{
  uint64_t uval;
  int ret;

  if ((ret = dwarf_readu64 (as, a, addr, &uval, arg)) < 0)
    return ret;
  *val = (int64_t) uval;
  return 0;
}

static inline int
dwarf_readw (unw_addr_space_t as, unw_accessors_t *a, unw_word_t *addr,
	     unw_word_t *val, void *arg)
{
  switch (sizeof (unw_word_t))
    {
    case 4:
      return dwarf_readu32 (as, a, addr, (uint32_t *) val, arg);

    case 8:
      return dwarf_readu64 (as, a, addr, (uint64_t *) val, arg);

    default:
      abort ();
    }
}

#endif /* !UNW_LOCAL_ONLY */

/* Read an unsigned "little-endian base 128" value.  See Chapter 7.6
   of DWARF spec v3.  */

static inline int
dwarf_read_uleb128 (unw_addr_space_t as, unw_accessors_t *a, unw_word_t *addr,
		    unw_word_t *valp, void *arg)
{
  unw_word_t val = 0, shift = 0;
  unsigned char byte;
  int ret;

  do
    {
      if ((ret = dwarf_readu8 (as, a, addr, &byte, arg)) < 0)
	return ret;

      val |= ((unw_word_t) byte & 0x7f) << shift;
      shift += 7;
    }
  while (byte & 0x80);

  *valp = val;
  return 0;
}

/* Read a signed "little-endian base 128" value.  See Chapter 7.6 of
   DWARF spec v3.  */

static inline int
dwarf_read_sleb128 (unw_addr_space_t as, unw_accessors_t *a, unw_word_t *addr,
		    unw_word_t *valp, void *arg)
{
  unw_word_t val = 0, shift = 0;
  unsigned char byte;
  int ret;

  do
    {
      if ((ret = dwarf_readu8 (as, a, addr, &byte, arg)) < 0)
	return ret;

      val |= ((unw_word_t) byte & 0x7f) << shift;
      shift += 7;
    }
  while (byte & 0x80);

  if (shift < 8 * sizeof (unw_word_t) && (byte & 0x40) != 0)
    /* sign-extend negative value */
    val |= ((unw_word_t) -1) << shift;

  *valp = val;
  return 0;
}

#endif /* DWARF_I_H */
