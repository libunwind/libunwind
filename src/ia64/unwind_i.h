/* libunwind - a platform-independent unwind library
   Copyright (C) 2001-2003 Hewlett-Packard Co
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
#include <inttypes.h>

#include <libunwind-ia64.h>

#include "config.h"
#include "internal.h"
#include "rse.h"

#define IA64_UNW_VER(x)			((x) >> 48)
#define IA64_UNW_FLAG_MASK		0x0000ffff00000000
#define IA64_UNW_FLAG_OSMASK		0x0000f00000000000
#define IA64_UNW_FLAG_EHANDLER(x)	((x) & 0x0000000100000000L)
#define IA64_UNW_FLAG_UHANDLER(x)	((x) & 0x0000000200000000L)
#define IA64_UNW_LENGTH(x)		((x) & 0x00000000ffffffffL)

#ifdef MIN
# undef MIN
#endif
#define MIN(a,b)	((a) < (b) ? (a) : (b))

#include "tdep.h"

/* Bits 0 and 1 of an location are used to encode its type:
	bit 0: set if location uses floating-point format.
	bit 1: set if location is a NaT bit on memory stack.  */

#define IA64_LOC_TYPE_FP		(1 << 0)
#define IA64_LOC_TYPE_MEMSTK_NAT	(1 << 1)

#ifdef UNW_LOCAL_ONLY

#define IA64_LOC_REG(r,t)	(((r) << 2) | (t))
#define IA64_LOC_ADDR(a,t)	(((a) & ~0x3) | (t))
#define IA64_LOC_UC_ADDR(a,t)	IA64_LOC_ADDR(a, t)
#define IA64_LOC_UC_REG(a,t)
#define IA64_NULL_LOC		(0)

#define IA64_GET_REG(l)		((l) >> 2)
#define IA64_GET_ADDR(l)	((l) & ~0x3)
#define IA64_IS_NULL_LOC(l)	((l) == 0)
#define IA64_IS_FP_LOC(l)	(((l) & IA64_LOC_TYPE_FP) != 0)
#define IA64_IS_MEMSTK_NAT(l)	(((l) & IA64_LOC_TYPE_MEMSTK_NAT) != 0)
#define IA64_IS_REG_LOC(l)	0
#define IA64_IS_UC_LOC(l)	0

#define IA64_REG_LOC(c,r)	((unw_word_t) tdep_uc_addr((c)->as_arg, (r)))
#define IA64_FPREG_LOC(c,r)						 \
	((unw_word_t) tdep_uc_addr((c)->as_arg, (r)) | IA64_LOC_TYPE_FP)

# define ia64_find_proc_info(c,ip,n)					\
	tdep_find_proc_info(unw_local_addr_space, (ip), &(c)->pi, (n),	\
			    (c)->as_arg)
# define ia64_put_unwind_info(c, pi)	do { ; } while (0)

/* Note: the register accessors (ia64_{get,set}{,fp}()) must check for
   NULL locations because tdep_uc_addr() returns NULL for unsaved
   registers.  */

static inline int
ia64_getfp (struct cursor *c, unw_word_t loc, unw_fpreg_t *val)
{
  if (!loc)
    {
      debug (150, "%s: access to unsaved register\n", __FUNCTION__);
      return -UNW_EBADREG;
    }
  *val = *(unw_fpreg_t *) IA64_GET_ADDR (loc);
  return 0;
}

static inline int
ia64_putfp (struct cursor *c, unw_word_t loc, unw_fpreg_t val)
{
  if (!loc)
    {
      debug (150, "%s: access to unsaved register\n", __FUNCTION__);
      return -UNW_EBADREG;
    }
  *(unw_fpreg_t *) IA64_GET_ADDR (loc) = val;
  return 0;
}

static inline int
ia64_get (struct cursor *c, unw_word_t loc, unw_word_t *val)
{
  if (!loc)
    {
      debug (150, "%s: access to unsaved register\n", __FUNCTION__);
      return -UNW_EBADREG;
    }
  *val = *(unw_word_t *) IA64_GET_ADDR (loc);
  return 0;
}

