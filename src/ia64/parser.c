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

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "unwind_i.h"

typedef unsigned long unw_word;

#define alloc_reg_state()	(malloc (sizeof(struct ia64_state_record)))
#define free_reg_state(usr)	(free (usr))

/* Unwind decoder routines */

static inline void
push (struct ia64_state_record *sr)
{
  struct ia64_reg_state *rs;

  rs = alloc_reg_state ();
  if (!rs)
    {
      fprintf (stderr, "unwind: cannot stack reg state!\n");
      return;
    }
  memcpy (rs, &sr->curr, sizeof (*rs));
  rs->next = sr->stack;
  sr->stack = rs;
}

static void
pop (struct ia64_state_record *sr)
{
  struct ia64_reg_state *rs;

  if (!sr->stack)
    {
      fprintf (stderr, "unwind: stack underflow!\n");
      return;
    }
  rs = sr->stack;
  sr->stack = rs->next;
  free_reg_state (rs);
}

static enum ia64_pregnum __attribute__ ((const))
decode_abreg (unsigned char abreg, int memory)
{
  switch (abreg)
    {
    case 0x04 ... 0x07:
      return IA64_REG_R4 + (abreg - 0x04);
    case 0x22 ... 0x25:
      return IA64_REG_F2 + (abreg - 0x22);
    case 0x30 ... 0x3f:
      return IA64_REG_F16 + (abreg - 0x30);
    case 0x41 ... 0x45:
      return IA64_REG_B1 + (abreg - 0x41);
    case 0x60:
      return IA64_REG_PR;
    case 0x61:
      return IA64_REG_PSP;
    case 0x62:
      return memory ? IA64_REG_PRI_UNAT_MEM : IA64_REG_PRI_UNAT_GR;
    case 0x63:
      return IA64_REG_RP;
    case 0x64:
      return IA64_REG_BSP;
    case 0x65:
      return IA64_REG_BSPSTORE;
    case 0x66:
      return IA64_REG_RNAT;
    case 0x67:
      return IA64_REG_UNAT;
    case 0x68:
      return IA64_REG_FPSR;
    case 0x69:
      return IA64_REG_PFS;
    case 0x6a:
      return IA64_REG_LC;
    default:
      break;
    }
  dprintf ("unwind: bad abreg=0x%x\n", abreg);
  return IA64_REG_LC;
}

static void
set_reg (struct ia64_reg_info *reg, enum ia64_where where, int when,
	 unsigned long val)
{
  reg->val = val;
  reg->where = where;
  if (reg->when == IA64_WHEN_NEVER)
    reg->when = when;
}

static void
alloc_spill_area (unsigned long *offp, unsigned long regsize,
		  struct ia64_reg_info *lo, struct ia64_reg_info *hi)
{
  struct ia64_reg_info *reg;

  for (reg = hi; reg >= lo; --reg)
    {
      if (reg->where == IA64_WHERE_SPILL_HOME)
	{
	  reg->where = IA64_WHERE_PSPREL;
	  reg->val = 0x10 - *offp;
	  *offp += regsize;
	}
    }
}

static inline void
spill_next_when (struct ia64_reg_info **regp, struct ia64_reg_info *lim,
		 unw_word t)
{
  struct ia64_reg_info *reg;

  for (reg = *regp; reg <= lim; ++reg)
    {
      if (reg->where == IA64_WHERE_SPILL_HOME)
	{
	  reg->when = t;
	  *regp = reg + 1;
	  return;
	}
    }
  dprintf ("unwind: excess spill!\n");
}

