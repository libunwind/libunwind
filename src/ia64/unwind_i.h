/* libunwind - a platform-independent unwind library
   Copyright (C) 2001-2002 Hewlett-Packard Co
	Contributed by David Mosberger-Tang <davidm@hpl.hp.com>

This file is part of libunwind.

libunwind is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2, or (at your option)
any later version.

libunwind is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

As a special exception, if you link this library with other files to
produce an executable, this library does not by itself cause the
resulting executable to be covered by the GNU General Public License.
This exception does not however invalidate any other reasons why the
executable file might be covered by the GNU General Public
License.  */

#ifndef unwind_i_h
#define unwind_i_h

#define IA64_UNW_SCRIPT_CACHE

#include <memory.h>
#include <stdint.h>

#include <libunwind-ia64.h>

#define struct_offset(str,fld)	((char *)&((str *)NULL)->fld - (char *) 0)

#define IA64_UNW_VER(x)			((x) >> 48)
#define IA64_UNW_FLAG_MASK		0x0000ffff00000000
#define IA64_UNW_FLAG_OSMASK		0x0000f00000000000
#define IA64_UNW_FLAG_EHANDLER(x)	((x) & 0x0000000100000000L)
#define IA64_UNW_FLAG_UHANDLER(x)	((x) & 0x0000000200000000L)
#define IA64_UNW_LENGTH(x)		((x) & 0x00000000ffffffffL)

#define MIN(a,b)	((a) < (b) ? (a) : (b))

#ifdef DEBUG
# define IA64_UNW_DEBUG	1
#endif

#define IA64_UNW_STATS	0

#if IA64_UNW_DEBUG
# include <stdio.h>
# define debug(level,format...)	\
	do { if (unw.debug_level > level) printf (format); } while (0)
# define dprintf(format...) \
	printf (format)
# define inline	__attribute__ ((unused))
#else
# define debug(level,format...)
# define dprintf(format...)
#endif

#if IA64_UNW_STATS
# define STAT(x...)	x
#else
# define STAT(x...)
#endif

enum ia64_pregnum
  {
    /* primary unat: */
    IA64_REG_PRI_UNAT_GR,
    IA64_REG_PRI_UNAT_MEM,

    /* register stack */
    IA64_REG_BSP,			/* register stack pointer */
    IA64_REG_BSPSTORE,
    IA64_REG_PFS,			/* previous function state */
    IA64_REG_RNAT,
    /* memory stack */
    IA64_REG_PSP,			/* previous memory stack pointer */
    /* return pointer: */
    IA64_REG_RP,

    /* preserved registers: */
    IA64_REG_R4, IA64_REG_R5, IA64_REG_R6, IA64_REG_R7,
    IA64_REG_NAT4, IA64_REG_NAT5, IA64_REG_NAT6, IA64_REG_NAT7,
    IA64_REG_UNAT, IA64_REG_PR, IA64_REG_LC, IA64_REG_FPSR,
    IA64_REG_B1, IA64_REG_B2, IA64_REG_B3, IA64_REG_B4, IA64_REG_B5,
    IA64_REG_F2, IA64_REG_F3, IA64_REG_F4, IA64_REG_F5,
    IA64_REG_F16, IA64_REG_F17, IA64_REG_F18, IA64_REG_F19,
    IA64_REG_F20, IA64_REG_F21, IA64_REG_F22, IA64_REG_F23,
    IA64_REG_F24, IA64_REG_F25, IA64_REG_F26, IA64_REG_F27,
    IA64_REG_F28, IA64_REG_F29, IA64_REG_F30, IA64_REG_F31,
    IA64_NUM_PREGS
  };

#define IA64_FLAG_SIGTRAMP	(1 << 0)	/* signal trampoline? */
#define IA64_FLAG_BIG_ENDIAN	(1 << 1)	/* big-endian? */
#define IA64_FLAG_HAS_HANDLER	(1 << 2)	/* has personality routine? */

