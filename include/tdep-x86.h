/* libunwind - a platform-independent unwind library
   Copyright (C) 2002-2003 Hewlett-Packard Co
	Contributed by David Mosberger-Tang <davidm@hpl.hp.com>

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

#ifndef TDEP_X86_H
#define TDEP_X86_H

/* Target-dependent definitions that are internal to libunwind but need
   to be shared with target-independent code.  */

#include <stdlib.h>
#include <libunwind.h>

#include "elf32.h"
#include "dwarf.h"

struct unw_addr_space
  {
    struct unw_accessors acc;
    unw_caching_policy_t caching_policy;
    uint32_t cache_generation;
    unw_word_t dyn_generation;		/* see dyn-common.h */
    unw_word_t dyn_info_list_addr;	/* (cached) dyn_info_list_addr */
   };

struct cursor
  {
    struct dwarf_cursor dwarf;		/* must be first */
  };

#define DWARF_GET_LOC(l)	((l).val)

#ifdef UNW_LOCAL_ONLY
# define DWARF_NULL_LOC		DWARF_LOC (0, 0)
# define DWARF_IS_NULL_LOC(l)	(DWARF_GET_LOC (l) == 0)
# define DWARF_LOC(r, t)	((dwarf_loc_t) { .val = (r) })
# define DWARF_REG_LOC(c,r)	(DWARF_LOC((unw_word_t)			     \
				 tdep_uc_addr((c)->as_arg, (r)), 0))
# define DWARF_MEM_LOC(c,m)	DWARF_LOC ((m), 0)
# define DWARF_FPREG_LOC(c,r)	(DWARF_LOC((unw_word_t)			     \
				 tdep_uc_addr((c)->as_arg, (r)), 0))

static inline int
dwarf_getfp (struct dwarf_cursor *c, dwarf_loc_t loc, unw_fpreg_t *val)
{
  if (!DWARF_GET_LOC (loc))
    return -1;
  *val = *(unw_fpreg_t *) DWARF_GET_LOC (loc);
  return 0;
}

static inline int
dwarf_putfp (struct dwarf_cursor *c, dwarf_loc_t loc, unw_fpreg_t *val)
{
  if (!DWARF_GET_LOC (loc))
    return -1;
  *(unw_fpreg_t *) DWARF_GET_LOC (loc) = *val;
  return 0;
}

static inline int
dwarf_get (struct dwarf_cursor *c, dwarf_loc_t loc, unw_word_t *val)
{
  if (!DWARF_GET_LOC (loc))
    return -1;
  *val = *(unw_word_t *) DWARF_GET_LOC (loc);
  return 0;
}

static inline int
dwarf_put (struct dwarf_cursor *c, dwarf_loc_t loc, unw_word_t val)
{
  if (!DWARF_GET_LOC (loc))
    return -1;
  *(unw_word_t *) DWARF_GET_LOC (loc) = val;
  return 0;
}

#else /* !UNW_LOCAL_ONLY */
# define DWARF_LOC_TYPE_FP	(1 << 0)
# define DWARF_LOC_TYPE_REG	(1 << 1)
# define DWARF_NULL_LOC		DWARF_LOC (0, 0)
# define DWARF_IS_NULL_LOC(l)						\
		({ dwarf_loc_t _l = (l); _l.val == 0 && _l.type == 0; })
# define DWARF_LOC(r, t)	((dwarf_loc_t) { .val = (r), .type = (t) })
# define DWARF_IS_REG_LOC(l)	(((l).type & DWARF_LOC_TYPE_REG) != 0)
# define DWARF_IS_FP_LOC(l)	(((l).type & DWARF_LOC_TYPE_FP) != 0)
# define DWARF_REG_LOC(c,r)	DWARF_LOC((r), DWARF_LOC_TYPE_REG)
# define DWARF_MEM_LOC(c,m)	DWARF_LOC ((m), 0)
# define DWARF_FPREG_LOC(c,r)	DWARF_LOC((r), (DWARF_LOC_TYPE_REG	\
						| DWARF_LOC_TYPE_FP))

static inline int
dwarf_getfp (struct dwarf_cursor *c, dwarf_loc_t loc, unw_fpreg_t *val)
{
  abort ();
}

static inline int
dwarf_putfp (struct dwarf_cursor *c, dwarf_loc_t loc, unw_fpreg_t val)
{
  abort ();
}

static inline int
dwarf_get (struct dwarf_cursor *c, dwarf_loc_t loc, unw_word_t *val)
{
  if (DWARF_IS_FP_LOC (loc))
    abort ();

  if (DWARF_IS_REG_LOC (loc))
    return (*c->as->acc.access_reg) (c->as, DWARF_GET_LOC (loc), val,
				     0, c->as_arg);
  else
    return (*c->as->acc.access_mem) (c->as, DWARF_GET_LOC (loc), val,
				     0, c->as_arg);
}

static inline int
dwarf_put (struct dwarf_cursor *c, dwarf_loc_t loc, unw_word_t val)
{
  if (DWARF_IS_FP_LOC (loc))
    abort ();

  if (DWARF_IS_REG_LOC (loc))
    return (*c->as->acc.access_reg) (c->as, DWARF_GET_LOC (loc), &val,
				     1, c->as_arg);
  else
    return (*c->as->acc.access_mem) (c->as, DWARF_GET_LOC (loc), &val,
				     1, c->as_arg);
}

#endif /* !UNW_LOCAL_ONLY */

/* Platforms that support UNW_INFO_FORMAT_TABLE need to define
   tdep_search_unwind_table.  */
#define tdep_search_unwind_table	dwarf_search_unwind_table
#define tdep_find_proc_info		dwarf_find_proc_info
#define tdep_put_unwind_info		dwarf_put_unwind_info
#define tdep_uc_addr(uc,reg)		UNW_ARCH_OBJ(uc_addr)(uc,reg)
#define tdep_get_elf_image(a,b,c,d,e)	UNW_ARCH_OBJ(get_elf_image) (a, b, c, \
								     d, e)
#define tdep_debug_level			UNW_ARCH_OBJ(debug_level)

extern int tdep_search_unwind_table (unw_addr_space_t as, unw_word_t ip,
				     unw_dyn_info_t *di, unw_proc_info_t *pi,
				     int need_unwind_info, void *arg);
extern void tdep_put_unwind_info (unw_addr_space_t as,
				  unw_proc_info_t *pi, void *arg);
extern void *tdep_uc_addr (ucontext_t *uc, int reg);
extern int tdep_get_elf_image (struct elf_image *ei, pid_t pid, unw_word_t ip,
			       unsigned long *segbase, unsigned long *mapoff);
extern int tdep_debug_level;

#endif /* TDEP_X86_H */