static inline void
finish_prologue (struct ia64_state_record *sr)
{
  struct ia64_reg_info *reg;
  unsigned long off;
  int i;

  /* First, resolve implicit register save locations (see Section
     "11.4.2.3 Rules for Using Unwind Descriptors", rule 3). */
  for (i = 0; i < (int) sizeof (unw.save_order) / sizeof (unw.save_order[0]);
       ++i)
    {
      reg = sr->curr.reg + unw.save_order[i];
      if (reg->where == IA64_WHERE_GR_SAVE)
	{
	  reg->where = IA64_WHERE_GR;
	  reg->val = sr->gr_save_loc++;
	}
    }

  /* Next, compute when the fp, general, and branch registers get
     saved.  This must come before alloc_spill_area() because we need
     to know which registers are spilled to their home locations.  */

  if (sr->imask)
    {
      unsigned char kind, mask = 0, *cp = sr->imask;
      unsigned long t;
      static const unsigned char limit[3] =
        {
	  IA64_REG_F31, IA64_REG_R7, IA64_REG_B5
	};
      struct ia64_reg_info *(regs[3]);

      regs[0] = sr->curr.reg + IA64_REG_F2;
      regs[1] = sr->curr.reg + IA64_REG_R4;
      regs[2] = sr->curr.reg + IA64_REG_B1;

      for (t = 0; t < sr->region_len; ++t)
	{
	  if ((t & 3) == 0)
	    mask = *cp++;
	  kind = (mask >> 2 * (3 - (t & 3))) & 3;
	  if (kind > 0)
	    spill_next_when (&regs[kind - 1], sr->curr.reg + limit[kind - 1],
			     sr->region_start + t);
	}
    }

  /* Next, lay out the memory stack spill area.  */

  if (sr->any_spills)
    {
      off = sr->spill_offset;
      alloc_spill_area (&off, 16, sr->curr.reg + IA64_REG_F2,
			sr->curr.reg + IA64_REG_F31);
      alloc_spill_area (&off, 8, sr->curr.reg + IA64_REG_B1,
			sr->curr.reg + IA64_REG_B5);
      alloc_spill_area (&off, 8, sr->curr.reg + IA64_REG_R4,
			sr->curr.reg + IA64_REG_R7);
    }
}

/* Region header descriptors.  */

static void
desc_prologue (int body, unw_word rlen, unsigned char mask,
	       unsigned char grsave, struct ia64_state_record *sr)
{
  int i;

  if (!(sr->in_body || sr->first_region))
    finish_prologue (sr);
  sr->first_region = 0;

  /* check if we're done: */
  if (body && sr->when_target < sr->region_start + sr->region_len)
    {
      sr->done = 1;
      return;
    }

  for (i = 0; i < sr->epilogue_count; ++i)
    pop (sr);
  sr->epilogue_count = 0;
  sr->epilogue_start = IA64_WHEN_NEVER;

  if (!body)
    push (sr);

  sr->region_start += sr->region_len;
  sr->region_len = rlen;
  sr->in_body = body;

  if (!body)
    {
      for (i = 0; i < 4; ++i)
	{
	  if (mask & 0x8)
	    set_reg (sr->curr.reg + unw.save_order[i], IA64_WHERE_GR,
		     sr->region_start + sr->region_len - 1, grsave++);
	  mask <<= 1;
	}
      sr->gr_save_loc = grsave;
      sr->any_spills = 0;
      sr->imask = 0;
      sr->spill_offset = 0x10;	/* default to psp+16 */
    }
}

/* Prologue descriptors.  */

static inline void
desc_abi (unsigned char abi, unsigned char context,
	  struct ia64_state_record *sr)
{
  if (abi == 0 && context == 's')
    sr->flags |= IA64_FLAG_SIGTRAMP;
  else
    dprintf ("unwind: ignoring unwabi(abi=0x%x,context=0x%x)\n", abi, context);
}

static inline void
desc_br_gr (unsigned char brmask, unsigned char gr,
	    struct ia64_state_record *sr)
{
  int i;

  for (i = 0; i < 5; ++i)
    {
      if (brmask & 1)
	set_reg (sr->curr.reg + IA64_REG_B1 + i, IA64_WHERE_GR,
		 sr->region_start + sr->region_len - 1, gr++);
      brmask >>= 1;
    }
}

static inline void
desc_br_mem (unsigned char brmask, struct ia64_state_record *sr)
{
  int i;

  for (i = 0; i < 5; ++i)
    {
      if (brmask & 1)
	{
	  set_reg (sr->curr.reg + IA64_REG_B1 + i, IA64_WHERE_SPILL_HOME,
		   sr->region_start + sr->region_len - 1, 0);
	  sr->any_spills = 1;
	}
      brmask >>= 1;
    }
}

static inline void
desc_frgr_mem (unsigned char grmask, unw_word frmask,
	       struct ia64_state_record *sr)
{
  int i;