struct ia64_proc_info
  {
    unsigned int flags;		/* see IA64_FLAG_* above */
    unw_word_t gp;		/* global pointer value */
    unw_word_t proc_start;	/* start address of procedure */
    uint64_t *pers_addr;	/* address of personality routine pointer */
    uint8_t *desc;		/* encoded unwind descriptors (or NULL) */
  };

struct ia64_cursor
  {
#ifdef UNW_LOCAL_ONLY
    ucontext_t *uc;		/* pointer to struct of preserved registers */
#else
    struct unw_accessors acc;
#endif

    /* IP and predicate cache (these are always equal to the values
       stored in ip_loc and pr_loc, respectively).  */
    unw_word_t ip;		/* instruction pointer value */
    unw_word_t pr;		/* current predicate values */

    /* current frame info: */
    unw_word_t bsp;		/* backing store pointer value */
    unw_word_t sp;		/* stack pointer value */
    unw_word_t psp;		/* previous sp value */
    unw_word_t cfm_loc;		/* cfm save location (or NULL) */
    unw_word_t rbs_top;		/* address of end of register backing store */
    unw_word_t top_rnat_loc;	/* location of final (topmost) RNaT word */
    struct ia64_proc_info pi;	/* info about current procedure */

    /* preserved state: */
    unw_word_t bsp_loc;		/* previous bsp save location */
    unw_word_t bspstore_loc;
    unw_word_t pfs_loc;
    unw_word_t rnat_loc;
    unw_word_t ip_loc;
    unw_word_t pri_unat_loc;
    unw_word_t unat_loc;
    unw_word_t pr_loc;
    unw_word_t lc_loc;
    unw_word_t fpsr_loc;
    unw_word_t r4_loc, r5_loc, r6_loc, r7_loc;
    unw_word_t nat4_loc, nat5_loc, nat6_loc, nat7_loc;
    unw_word_t b1_loc, b2_loc, b3_loc, b4_loc, b5_loc;
    unw_word_t f2_loc, f3_loc, f4_loc, f5_loc, fr_loc[16];

    unw_word_t eh_args[4];	/* exception handler arguments */

#ifdef IA64_UNW_SCRIPT_CACHE
    short hint;
    short prev_script;
#endif
};

#ifdef IA64_UNW_SCRIPT_CACHE
# include "script.h"
#endif

/* Bits 0 to 2 of an location are used to encode its type:

	bit 0: set if location uses floating-point format.
	bit 1: set if location is a NaT bit on memory stack
	bit 2: set if location is a register (not needed for local-only).  */

#define IA64_LOC_TYPE_FP		(1 << 0)
#define IA64_LOC_TYPE_MEMSTK_NAT	(1 << 1)
#define IA64_LOC_TYPE_REG		(1 << 2)

#define IA64_LOC(l,t)		(((l) << 3) | (t))

#define IA64_GET_LOC(l)		((l) >> 3)
#define IA64_MASK_LOC_TYPE(l)	((l) & ~0x7)
#define IA64_IS_FP_LOC(loc)	(((loc) & IA64_LOC_TYPE_FP) != 0)
#define IA64_IS_MEMSTK_NAT(loc)	(((loc) & IA64_LOC_TYPE_MEMSTK_NAT) != 0)

#ifdef UNW_LOCAL_ONLY

extern void *_Uia64_uc_addr (ucontext_t *uc, unw_regnum_t regnum);

#define IA64_REG_LOC(c,r)	((unw_word_t) _Uia64_uc_addr((c)->uc, (r)))
#define IA64_FPREG_LOC(c,r)						\
	((unw_word_t) _Uia64_uc_addr((c)->uc, (r)) | IA64_LOC_TYPE_FP)

# define ia64_acquire_unwind_info(c,ip,i)			\
	_Uia64_glibc_acquire_unwind_info((ip), (i), (c)->uc)