static inline int
ia64_put (struct cursor *c, unw_word_t loc, unw_word_t val)
{
  if (!loc)
    {
      debug (150, "%s: access to unsaved register\n", __FUNCTION__);
      return -UNW_EBADREG;
    }
  *(unw_word_t *) IA64_GET_ADDR (loc) = (val);
  return 0;
}

#else /* !UNW_LOCAL_ONLY */

/* Bits 0 and 1 of the second word (w1) of a location are used
   to further distinguish what location we're dealing with:

   	bit 0: set if the location is a register
	bit 1: set of the location is accessed via uc_access(3)  */
#define IA64_LOC_TYPE_REG	(1 << 0)
#define IA64_LOC_TYPE_UC	(1 << 1)

#define IA64_LOC_REG(r,t)	((ia64_loc_t) { ((r) << 2) | (t),	\
						IA64_LOC_TYPE_REG })
#define IA64_LOC_ADDR(a,t)	((ia64_loc_t) { ((a) & ~0x3) | (t), 0 })
#define IA64_LOC_UC_ADDR(a,t)	((ia64_loc_t) { ((a) & ~0x3) | (t),	\
						IA64_LOC_TYPE_UC })
#define IA64_LOC_UC_REG(r,a)	((ia64_loc_t) { ((r) << 2),		 \
						((a) | IA64_LOC_TYPE_REG \
					         | IA64_LOC_TYPE_UC) })
#define IA64_NULL_LOC		((ia64_loc_t) { 0, 0 })

#define IA64_GET_REG(l)		((l).w0 >> 2)
#define IA64_GET_ADDR(l)	((l).w0 & ~0x3)
#define IA64_GET_AUX_ADDR(l)	((l).w1 & ~0x3)
#define IA64_IS_NULL_LOC(l)	(((l).w0 | (l).w1) == 0)
#define IA64_IS_FP_LOC(l)	(((l).w0 & IA64_LOC_TYPE_FP) != 0)
#define IA64_IS_MEMSTK_NAT(l)	(((l).w0 & IA64_LOC_TYPE_MEMSTK_NAT) != 0)
#define IA64_IS_REG_LOC(l)	(((l).w1 & IA64_LOC_TYPE_REG) != 0)
#define IA64_IS_UC_LOC(l)	(((l).w1 & IA64_LOC_TYPE_UC) != 0)

#define IA64_REG_LOC(c,r)	IA64_LOC_REG ((r), 0)
#define IA64_FPREG_LOC(c,r)	IA64_LOC_REG ((r), IA64_LOC_TYPE_FP)

# define ia64_find_proc_info(c,ip,n)					\
	(*(c)->as->acc.find_proc_info)((c)->as, (ip), &(c)->pi, (n),	\
				       (c)->as_arg)
# define ia64_put_unwind_info(c,pi)					\
	(*(c)->as->acc.put_unwind_info)((c)->as, (pi), (c)->as_arg)

#define ia64_uc_access_reg	UNW_OBJ(uc_access_reg)
#define ia64_uc_access_fpreg	UNW_OBJ(uc_access_fpreg)

extern int ia64_uc_access_reg (struct cursor *c, ia64_loc_t loc,
			       unw_word_t *valp, int write);
extern int ia64_uc_access_fpreg (struct cursor *c, ia64_loc_t loc,
				 unw_fpreg_t *valp, int write);

static inline int
ia64_getfp (struct cursor *c, ia64_loc_t loc, unw_fpreg_t *val)
{
  unw_word_t addr;
  int ret;

  if (IA64_IS_UC_LOC (loc))
    return ia64_uc_access_fpreg (c, loc, val, 0);

  if (IA64_IS_REG_LOC (loc))
    return (*c->as->acc.access_fpreg) (c->as, IA64_GET_REG (loc),
				       val, 0, c->as_arg);

  addr = IA64_GET_ADDR (loc);
  ret = (*c->as->acc.access_mem) (c->as, addr + 0, &val->raw.bits[0], 0,
				  c->as_arg);
  if (ret < 0)
    return ret;

  return (*c->as->acc.access_mem) (c->as, addr + 8, &val->raw.bits[1], 0,
				   c->as_arg);
}