  for (i = 0; i < 4; ++i)
    {
      if ((grmask & 1) != 0)
	{
	  set_reg (sr->curr.reg + IA64_REG_R4 + i, IA64_WHERE_SPILL_HOME,
		   sr->region_start + sr->region_len - 1, 0);
	  sr->any_spills = 1;
	}
      grmask >>= 1;
    }
  for (i = 0; i < 20; ++i)
    {
      if ((frmask & 1) != 0)
	{
	  set_reg (sr->curr.reg + IA64_REG_F2 + i, IA64_WHERE_SPILL_HOME,
		   sr->region_start + sr->region_len - 1, 0);
	  sr->any_spills = 1;
	}
      frmask >>= 1;
    }
}

static inline void
desc_fr_mem (unsigned char frmask, struct ia64_state_record *sr)
{
  int i;

  for (i = 0; i < 4; ++i)
    {
      if ((frmask & 1) != 0)
	{
	  set_reg (sr->curr.reg + IA64_REG_F2 + i, IA64_WHERE_SPILL_HOME,
		   sr->region_start + sr->region_len - 1, 0);
	  sr->any_spills = 1;
	}
      frmask >>= 1;
    }
}

static inline void
desc_gr_gr (unsigned char grmask, unsigned char gr,
	    struct ia64_state_record *sr)
{
  int i;

  for (i = 0; i < 4; ++i)
    {
      if ((grmask & 1) != 0)
	set_reg (sr->curr.reg + IA64_REG_R4 + i, IA64_WHERE_GR,
		 sr->region_start + sr->region_len - 1, gr++);
      grmask >>= 1;
    }
}

static inline void
desc_gr_mem (unsigned char grmask, struct ia64_state_record *sr)
{
  int i;

  for (i = 0; i < 4; ++i)
    {
      if ((grmask & 1) != 0)
	{
	  set_reg (sr->curr.reg + IA64_REG_R4 + i, IA64_WHERE_SPILL_HOME,
		   sr->region_start + sr->region_len - 1, 0);
	  sr->any_spills = 1;
	}
      grmask >>= 1;
    }
}

static inline void
desc_mem_stack_f (unw_word t, unw_word size, struct ia64_state_record *sr)
{
  set_reg (sr->curr.reg + IA64_REG_PSP, IA64_WHERE_NONE,
	   sr->region_start + MIN ((int) t, sr->region_len - 1), 16 * size);
}

static inline void
desc_mem_stack_v (unw_word t, struct ia64_state_record *sr)
{
  sr->curr.reg[IA64_REG_PSP].when =
    sr->region_start + MIN ((int) t, sr->region_len - 1);
}

static inline void
desc_reg_gr (unsigned char reg, unsigned char dst,
	     struct ia64_state_record *sr)
{
  set_reg (sr->curr.reg + reg, IA64_WHERE_GR,
	   sr->region_start + sr->region_len - 1, dst);
}

static inline void
desc_reg_psprel (unsigned char reg, unw_word pspoff,
		 struct ia64_state_record *sr)
{
  set_reg (sr->curr.reg + reg, IA64_WHERE_PSPREL,
	   sr->region_start + sr->region_len - 1, 0x10 - 4 * pspoff);
}

static inline void
desc_reg_sprel (unsigned char reg, unw_word spoff,
		struct ia64_state_record *sr)
{
  set_reg (sr->curr.reg + reg, IA64_WHERE_SPREL,
	   sr->region_start + sr->region_len - 1, 4 * spoff);
}

static inline void
desc_rp_br (unsigned char dst, struct ia64_state_record *sr)
{
  sr->return_link_reg = dst;
}

static inline void
desc_reg_when (unsigned char regnum, unw_word t, struct ia64_state_record *sr)
{
  struct ia64_reg_info *reg = sr->curr.reg + regnum;

  if (reg->where == IA64_WHERE_NONE)
    reg->where = IA64_WHERE_GR_SAVE;
  reg->when = sr->region_start + MIN ((int) t, sr->region_len - 1);
}

static inline void
desc_spill_base (unw_word pspoff, struct ia64_state_record *sr)
{
  sr->spill_offset = 0x10 - 4 * pspoff;
}

static inline unsigned char *
desc_spill_mask (unsigned char *imaskp, struct ia64_state_record *sr)
{
  sr->imask = imaskp;
  return imaskp + (2 * sr->region_len + 7) / 8;
}