# define ia64_release_unwind_info(c,ip,i)			\
	_Uia64_glibc_release_unwind_info((i), (c)->uc)
# define ia64_get(c,l,v)					\
	(*(v) = *(unw_word_t *) IA64_MASK_LOC_TYPE(l), 0)
# define ia64_put(c,l,v)					\
	(*(unw_word_t *) IA64_MASK_LOC_TYPE(l) = (v), 0)
# define ia64_getfp(c,l,v)					\
	(*(v) = *(unw_fpreg_t *) IA64_MASK_LOC_TYPE(l), 0)
# define ia64_putfp(c,l,v)					\
	(*(unw_fpreg_t *) IA64_MASK_LOC_TYPE(l) = (v), 0)

#else /* !UNW_LOCAL_ONLY */

#define IA64_IS_REG_LOC(r)	(((r) & IA64_LOC_TYPE_REG) != 0)
#define IA64_REG_LOC(c,r)	IA64_LOC((r), IA64_LOC_TYPE_REG)
#define IA64_FPREG_LOC(c,r)	IA64_LOC((r), (IA64_LOC_TYPE_REG \
					       | IA64_LOC_TYPE_FP))

# define ia64_acquire_unwind_info(c,ip,i) \
	(*(c)->acc.acquire_unwind_info)((ip), (i), (c)->acc.arg)
# define ia64_release_unwind_info(c,ip,i) \
	(*(c)->acc.release_unwind_info)((i), (c)->acc.arg)

static inline int
ia64_getfp (struct ia64_cursor *c, unw_word_t loc, unw_fpreg_t *val)
{
  int ret;

  if (IA64_IS_REG_LOC (loc))
    return (*c->acc.access_fpreg) (IA64_GET_LOC (loc), val, 0, c->acc.arg);

  loc = IA64_MASK_LOC_TYPE(loc);
  ret = (*c->acc.access_mem) (loc + 0, &val->raw.bits[0], 0, c->acc.arg);
  if (ret < 0)
    return ret;

  return (*c->acc.access_mem) (loc + 8, &val->raw.bits[1], 0, c->acc.arg);
}

static inline int
ia64_putfp (struct ia64_cursor *c, unw_word_t loc, unw_fpreg_t val)
{
  int ret;

  if (IA64_IS_REG_LOC (loc))
    return (*c->acc.access_fpreg) (IA64_GET_LOC (loc), &val, 1, c->acc.arg);

  loc = IA64_MASK_LOC_TYPE(loc);
  ret = (*c->acc.access_mem) (loc + 0, &val.raw.bits[0], 1, c->acc.arg);
  if (ret < 0)
    return ret;

  return (*c->acc.access_mem) (loc + 8, &val.raw.bits[1], 1, c->acc.arg);
}

/* Get the 64 data bits from location LOC.  If bit 0 is cleared, LOC
   is a memory address, otherwise it is a register number.  If the
   register is a floating-point register, the 64 bits are read from
   the significand bits.  */

static inline int
ia64_get (struct ia64_cursor *c, unw_word_t loc, unw_word_t *val)
{
  if (IA64_IS_FP_LOC (loc))
    {
      unw_fpreg_t tmp;
      int ret;

      ret = ia64_getfp (c, loc, &tmp);
      if (ret < 0)
	return ret;

      if (c->pi.flags & IA64_FLAG_BIG_ENDIAN)
	*val = tmp.raw.bits[1];
      else
	*val = tmp.raw.bits[0];
      return 0;
    }

  if (IA64_IS_REG_LOC (loc))
    return (*c->acc.access_reg)(IA64_GET_LOC (loc), val, 0, c->acc.arg);
  else
    return (*c->acc.access_mem)(loc, val, 0, c->acc.arg);
}