static inline int
ia64_putfp (struct cursor *c, ia64_loc_t loc, unw_fpreg_t val)
{
  unw_word_t addr;
  int ret;

  if (IA64_IS_UC_LOC (loc))
    return ia64_uc_access_fpreg (c, loc, &val, 1);

  if (IA64_IS_REG_LOC (loc))
    return (*c->as->acc.access_fpreg) (c->as, IA64_GET_REG (loc), &val, 1,
				       c->as_arg);

  addr = IA64_GET_ADDR (loc);
  ret = (*c->as->acc.access_mem) (c->as, addr + 0, &val.raw.bits[0], 1,
				  c->as_arg);
  if (ret < 0)
    return ret;

  return (*c->as->acc.access_mem) (c->as, addr + 8, &val.raw.bits[1], 1,
				   c->as_arg);
}

/* Get the 64 data bits from location LOC.  If bit 0 is cleared, LOC
   is a memory address, otherwise it is a register number.  If the
   register is a floating-point register, the 64 bits are read from
   the significand bits.  */

static inline int
ia64_get (struct cursor *c, ia64_loc_t loc, unw_word_t *val)
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

  if (IA64_IS_UC_LOC (loc))
    return ia64_uc_access_reg (c, loc, val, 0);

  if (IA64_IS_REG_LOC (loc))
    return (*c->as->acc.access_reg)(c->as, IA64_GET_REG (loc), val, 0,
				    c->as_arg);
  else
    return (*c->as->acc.access_mem)(c->as, IA64_GET_ADDR (loc), val, 0,
				    c->as_arg);
}

static inline int
ia64_put (struct cursor *c, ia64_loc_t loc, unw_word_t val)
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

  if (IA64_IS_UC_LOC (loc))
    return ia64_uc_access_reg (c, loc, &val, 1);

  if (IA64_IS_REG_LOC (loc))
    return (*c->as->acc.access_reg)(c->as, IA64_GET_REG (loc), &val, 1,
				    c->as_arg);
  else
    return (*c->as->acc.access_mem)(c->as, IA64_GET_ADDR (loc), &val, 1,
				    c->as_arg);
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
    uint8_t *imask;		/* imask of spill_mask record or NULL */
    uint16_t abi_marker;

    unw_word_t pr_val;		/* predicate values */
    unw_word_t pr_mask;		/* predicate mask */

    long spill_offset;		/* psp-relative offset for spill base */
    int region_start;
    int region_len;
    int when_sp_restored;
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

/* Convenience macros: */
#define ia64_make_proc_info		UNW_OBJ(make_proc_info)
#define ia64_fetch_proc_info		UNW_OBJ(fetch_proc_info)
#define ia64_create_state_record	UNW_OBJ(create_state_record)
#define ia64_free_state_record		UNW_OBJ(free_state_record)
#define ia64_find_save_locs		UNW_OBJ(find_save_locs)
#define ia64_per_thread_cache		UNW_OBJ(per_thread_cache)
#define ia64_access_reg			UNW_OBJ(access_reg)
#define ia64_access_fpreg		UNW_OBJ(access_fpreg)
#define ia64_scratch_loc		UNW_OBJ(scratch_loc)
#define ia64_local_resume		UNW_OBJ(local_resume)
#define ia64_local_addr_space_init	UNW_OBJ(local_addr_space_init)
#define ia64_strloc			UNW_OBJ(strloc)
#define ia64_install_cursor		UNW_OBJ(install_cursor)
#define rbs_switch			UNW_OBJ(rbs_switch)
#define rbs_find_stacked		UNW_OBJ(rbs_find_stacked)
#define rbs_cover_and_flush		UNW_OBJ(rbs_cover_and_flush)
#define ia64_init			UNW_ARCH_OBJ(init)

extern int ia64_make_proc_info (struct cursor *c);
extern int ia64_fetch_proc_info (struct cursor *c, unw_word_t ip,
				 int need_unwind_info);
/* The proc-info must be valid for IP before this routine can be
   called:  */
extern int ia64_create_state_record (struct cursor *c,
				     struct ia64_state_record *sr);