/* Body descriptors.  */

static inline void
desc_epilogue (unw_word t, unw_word ecount, struct ia64_state_record *sr)
{
  sr->epilogue_start = sr->region_start + sr->region_len - 1 - t;
  sr->epilogue_count = ecount + 1;
}

static inline void
desc_copy_state (unw_word label, struct ia64_state_record *sr)
{
  struct ia64_reg_state *rs;

  for (rs = sr->reg_state_list; rs; rs = rs->next)
    {
      if (rs->label == label)
	{
	  memcpy (&sr->curr, rs, sizeof (sr->curr));
	  return;
	}
    }
  fprintf (stderr, "unwind: failed to find state labelled 0x%lx\n", label);
}

static inline void
desc_label_state (unw_word label, struct ia64_state_record *sr)
{
  struct ia64_reg_state *rs;

  rs = alloc_reg_state ();
  if (!rs)
    {
      fprintf (stderr, "unwind: cannot stack!\n");
      return;
    }
  memcpy (rs, &sr->curr, sizeof (*rs));
  rs->label = label;
  rs->next = sr->reg_state_list;
  sr->reg_state_list = rs;
}

/** General descriptors.  */

static inline int
desc_is_active (unsigned char qp, unw_word t, struct ia64_state_record *sr)
{
  if (sr->when_target <= sr->region_start + MIN ((int) t, sr->region_len - 1))
    return 0;
  if (qp > 0)
    {
      if ((sr->pr_val & (1UL << qp)) == 0)
	return 0;
      sr->pr_mask |= (1UL << qp);
    }
  return 1;
}

static inline void
desc_restore_p (unsigned char qp, unw_word t, unsigned char abreg,
		struct ia64_state_record *sr)
{
  struct ia64_reg_info *r;

  if (!desc_is_active (qp, t, sr))
    return;

  r = sr->curr.reg + decode_abreg (abreg, 0);
  r->where = IA64_WHERE_NONE;
  r->when = IA64_WHEN_NEVER;
  r->val = 0;
}

static inline void
desc_spill_reg_p (unsigned char qp, unw_word t, unsigned char abreg,
		  unsigned char x, unsigned char ytreg,
		  struct ia64_state_record *sr)
{
  enum ia64_where where = IA64_WHERE_GR;
  struct ia64_reg_info *r;

  if (!desc_is_active (qp, t, sr))
    return;

  if (x)
    where = IA64_WHERE_BR;
  else if (ytreg & 0x80)
    where = IA64_WHERE_FR;

  r = sr->curr.reg + decode_abreg (abreg, 0);
  r->where = where;
  r->when = sr->region_start + MIN ((int) t, sr->region_len - 1);
  r->val = (ytreg & 0x7f);
}

static inline void
desc_spill_psprel_p (unsigned char qp, unw_word t, unsigned char abreg,
		     unw_word pspoff, struct ia64_state_record *sr)
{
  struct ia64_reg_info *r;

  if (!desc_is_active (qp, t, sr))
    return;

  r = sr->curr.reg + decode_abreg (abreg, 1);
  r->where = IA64_WHERE_PSPREL;
  r->when = sr->region_start + MIN ((int) t, sr->region_len - 1);
  r->val = 0x10 - 4 * pspoff;
}

static inline void
desc_spill_sprel_p (unsigned char qp, unw_word t, unsigned char abreg,
		    unw_word spoff, struct ia64_state_record *sr)
{
  struct ia64_reg_info *r;

  if (!desc_is_active (qp, t, sr))
    return;

  r = sr->curr.reg + decode_abreg (abreg, 1);
  r->where = IA64_WHERE_SPREL;
  r->when = sr->region_start + MIN ((int) t, sr->region_len - 1);
  r->val = 4 * spoff;
}

#define UNW_DEC_BAD_CODE(code)			fprintf (stderr, "unwind: unknown code 0x%02x\n", code);