static inline int
ia64_put (struct ia64_cursor *c, unw_word_t loc, unw_word_t val)
{
  if (IA64_IS_FP_LOC (loc))
    {
      unw_fpreg_t tmp;

      memset (&tmp, 0, sizeof (tmp));
      if (c->pi.flags & IA64_FLAG_BIG_ENDIAN)
	tmp.raw.bits[1] = val;
      else
	tmp.raw.bits[0] = val;
      return ia64_putfp (c, loc, tmp);
    }

  if (loc & 1)
    return (*c->acc.access_reg)(loc >> 1, &val, 1, c->acc.arg);
  else
    return (*c->acc.access_mem)(loc, &val, 1, c->acc.arg);
}

#endif /* !UNW_LOCAL_ONLY */

struct ia64_unwind_block
  {
    unw_word_t header;
    unw_word_t desc[0];			/* unwind descriptors */

    /* Personality routine and language-specific data follow behind
       descriptors.  */
  };

struct ia64_unwind_table_entry
  {
    unw_word_t start_offset;
    unw_word_t end_offset;
    unw_word_t info_offset;
  };

struct ia64_unwind_table
  {
    struct ia64_unwind_table *next;	/* must be first member! */
    unw_ia64_table_t info;
  };

enum ia64_where
  {
    IA64_WHERE_NONE,	/* register isn't saved at all */
    IA64_WHERE_GR,	/* register is saved in a general register */
    IA64_WHERE_FR,	/* register is saved in a floating-point register */
    IA64_WHERE_BR,	/* register is saved in a branch register */
    IA64_WHERE_SPREL,	/* register is saved on memstack (sp-relative) */
    IA64_WHERE_PSPREL,	/* register is saved on memstack (psp-relative) */

    /* At the end of each prologue these locations get resolved to
       IA64_WHERE_PSPREL and IA64_WHERE_GR, respectively:  */

    IA64_WHERE_SPILL_HOME, /* register is saved in its spill home */
    IA64_WHERE_GR_SAVE	/* register is saved in next general register */
};

#define IA64_WHEN_NEVER	0x7fffffff

struct ia64_reg_info
  {
    unw_word_t val;		/* save location: register number or offset */
    enum ia64_where where;	/* where the register gets saved */
    int when;			/* when the register gets saved */
  };

struct ia64_labeled_state;	/* opaque structure */

struct ia64_reg_state {
  struct ia64_reg_state *next;	/* next (outer) element on state stack */
  struct ia64_reg_info reg[IA64_NUM_PREGS];	/* register save locations */
};

struct ia64_state_record
  {
    unsigned int first_region : 1;	/* is this the first region? */
    unsigned int done : 1;		/* are we done scanning descriptors? */
    unsigned int any_spills : 1;	/* got any register spills? */
    unsigned int in_body : 1;		/* are we inside prologue or body? */
    unsigned int flags;		/* see IA64_FLAG_* */

    uint8_t *imask;		/* imask of spill_mask record or NULL */
    unw_word_t pr_val;		/* predicate values */
    unw_word_t pr_mask;		/* predicate mask */

    long spill_offset;		/* psp-relative offset for spill base */
    int region_start;
    int region_len;
    int epilogue_start;
    int epilogue_count;
    int when_target;

    uint8_t gr_save_loc;	/* next save register */
    uint8_t return_link_reg;	/* branch register used as return pointer */

    struct ia64_labeled_state *labeled_states;
    struct ia64_reg_state curr;
};