extern int ia64_free_state_record (struct ia64_state_record *sr);
extern int ia64_find_save_locs (struct cursor *c);
extern void ia64_local_addr_space_init (void);
extern void ia64_init (void);
extern int ia64_access_reg (struct cursor *c, unw_regnum_t reg,
			    unw_word_t *valp, int write);
extern int ia64_access_fpreg (struct cursor *c, unw_regnum_t reg,
			      unw_fpreg_t *valp, int write);
extern ia64_loc_t ia64_scratch_loc (struct cursor *c, unw_regnum_t reg);

extern void ia64_install_cursor (struct cursor *c, unw_word_t pri_unat,
				 unw_word_t *extra)
	__attribute__ ((noreturn));
extern int ia64_local_resume (unw_addr_space_t as, unw_cursor_t *cursor,
			      void *arg);
extern int rbs_switch (struct cursor *c,
		       unw_word_t saved_bsp, unw_word_t saved_bspstore,
		       ia64_loc_t saved_rnat_loc);
extern int rbs_find_stacked (struct cursor *c, unw_word_t regs_to_skip,
			     ia64_loc_t *locp, ia64_loc_t *rnat_locp);
extern int rbs_cover_and_flush (struct cursor *c, unw_word_t nregs);

/* Warning: ia64_strloc() is for debugging only and it is NOT re-entrant! */
extern const char *ia64_strloc (ia64_loc_t loc);

/* Return true if the register-backing store is inside a ucontext_t
   that needs to be accessed via uc_access(3).  */

static inline int
rbs_on_uc (struct rbs_area *rbs)
{
  return IA64_IS_UC_LOC (rbs->rnat_loc) && !IA64_IS_REG_LOC (rbs->rnat_loc);
}

/* Return true if BSP points to a word that's stored on register
   backing-store RBS.  */
static inline int
rbs_contains (struct rbs_area *rbs, unw_word_t bsp)
{
  int result;

  /* Caveat: this takes advantage of unsigned arithmetic.  The full
     test is (bsp >= rbs->end - rbs->size) && (bsp < rbs->end).  We
     take advantage of the fact that -n == ~n + 1.  */
  result = bsp - rbs->end > ~rbs->size;
  debug (150, "%s: 0x%lx in [0x%lx-0x%lx) => %d\n", __FUNCTION__,
	 (long) bsp, (long) (rbs->end - rbs->size), (long) rbs->end, result);
  return result;
}

static inline ia64_loc_t
rbs_get_rnat_loc (struct rbs_area *rbs, unw_word_t bsp)
{
  unw_word_t rnat_addr = ia64_rse_rnat_addr (bsp);
  ia64_loc_t rnat_loc;

  if (rbs_contains (rbs, rnat_addr))
    {
      if (rbs_on_uc (rbs))
	rnat_loc = IA64_LOC_UC_ADDR (rnat_addr, 0);
      else
	rnat_loc = IA64_LOC_ADDR (rnat_addr, 0);
    }
  else
    rnat_loc = rbs->rnat_loc;
  return rnat_loc;
}

static inline ia64_loc_t
rbs_loc (struct rbs_area *rbs, unw_word_t bsp)
{
  if (rbs_on_uc (rbs))
    return IA64_LOC_UC_ADDR (bsp, 0);
  else
    return IA64_LOC_ADDR (bsp, 0);
}

static inline int
ia64_get_stacked (struct cursor *c, unw_word_t reg,
		  ia64_loc_t *locp, ia64_loc_t *rnat_locp)
{
  struct rbs_area *rbs = c->rbs_area + c->rbs_curr;
  unw_word_t addr, regs_to_skip = reg - 32;
  int ret = 0;

  assert (reg >= 32 && reg < 128);

  addr = ia64_rse_skip_regs (c->bsp, regs_to_skip);
  if (locp)
    *locp = rbs_loc (rbs, addr);
  if (rnat_locp)
    *rnat_locp = rbs_get_rnat_loc (rbs, addr);

  if (!rbs_contains (rbs, addr))
    ret = rbs_find_stacked (c, regs_to_skip, locp, rnat_locp);
  return ret;
}

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