/* Register names.  */
#define UNW_REG_BSP		IA64_REG_BSP
#define UNW_REG_BSPSTORE	IA64_REG_BSPSTORE
#define UNW_REG_FPSR		IA64_REG_FPSR
#define UNW_REG_LC		IA64_REG_LC
#define UNW_REG_PFS		IA64_REG_PFS
#define UNW_REG_PR		IA64_REG_PR
#define UNW_REG_RNAT		IA64_REG_RNAT
#define UNW_REG_PSP		IA64_REG_PSP
#define UNW_REG_RP		IA64_REG_RP
#define UNW_REG_UNAT		IA64_REG_UNAT

/* Region headers.  */
#define UNW_DEC_PROLOGUE_GR(fmt,r,m,gr,arg)	desc_prologue(0,r,m,gr,arg)
#define UNW_DEC_PROLOGUE(fmt,b,r,arg)		desc_prologue(b,r,0,32,arg)

/* Prologue descriptors.  */
#define UNW_DEC_ABI(fmt,a,c,arg)		desc_abi(a,c,arg)
#define UNW_DEC_BR_GR(fmt,b,g,arg)		desc_br_gr(b,g,arg)
#define UNW_DEC_BR_MEM(fmt,b,arg)		desc_br_mem(b,arg)
#define UNW_DEC_FRGR_MEM(fmt,g,f,arg)		desc_frgr_mem(g,f,arg)
#define UNW_DEC_FR_MEM(fmt,f,arg)		desc_fr_mem(f,arg)
#define UNW_DEC_GR_GR(fmt,m,g,arg)		desc_gr_gr(m,g,arg)
#define UNW_DEC_GR_MEM(fmt,m,arg)		desc_gr_mem(m,arg)
#define UNW_DEC_MEM_STACK_F(fmt,t,s,arg)	desc_mem_stack_f(t,s,arg)
#define UNW_DEC_MEM_STACK_V(fmt,t,arg)		desc_mem_stack_v(t,arg)
#define UNW_DEC_REG_GR(fmt,r,d,arg)		desc_reg_gr(r,d,arg)
#define UNW_DEC_REG_PSPREL(fmt,r,o,arg)		desc_reg_psprel(r,o,arg)
#define UNW_DEC_REG_SPREL(fmt,r,o,arg)		desc_reg_sprel(r,o,arg)
#define UNW_DEC_REG_WHEN(fmt,r,t,arg)		desc_reg_when(r,t,arg)
#define UNW_DEC_PRIUNAT_WHEN_GR(fmt,t,arg) \
	desc_reg_when(IA64_REG_PRI_UNAT_GR,t,arg)
#define UNW_DEC_PRIUNAT_WHEN_MEM(fmt,t,arg) \
	desc_reg_when(IA64_REG_PRI_UNAT_MEM,t,arg)
#define UNW_DEC_PRIUNAT_GR(fmt,r,arg) \
	desc_reg_gr(IA64_REG_PRI_UNAT_GR,r,arg)
#define UNW_DEC_PRIUNAT_PSPREL(fmt,o,arg) \
	desc_reg_psprel(IA64_REG_PRI_UNAT_MEM,o,arg)
#define UNW_DEC_PRIUNAT_SPREL(fmt,o,arg) \
	desc_reg_sprel(IA64_REG_PRI_UNAT_MEM,o,arg)
#define UNW_DEC_RP_BR(fmt,d,arg)		desc_rp_br(d,arg)
#define UNW_DEC_SPILL_BASE(fmt,o,arg)		desc_spill_base(o,arg)
#define UNW_DEC_SPILL_MASK(fmt,m,arg)		(m = desc_spill_mask(m,arg))

/* Body descriptors.  */
#define UNW_DEC_EPILOGUE(fmt,t,c,arg)		desc_epilogue(t,c,arg)
#define UNW_DEC_COPY_STATE(fmt,l,arg)		desc_copy_state(l,arg)
#define UNW_DEC_LABEL_STATE(fmt,l,arg)		desc_label_state(l,arg)

/* General unwind descriptors.  */
#define UNW_DEC_SPILL_REG_P(f,p,t,a,x,y,arg)	desc_spill_reg_p(p,t,a,x,y,arg)
#define UNW_DEC_SPILL_REG(f,t,a,x,y,arg)	desc_spill_reg_p(0,t,a,x,y,arg)
#define UNW_DEC_SPILL_PSPREL_P(f,p,t,a,o,arg) \
	desc_spill_psprel_p(p,t,a,o,arg)
