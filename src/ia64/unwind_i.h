/* libunwind - a platform-independent unwind library
   Copyright (C) 2001-2002 Hewlett-Packard Co
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

#ifndef unwind_i_h
#define unwind_i_h

#include <memory.h>
#include <stdint.h>

#include <libunwind-ia64.h>

#include "config.h"
#include "internal.h"
#include "mempool.h"

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

#include "tdep.h"

struct ia64_cursor
  {
    void *as_arg;		/* argument to address-space callbacks */
    unw_addr_space_t as;	/* reference to per-address-space info */

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
    unw_word_t sigcontext_loc;	/* location of sigcontext or NULL */
    unw_word_t is_signal_frame;	/* is this a signal trampoline frame? */

    short hint;
    short prev_script;

    int pi_valid : 1;		/* is proc_info valid? */
    int pi_is_dynamic : 1;	/* proc_info found via dynamic proc info? */
    unw_proc_info_t pi;		/* info about current procedure */
};

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

#define IA64_REG_LOC(c,r)	((unw_word_t) _Uia64_uc_addr((c)->as_arg, (r)))
#define IA64_FPREG_LOC(c,r)						   \
	((unw_word_t) _Uia64_uc_addr((c)->as_arg, (r)) | IA64_LOC_TYPE_FP)

# define ia64_find_proc_info(c,ip,n)					 \
	_Uia64_find_proc_info(unw_local_addr_space, (ip), &(c)->pi, (n), \
			      (c)->as_arg)
# define ia64_put_unwind_info(c, pi)	do { ; } while (0)

/* Note: the register accessors (ia64_{get,set}{,fp}()) must check for
   NULL locations because _Uia64_uc_addr() returns NULL for unsaved
   registers.  */

static inline int
ia64_getfp (struct ia64_cursor *c, unw_word_t loc, unw_fpreg_t *val)
{
  if (!loc)
    return -1;
  *val = *(unw_fpreg_t *) IA64_MASK_LOC_TYPE(loc);
  return 0;
}

static inline int
ia64_putfp (struct ia64_cursor *c, unw_word_t loc, unw_fpreg_t val)
{
  if (!loc)
    return -1;
  *(unw_fpreg_t *) IA64_MASK_LOC_TYPE(loc) = val;
  return 0;
}

static inline int
ia64_get (struct ia64_cursor *c, unw_word_t loc, unw_word_t *val)
{
  if (!loc)
    return -1;
  *val = *(unw_word_t *) IA64_MASK_LOC_TYPE(loc);
  return 0;
}

static inline int
ia64_put (struct ia64_cursor *c, unw_word_t loc, unw_word_t val)
{
  if (!loc)
    return -1;
  *(unw_word_t *) IA64_MASK_LOC_TYPE(loc) = (val);
  return 0;
}

#else /* !UNW_LOCAL_ONLY */

#define IA64_IS_REG_LOC(r)	(((r) & IA64_LOC_TYPE_REG) != 0)
#define IA64_REG_LOC(c,r)	IA64_LOC((r), IA64_LOC_TYPE_REG)
#define IA64_FPREG_LOC(c,r)	IA64_LOC((r), (IA64_LOC_TYPE_REG	\
					       | IA64_LOC_TYPE_FP))

# define ia64_find_proc_info(c,ip,n)					\
	(*(c)->as->acc.find_proc_info)((c)->as, (ip), &(c)->pi, (n),	\
				       (c)->as_arg)
# define ia64_put_unwind_info(c,pi)					\
	(*(c)->as->acc.put_unwind_info)((c)->as, (pi), (c)->as_arg)

static inline int
ia64_getfp (struct ia64_cursor *c, unw_word_t loc, unw_fpreg_t *val)
{
  int ret;

  if (IA64_IS_REG_LOC (loc))
    return (*c->as->acc.access_fpreg) (c->as, IA64_GET_LOC (loc),
				       val, 0, c->as_arg);

  loc = IA64_MASK_LOC_TYPE(loc);
  ret = (*c->as->acc.access_mem) (c->as, loc + 0, &val->raw.bits[0], 0,
				  c->as_arg);
  if (ret < 0)
    return ret;

  return (*c->as->acc.access_mem) (c->as, loc + 8, &val->raw.bits[1], 0,
				   c->as_arg);
}