struct ia64_global_unwind_state
  {
    int first_time;

    /* List of unwind tables (one per load-module).  */
    struct ia64_unwind_table *tables;

    /* Table of registers that prologues can save (and order in which
       they're saved).  */
    const unsigned char save_order[8];

    /* Maps a preserved register index (preg_index) to corresponding
       ucontext_t offset.  */
    unsigned short uc_off[sizeof(unw_cursor_t) / 8];

    /* Index into unw_cursor_t for preserved register i */
    unsigned short preg_index[IA64_NUM_PREGS];

    unw_fpreg_t f0, f1_le, f1_be, nat_val_le;
    unw_fpreg_t nat_val_be, int_val_le, int_val_be;

#ifdef IA64_UNW_SCRIPT_CACHE
    unsigned short lru_head;	/* index of lead-recently used script */
    unsigned short lru_tail;	/* index of most-recently used script */

    /* hash table that maps instruction pointer to script index: */
    unsigned short hash[IA64_UNW_HASH_SIZE];

    /* script cache: */
    struct ia64_script cache[IA64_UNW_CACHE_SIZE];
#endif

# if IA64_UNW_DEBUG
    long debug_level;
    const char *preg_name[IA64_NUM_PREGS];
# endif
# if IA64_UNW_STATS
    struct
      {
	struct
          {
	    int lookups;
	    int hinted_hits;
	    int normal_hits;
	    int collision_chain_traversals;
          }
	cache;
	struct
          {
	    unsigned long build_time;
	    unsigned long run_time;
	    unsigned long parse_time;
	    int builds;
	    int news;
	    int collisions;
	    int runs;
          }
	script;
	struct
          {
	    unsigned long init_time;
	    unsigned long unwind_time;
	    int inits;
	    int unwinds;
          }
	api;
     }
    stat;
# endif /* IA64_UNW_STATS */
  };

/* Convenience macros: */
#define unw				UNW_OBJ(data)
#define ia64_get_proc_info		UNW_OBJ(get_proc_info)
#define ia64_create_state_record	UNW_OBJ(create_state_record)
#define ia64_free_state_record		UNW_OBJ(free_state_record)
#define ia64_find_save_locs		UNW_OBJ(find_save_locs)
#define ia64_init			UNW_OBJ(init)
#define ia64_access_reg			UNW_OBJ(access_reg)
#define ia64_access_fpreg		UNW_OBJ(access_fpreg)
#define ia64_scratch_loc		UNW_OBJ(scratch_loc)
#define ia64_local_resume		UNW_OBJ(local_resume)

extern struct ia64_global_unwind_state unw;

extern int ia64_get_proc_info (struct ia64_cursor *c);
extern int ia64_create_state_record (struct ia64_cursor *c,
				     struct ia64_state_record *sr);
extern int ia64_free_state_record (struct ia64_state_record *sr);
extern int ia64_find_save_locs (struct ia64_cursor *c);
extern void ia64_init (void);
extern int ia64_access_reg (struct ia64_cursor *c, unw_regnum_t reg,
			    unw_word_t *valp, int write);
extern int ia64_access_fpreg (struct ia64_cursor *c, unw_regnum_t reg,
			      unw_fpreg_t *valp, int write);
extern unw_word_t ia64_scratch_loc (struct ia64_cursor *c, unw_regnum_t reg);

extern void __ia64_install_context (const ucontext_t *ucp, long r15, long r16,
				    long r17, long r18)
	__attribute__ ((noreturn));
extern int ia64_local_resume (unw_cursor_t *cursor, void *arg);

extern int _Uia64_glibc_acquire_unwind_info (unw_word_t ip, void *info,
					     void *arg);
extern int _Uia64_glibc_release_unwind_info (void *info, void *arg);

/* XXX should be in glibc: */
#ifndef IA64_SC_FLAG_ONSTACK
# define IA64_SC_FLAG_ONSTACK_BIT    0 /* running on signal stack? */
# define IA64_SC_FLAG_IN_SYSCALL_BIT 1 /* did signal interrupt a syscall? */
# define IA64_SC_FLAG_FPH_VALID_BIT  2 /* is state in f[32]-f[127] valid? */

# define IA64_SC_FLAG_ONSTACK		(1 << IA64_SC_FLAG_ONSTACK_BIT)
# define IA64_SC_FLAG_IN_SYSCALL	(1 << IA64_SC_FLAG_IN_SYSCALL_BIT)
# define IA64_SC_FLAG_FPH_VALID		(1 << IA64_SC_FLAG_FPH_VALID_BIT)
#endif

#endif /* unwind_i_h */