#define UNW_DEC_SPILL_PSPREL(f,t,a,o,arg) \
	desc_spill_psprel_p(0,t,a,o,arg)
#define UNW_DEC_SPILL_SPREL_P(f,p,t,a,o,arg)	desc_spill_sprel_p(p,t,a,o,arg)
#define UNW_DEC_SPILL_SPREL(f,t,a,o,arg)	desc_spill_sprel_p(0,t,a,o,arg)
#define UNW_DEC_RESTORE_P(f,p,t,a,arg)		desc_restore_p(p,t,a,arg)
#define UNW_DEC_RESTORE(f,t,a,arg)		desc_restore_p(0,t,a,arg)

#include "unwind_decoder.c"

static inline const struct ia64_unwind_table_entry *
lookup (struct ia64_unwind_table *table, unw_word_t rel_ip)
{
  const struct ia64_unwind_table_entry *e = 0;
  unsigned long lo, hi, mid;

  /* do a binary search for right entry: */
  for (lo = 0, hi = table->info.length; lo < hi;)
    {
      mid = (lo + hi) / 2;
      e = &table->info.array[mid];
      if (rel_ip < e->start_offset)
	hi = mid;
      else if (rel_ip >= e->end_offset)
	lo = mid + 1;
      else
	break;
    }
  return e;
}

static int
get_proc_info (struct ia64_cursor *c)
{
  const struct ia64_unwind_table_entry *e = 0;
  struct ia64_unwind_table *table;
  unw_word_t segbase, len;
  uint8_t *dp, *desc_end;
  unw_ia64_table_t info;
  unw_word_t ip = c->ip;
  uint64_t hdr;
  int ret;

  /* search the kernels and the modules' unwind tables for IP: */

  for (table = unw.tables; table; table = table->next)
    if (ip >= table->start && ip < table->end)
      break;

  if (!table)
    {
      ret = ia64_acquire_unwind_info (c, ip, &info);
      if (ret < 0)
	return ret;

      segbase = info.segbase;
      len = info.length;

      /* XXX avoid malloc: */
      table = malloc (sizeof (struct ia64_unwind_table));
      if (!table)
	{
	  dprintf ("%s: out of memory\n", __FUNCTION__);
	  return -1;
	}
      memset (table, 0, sizeof (*table));
      table->info = info;
      table->start = segbase + table->info.array[0].start_offset;
      table->end   = segbase + table->info.array[len - 1].end_offset;

      /* XXX LOCK { */
      table->next = unw.tables;
      unw.tables = table;
      /* XXX LOCK } */
    }

  assert (ip >= table->start && ip < table->end);

  e = lookup (table, ip - table->info.segbase);

  hdr = *(uint64_t *) (table->info.unwind_info_base + e->info_offset);
  dp = (uint8_t *) (table->info.unwind_info_base + e->info_offset + 8);
  desc_end = dp + 8 * IA64_UNW_LENGTH (hdr);

  c->pi.flags = 0;
  c->pi.gp = table->info.gp;
  c->pi.proc_start = table->info.segbase + e->start_offset;
  c->pi.pers_addr = (uint64_t *) desc_end;
  c->pi.desc = dp;

  /* XXX Perhaps check UNW_VER / UNW_FLAG_OSMASK ? */
  if (IA64_UNW_FLAG_EHANDLER (hdr) | IA64_UNW_FLAG_UHANDLER (hdr))
    c->pi.flags |= IA64_FLAG_HAS_HANDLER;

  return 0;
}