static inline int
ia64_putfp (struct ia64_cursor *c, unw_word_t loc, unw_fpreg_t val)
{
  int ret;

  if (IA64_IS_REG_LOC (loc))
    return (*c->as->acc.access_fpreg) (c->as, IA64_GET_LOC (loc), &val, 1,
				       c->as_arg);

  loc = IA64_MASK_LOC_TYPE(loc);
  ret = (*c->as->acc.access_mem) (c->as, loc + 0, &val.raw.bits[0], 1,
				  c->as_arg);
  if (ret < 0)
    return ret;

  return (*c->as->acc.access_mem) (c->as, loc + 8, &val.raw.bits[1], 1,
				   c->as_arg);
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

      if (c->as->big_endian)
	*val = tmp.raw.bits[1];
      else
	*val = tmp.raw.bits[0];
      return 0;
    }

  if (IA64_IS_REG_LOC (loc))
    return (*c->as->acc.access_reg)(c->as, IA64_GET_LOC (loc), val, 0,
				    c->as_arg);
  else
    return (*c->as->acc.access_mem)(c->as, loc, val, 0, c->as_arg);
}

static inline int
ia64_put (struct ia64_cursor *c, unw_word_t loc, unw_word_t val)
{
  if (IA64_IS_FP_LOC (loc))
    {
      unw_fpreg_t tmp;

      memset (&tmp, 0, sizeof (tmp));
      if (c->as->big_endian)
	tmp.raw.bits[1] = val;
      else
	tmp.raw.bits[0] = val;
      return ia64_putfp (c, loc, tmp);
    }

  if (IA64_IS_REG_LOC (loc))
    return (*c->as->acc.access_reg)(c->as, IA64_GET_LOC (loc), &val, 1,
				    c->as_arg);
  else
    return (*c->as->acc.access_mem)(c->as, loc, &val, 1, c->as_arg);
}

#endif /* !UNW_LOCAL_ONLY */

struct ia64_unwind_block
  {
    unw_word_t header;
    unw_word_t desc[0];			/* unwind descriptors */

    /* Personality routine and language-specific data follow behind
       descriptors.  */
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

struct ia64_reg_state
  {
    struct ia64_reg_state *next;    /* next (outer) element on state stack */
    struct ia64_reg_info reg[IA64_NUM_PREGS];	/* register save locations */
  };

struct ia64_state_record
  {
    unsigned int first_region : 1;	/* is this the first region? */
    unsigned int done : 1;		/* are we done scanning descriptors? */
    unsigned int any_spills : 1;	/* got any register spills? */
    unsigned int in_body : 1;		/* are we inside prologue or body? */
    unsigned int is_signal_frame : 1;

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

struct ia64_labeled_state
  {
    struct ia64_labeled_state *next;	/* next label (or NULL) */
    unsigned long label;			/* label for this state */
    struct ia64_reg_state saved_state;
  };

struct ia64_global_unwind_state
  {
    int first_time;

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

    struct mempool state_record_pool;
    struct mempool labeled_state_pool;

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
#define ia64_make_proc_info		UNW_OBJ(make_proc_info)
#define ia64_create_state_record	UNW_OBJ(create_state_record)
#define ia64_free_state_record		UNW_OBJ(free_state_record)
#define ia64_find_save_locs		UNW_OBJ(find_save_locs)
#define ia64_per_thread_cache		UNW_OBJ(per_thread_cache)
#define ia64_script_cache_init		UNW_OBJ(script_cache_init)
#define ia64_init			UNW_OBJ(init)
#define ia64_access_reg			UNW_OBJ(access_reg)
#define ia64_access_fpreg		UNW_OBJ(access_fpreg)
#define ia64_scratch_loc		UNW_OBJ(scratch_loc)
#define ia64_local_resume		UNW_OBJ(local_resume)

extern struct ia64_global_unwind_state unw;

extern int ia64_make_proc_info (struct ia64_cursor *c);
extern int ia64_create_state_record (struct ia64_cursor *c,
				     struct ia64_state_record *sr);
extern int ia64_free_state_record (struct ia64_state_record *sr);
extern int ia64_find_save_locs (struct ia64_cursor *c);
extern void ia64_script_cache_init (struct ia64_script_cache *cache);
extern void ia64_init (void);
extern int ia64_access_reg (struct ia64_cursor *c, unw_regnum_t reg,
			    unw_word_t *valp, int write);
extern int ia64_access_fpreg (struct ia64_cursor *c, unw_regnum_t reg,
			      unw_fpreg_t *valp, int write);
extern unw_word_t ia64_scratch_loc (struct ia64_cursor *c, unw_regnum_t reg);

extern void __ia64_install_context (const ucontext_t *ucp, long r15, long r16,
				    long r17, long r18)
	__attribute__ ((noreturn));
extern int ia64_local_resume (unw_addr_space_t as, unw_cursor_t *cursor,
			      void *arg);

extern int _Uia64_find_proc_info (unw_addr_space_t as, unw_word_t ip,
				  unw_proc_info_t *pi, int need_unwind_info,
				  void *arg);

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