int
ia64_create_state_record (struct ia64_cursor *c, struct ia64_state_record *sr)
{
  unw_word_t ip = c->ip, predicates = c->pr;
  struct ia64_reg_info *r;
  uint8_t *dp, *desc_end;
  int ret;
  STAT(unsigned long start;)
  STAT(++unw.stat.parse.calls; start = ia64_get_itc ());

  /* build state record */
  memset (sr, 0, sizeof (*sr));
  for (r = sr->curr.reg; r < sr->curr.reg + IA64_NUM_PREGS; ++r)
    r->when = IA64_WHEN_NEVER;
  sr->pr_val = predicates;

  ret = get_proc_info (c);
  if (ret < 0)
    return ret;

  if (!c->pi.desc)
    {
      /* No info, return default unwinder (leaf proc, no mem stack, no
         saved regs).  */
      dprintf ("unwind: no unwind info for ip=0x%lx\n", ip);
      sr->curr.reg[IA64_REG_RP].where = IA64_WHERE_BR;
      sr->curr.reg[IA64_REG_RP].when = -1;
      sr->curr.reg[IA64_REG_RP].val = 0;
      STAT(unw.stat.parse.time += ia64_get_itc () - start);
      return 0;
    }

  sr->when_target = (3 * ((ip & ~0xfUL) - c->pi.proc_start)
		     / 16 + (ip & 0xfUL));

  dp = c->pi.desc;
  desc_end = (uint8_t *) c->pi.pers_addr;
  while (!sr->done && dp < desc_end)
    dp = unw_decode (dp, sr->in_body, sr);

  c->pi.flags |= sr->flags;

  if (sr->when_target > sr->epilogue_start)
    {
      /* sp has been restored and all values on the memory stack below
	 psp also have been restored.  */
      sr->curr.reg[IA64_REG_PSP].val = 0;
      sr->curr.reg[IA64_REG_PSP].where = IA64_WHERE_NONE;
      sr->curr.reg[IA64_REG_PSP].when = IA64_WHEN_NEVER;
      for (r = sr->curr.reg; r < sr->curr.reg + IA64_NUM_PREGS; ++r)
	if ((r->where == IA64_WHERE_PSPREL && r->val <= 0x10)
	    || r->where == IA64_WHERE_SPREL)
	  {
	    r->val = 0;
	    r->where = IA64_WHERE_NONE;
	    r->when = IA64_WHEN_NEVER;
	  }
    }

  /* If RP did't get saved, generate entry for the return link
     register.  */
  if (sr->curr.reg[IA64_REG_RP].when >= sr->when_target)
    {
      sr->curr.reg[IA64_REG_RP].where = IA64_WHERE_BR;
      sr->curr.reg[IA64_REG_RP].when = -1;
      sr->curr.reg[IA64_REG_RP].val = sr->return_link_reg;
    }

#if IA64_UNW_DEBUG
  if (unw.debug_level > 0)
    {
      printf ("unwind: state record for func 0x%lx, t=%u:\n",
	      c->pi.proc_start, sr->when_target);
      for (r = sr->curr.reg; r < sr->curr.reg + IA64_NUM_PREGS; ++r)
	{
	  if (r->where != IA64_WHERE_NONE || r->when != IA64_WHEN_NEVER)
	    {
	      printf ("  %s <- ", unw.preg_name[r - sr->curr.reg]);
	      switch (r->where)
		{
		case IA64_WHERE_GR:
		  printf ("r%lu", r->val);
		  break;
		case IA64_WHERE_FR:
		  printf ("f%lu", r->val);
		  break;
		case IA64_WHERE_BR:
		  printf ("b%lu", r->val);
		  break;
		case IA64_WHERE_SPREL:
		  printf ("[sp+0x%lx]", r->val);
		  break;
		case IA64_WHERE_PSPREL:
		  printf ("[psp+0x%lx]", r->val);
		  break;
		case IA64_WHERE_NONE:
		  printf ("%s+0x%lx", unw.preg_name[r - sr->curr.reg], r->val);
		  break;
		default:
		  printf ("BADWHERE(%d)", r->where);
		  break;
		}
	      printf ("\t\t%d\n", r->when);
	    }
	}
    }
#endif
  return 0;
}

int
ia64_free_state_record (struct ia64_state_record *sr)
{
  struct ia64_reg_state *rs, *next;

  /* free labelled register states & stack: */

  STAT(parse_start = ia64_get_itc ());
  for (rs = sr->reg_state_list; rs; rs = next)
    {
      next = rs->next;
      free_reg_state (rs);
    }
  while (sr->stack)
    pop (sr);

  STAT(unw.stat.script.parse_time += ia64_get_itc () - parse_start);
  return 0;
}

int
ia64_get_proc_info (struct ia64_cursor *c)
{
#ifdef IA64_UNW_SCRIPT_CACHE
  struct ia64_script *script = ia64_script_lookup (c);

  if (script)
    {
      c->pi = script->pi;
      return 0;
    }
#endif

  return get_proc_info (c);
}
