/* libunwind - a platform-independent unwind library
   Copyright (c) 2003 Hewlett-Packard Development Company, L.P.
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

#include <string.h>

#include "dwarf_i.h"
#include "tdep.h"

#define alloc_reg_state()	(mempool_alloc (&dwarf_reg_state_pool))
#define free_reg_state(rs)	(mempool_free (&dwarf_reg_state_pool, rs))

static inline int
read_regnum (unw_addr_space_t as, unw_accessors_t *a, unw_word_t *addr,
	     unw_word_t *valp, void *arg)
{
  int ret;

  if ((ret = dwarf_read_uleb128 (as, a, addr, valp, arg)) < 0)
    return ret;

  if (*valp >= DWARF_NUM_PRESERVED_REGS)
    {
      Debug (1, "Invalid register number %u\n", *valp);
      return -UNW_EBADREG;
    }
  return 0;
}

static inline void
set_reg (dwarf_state_record_t *sr, unw_word_t regnum, dwarf_where_t where,
	 unw_word_t val)
{
  sr->rs_current.reg[regnum].where = where;
  sr->rs_current.reg[regnum].val = val;
}

/* Run a CFI program to update the register state.  */
static int
run_cfi_program (struct dwarf_cursor *c, dwarf_state_record_t *sr,
		 unw_word_t ip, unw_word_t *addr, unw_word_t end_addr,
		 unw_dyn_dwarf_fde_info_t *dfi)
{
  unw_word_t curr_ip, operand = 0, regnum, val, len, fde_encoding;
  dwarf_reg_state_t *rs_stack = NULL, *new_rs, *old_rs;
  unw_addr_space_t as;
  unw_accessors_t *a;
  uint8_t u8, op;
  uint16_t u16;
  uint32_t u32;
  void *arg;
  int ret;

  as = c->as;
  arg = c->as_arg;
  a = unw_get_accessors (as);
  curr_ip = c->pi.start_ip;

  while (curr_ip < ip && *addr < end_addr)
    {
      if ((ret = dwarf_readu8 (as, a, addr, &op, arg)) < 0)
	return ret;

      if (op & DWARF_CFA_OPCODE_MASK)
	{
	  operand = op & DWARF_CFA_OPERAND_MASK;
	  op &= ~DWARF_CFA_OPERAND_MASK;
	}
      switch ((dwarf_cfa_t) op)
	{
	case DW_CFA_advance_loc:
	  curr_ip += operand * dfi->code_align;
	  Debug (16, "CFA_advance_loc to 0x%lx\n", (long) curr_ip);
	  break;

	case DW_CFA_advance_loc1:
	  if ((ret = dwarf_readu8 (as, a, addr, &u8, arg)) < 0)
	    goto fail;
	  curr_ip += u8 * dfi->code_align;
	  Debug (16, "CFA_advance_loc1 to 0x%lx\n", (long) curr_ip);
	  break;

	case DW_CFA_advance_loc2:
	  if ((ret = dwarf_readu16 (as, a, addr, &u16, arg)) < 0)
	    goto fail;
	  curr_ip += u16 * dfi->code_align;
	  Debug (16, "CFA_advance_loc2 to 0x%lx\n", (long) curr_ip);
	  break;

	case DW_CFA_advance_loc4:
	  if ((ret = dwarf_readu32 (as, a, addr, &u32, arg)) < 0)
	    goto fail;
	  curr_ip += u32 * dfi->code_align;
	  Debug (16, "CFA_advance_loc4 to 0x%lx\n", (long) curr_ip);
	  break;

	case DW_CFA_MIPS_advance_loc8:
#ifdef UNW_TARGET_MIPS
	  {
	    uint64_t u64;

	    if ((ret = dwarf_readu64 (as, a, addr, &u64, arg)) < 0)
	      goto fail;
	    curr_ip += u64 * dfi->code_align;
	    Debug (16, "CFA_MIPS_advance_loc8\n");
	    break;
	  }
#else
	  Debug (1, "DW_CFA_MIPS_advance_loc8 on non-MIPS target\n");
	  ret = -UNW_EINVAL;
	  goto fail;
#endif

	case DW_CFA_offset:
	  regnum = operand;
	  if (regnum >= DWARF_NUM_PRESERVED_REGS)
	    {
	      Debug (1, "Invalid register number %u in DW_cfa_OFFSET\n",
		     regnum);
	      ret = -UNW_EBADREG;
	      goto fail;
	    }
	  if ((ret = dwarf_read_uleb128 (as, a, addr, &val, arg)) < 0)
	    goto fail;
	  set_reg (sr, regnum, DWARF_WHERE_CFAREL, val * dfi->data_align);
	  Debug (16, "CFA_offset r%lu at cfa+0x%lx\n",
		 (long) regnum, (long) (val * dfi->data_align));
	  break;

	case DW_CFA_offset_extended:
	  if (((ret = read_regnum (as, a, addr, &regnum, arg)) < 0)
	      || ((ret = dwarf_read_uleb128 (as, a, addr, &val, arg)) < 0))
	    goto fail;
	  set_reg (sr, regnum, DWARF_WHERE_CFAREL, val * dfi->data_align);
	  Debug (16, "CFA_offset_extended r%lu at cf+0x%lx\n",
		 (long) regnum, (long) (val * dfi->data_align));
	  break;

	case DW_CFA_offset_extended_sf:
	  if (((ret = read_regnum (as, a, addr, &regnum, arg)) < 0)
	      || ((ret = dwarf_read_sleb128 (as, a, addr, &val, arg)) < 0))
	    goto fail;
	  set_reg (sr, regnum, DWARF_WHERE_CFAREL, val * dfi->data_align);
	  Debug (16, "CFA_offset_extended_sf r%lu at cf+0x%lx\n",
		 (long) regnum, (long) (val * dfi->data_align));
	  break;

	case DW_CFA_restore:
	  regnum = operand;
	  if (regnum >= DWARF_NUM_PRESERVED_REGS)
	    {
	      Debug (1, "Invalid register number %u in DW_CFA_restore\n",
		     regnum);
	      ret = -UNW_EINVAL;
	      goto fail;
	    }
	  sr->rs_current.reg[regnum] = sr->rs_initial.reg[regnum];
	  Debug (16, "CFA_restore r%lu\n", (long) regnum);
	  break;

	case DW_CFA_restore_extended:
	  if ((ret = dwarf_read_uleb128 (as, a, addr, &regnum, arg)) < 0)
	    goto fail;
	  if (regnum >= DWARF_NUM_PRESERVED_REGS)
	    {
	      Debug (1, "Invalid register number %u in "
		     "DW_CFA_restore_extended\n", regnum);
	      ret = -UNW_EINVAL;
	      goto fail;
	    }
	  sr->rs_current.reg[regnum] = sr->rs_initial.reg[regnum];
	  Debug (16, "CFA_restore_extended r%lu\n", (long) regnum);
	  break;

	case DW_CFA_nop:
	  break;

	case DW_CFA_set_loc:
	  fde_encoding = dfi->flags & UNW_DYN_DFI_FLAG_FDE_PE_MASK;
	  if ((ret = dwarf_read_encoded_pointer (as, a, addr, fde_encoding,
						 &c->pi, &curr_ip,
						 arg)) < 0)
	    goto fail;
	  Debug (16, "CFA_set_loc to 0x%lx\n", (long) curr_ip);
	  break;

	case DW_CFA_undefined:
	  if ((ret = read_regnum (as, a, addr, &regnum, arg)) < 0)
	    goto fail;
	  set_reg (sr, regnum, DWARF_WHERE_UNDEF, 0);
	  Debug (16, "CFA_undefined r%lu\n", (long) regnum);
	  break;

	case DW_CFA_same_value:
	  if ((ret = read_regnum (as, a, addr, &regnum, arg)) < 0)
	    goto fail;
	  set_reg (sr, regnum, DWARF_WHERE_SAME, 0);
	  Debug (16, "CFA_same_value r%lu\n", (long) regnum);
	  break;

	case DW_CFA_register:
	  if (((ret = read_regnum (as, a, addr, &regnum, arg)) < 0)
	      || ((ret = dwarf_read_uleb128 (as, a, addr, &val, arg)) < 0))
	    goto fail;
	  set_reg (sr, regnum, DWARF_WHERE_REG, val);
	  Debug (16, "CFA_register r%lu to r%lu\n", (long) regnum, (long) val);
	  break;

	case DW_CFA_remember_state:
	  new_rs = alloc_reg_state ();
	  if (!new_rs)
	    {
	      Debug (1, "Out of memory in DW_CFA_remember_state\n");
	      ret = -UNW_ENOMEM;
	      goto fail;
	    }

	  memcpy (new_rs->reg, sr->rs_current.reg, sizeof (new_rs->reg));
	  new_rs->next = rs_stack;
	  rs_stack = new_rs;
	  Debug (16, "CFA_remember_state\n");
	  break;

	case DW_CFA_restore_state:
	  if (!rs_stack)
	    {
	      Debug (1, "register-state stack underflow\n");
	      ret = -UNW_EINVAL;
	      goto fail;
	    }
	  memcpy (&sr->rs_current.reg, &rs_stack->reg, sizeof (rs_stack->reg));
	  old_rs = rs_stack;
	  rs_stack = rs_stack->next;
	  free_reg_state (old_rs);
	  Debug (16, "CFA_restore_state\n");
	  break;

	case DW_CFA_def_cfa:
	  if (((ret = read_regnum (as, a, addr, &regnum, arg)) < 0)
	      || ((ret = dwarf_read_uleb128 (as, a, addr, &val, arg)) < 0))
	    goto fail;
	  set_reg (sr, DWARF_CFA_REG_COLUMN, DWARF_WHERE_REG, regnum);
	  set_reg (sr, DWARF_CFA_OFF_COLUMN, 0, val);	/* NOT factored! */
	  Debug (16, "CFA_def_cfa r%lu+0x%lx\n", (long) regnum, (long) val);
	  break;

	case DW_CFA_def_cfa_sf:
	  if (((ret = read_regnum (as, a, addr, &regnum, arg)) < 0)
	      || ((ret = dwarf_read_sleb128 (as, a, addr, &val, arg)) < 0))
	    goto fail;
	  set_reg (sr, DWARF_CFA_REG_COLUMN, DWARF_WHERE_REG, regnum);
	  set_reg (sr, DWARF_CFA_OFF_COLUMN, 0,
		   val * dfi->data_align);		/* factored! */
	  Debug (16, "CFA_def_cfa_sf r%lu+0x%lx\n",
		 (long) regnum, (long) (val * dfi->data_align));
	  break;

	case DW_CFA_def_cfa_register:
	  if ((ret = read_regnum (as, a, addr, &regnum, arg)) < 0)
	    goto fail;
	  set_reg (sr, DWARF_CFA_REG_COLUMN, DWARF_WHERE_REG, regnum);
	  Debug (16, "CFA_def_cfa_register r%lu\n", (long) regnum);
	  break;

	case DW_CFA_def_cfa_offset:
	  if ((ret = dwarf_read_uleb128 (as, a, addr, &val, arg)) < 0)
	    goto fail;
	  set_reg (sr, DWARF_CFA_OFF_COLUMN, 0, val);	/* NOT factored! */
	  Debug (16, "CFA_def_cfa_offsets 0x%lx\n", (long) val);
	  break;

	case DW_CFA_def_cfa_offset_sf:
	  if ((ret = dwarf_read_sleb128 (as, a, addr, &val, arg)) < 0)
	    goto fail;
	  set_reg (sr, DWARF_CFA_OFF_COLUMN, 0,
		   val * dfi->data_align);	/* factored! */
	  Debug (16, "CFA_def_cfa_offsets_sf 0x%lx\n",
		 (long) (val * dfi->data_align));
	  break;

	case DW_CFA_def_cfa_expression:
	  /* Save the address of the DW_FORM_block for later evaluation. */
	  set_reg (sr, DWARF_CFA_REG_COLUMN, DWARF_WHERE_EXPR, *addr);

	  if ((ret = dwarf_read_uleb128 (as, a, addr, &len, arg)) < 0)
	    goto fail;

	  Debug (16, "CFA_def_cfa_expr @ 0x%lx [%lu bytes]\n",
		 (long) *addr, (long) len);
	  *addr += len;
	  break;

	case DW_CFA_CFA_expression:
	  if ((ret = read_regnum (as, a, addr, &regnum, arg)) < 0)
	    goto fail;

	  /* Save the address of the DW_FORM_block for later evaluation. */
	  set_reg (sr, regnum, DWARF_WHERE_EXPR, *addr);

	  if ((ret = dwarf_read_uleb128 (as, a, addr, &len, arg)) < 0)
	    goto fail;

	  Debug (16, "CFA_expression r%lu @ 0x%lx [%lu bytes]\n",
		 (long) regnum, (long) addr, (long) len);
	  *addr += len;
	  break;

	case DW_CFA_GNU_args_size:
	  if ((ret = dwarf_read_uleb128 (as, a, addr, &val, arg)) < 0)
	    goto fail;
	  sr->args_size = val;
	  Debug (16, "CFA_GNU_args_size %lu\n", (long) val);
	  break;

	case DW_CFA_GNU_negative_offset_extended:
	  /* A comment in GCC says that this is obsoleted by
	     DW_CFA_offset_extended_sf, but that it's used by older
	     PowerPC code.  */
	  if (((ret = read_regnum (as, a, addr, &regnum, arg)) < 0)
	      || ((ret = dwarf_read_uleb128 (as, a, addr, &val, arg)) < 0))
	    goto fail;
	  set_reg (sr, regnum, DWARF_WHERE_CFAREL, -(val * dfi->data_align));
	  Debug (16, "CFA_GNU_negative_offsets_extended cfa+0x%lx\n",
		 (long) -(val * dfi->data_align));
	  break;

	case DW_CFA_GNU_window_save:
#ifdef UNW_TARGET_SPARC
	  /* This is a special CFA to handle all 16 windowed registers
	     on SPARC.  */
	  for (regnum = 16; regnum < 32; ++regnum)
	    set_reg (sr, regnum, DWARF_WHERE_CFAREL,
		     (regnum - 16) * sizeof (unw_word_t));
	  Debug (16, "CFA_GNU_window_save\n");
	  break;
#else
	  /* FALL THROUGH */
#endif
	case DW_CFA_lo_user:
	case DW_CFA_hi_user:
	  Debug (1, "Unexpected CFA opcode 0x%x", op);
	  ret = -UNW_EINVAL;
	  goto fail;
	}
    }
  ret = 0;

 fail:
  /* Free the register-state stack, if not empty already.  */
  while (rs_stack)
    {
      old_rs = rs_stack;
      rs_stack = rs_stack->next;
      free_reg_state (old_rs);
    }
  return ret;
}

static int
fetch_proc_info (struct dwarf_cursor *c, unw_word_t ip, int need_unwind_info)
{
  int ret, dynamic = 1;

  if (c->pi_valid && !need_unwind_info)
    return 0;

  /* check dynamic info first --- it overrides everything else */
  ret = unwi_find_dynamic_proc_info (c->as, ip, &c->pi, need_unwind_info,
				     c->as_arg);
  if (ret == -UNW_ENOINFO)
    {
      dynamic = 0;
      if ((ret = dwarf_find_proc_info (c->as, ip, &c->pi, need_unwind_info,
				       c->as_arg)) < 0)
	return ret;
    }

  c->pi_valid = 1;
  c->pi_is_dynamic = dynamic;
  return ret;
}

static int
parse_dynamic (struct dwarf_cursor *c, unw_word_t ip, dwarf_state_record_t *sr)
{
  Debug (1, "Not yet implemented\n");
#if 0
  /* Don't forget to set the ret_addr_column!  */
  c->ret_addr_column = XXX;
#endif
  return -UNW_ENOINFO;
}

static inline void
put_unwind_info (struct dwarf_cursor *c, unw_proc_info_t *pi)
{
  if (!c->pi_valid)
    return;

  if (c->pi_is_dynamic)
    unwi_put_dynamic_unwind_info (c->as, pi, c->as_arg);
}

static inline int
parse_fde (struct dwarf_cursor *c, unw_word_t ip, dwarf_state_record_t *sr)
{
  unw_dyn_dwarf_fde_info_t *dfi;
  unw_word_t addr;
  int ret;

  dfi = c->pi.unwind_info;
  c->ret_addr_column = dfi->ret_addr_column;

  addr = dfi->cie_instr_start;
  if ((ret = run_cfi_program (c, sr, ~(unw_word_t) 0, &addr,
			      dfi->cie_instr_end, dfi)) < 0)
    return ret;

  memcpy (&sr->rs_initial, &sr->rs_current, sizeof (sr->rs_initial));

  addr = dfi->fde_instr_start;
  if ((ret = run_cfi_program (c, sr, ip, &addr, dfi->fde_instr_end, dfi)) < 0)
    return ret;

  return 0;
}

static int
create_state_record_for (struct dwarf_cursor *c, dwarf_state_record_t *sr,
			 unw_word_t ip)
{
  int i, ret;

  assert (c->pi_valid);

  for (i = 0; i < DWARF_NUM_PRESERVED_REGS; ++i)
    set_reg (sr, i, DWARF_WHERE_SAME, 0);

  switch (c->pi.format)
    {
    case UNW_INFO_FORMAT_DWARF_FDE:
      ret = parse_fde (c, ip, sr);
      break;

    case UNW_INFO_FORMAT_DYNAMIC:
      ret = parse_dynamic (c, ip, sr);
      break;

    case UNW_INFO_FORMAT_REMOTE_TABLE:
    case UNW_INFO_FORMAT_TABLE:
    default:
      Debug (1, "Unexpected unwind-info format %d\n", c->pi.format);
      ret = -UNW_EINVAL;
    }
  return ret;
}

static inline int
eval_location_expr (struct dwarf_cursor *c, unw_addr_space_t as,
		    unw_accessors_t *a, unw_word_t addr,
		    dwarf_loc_t *locp, void *arg)
{
  int ret, is_register;
  unw_word_t len, val;

  /* read the length of the expression: */
  if ((ret = dwarf_read_uleb128 (as, a, &addr, &len, arg)) < 0)
    return ret;

  /* evaluate the expression: */
  if ((ret = dwarf_eval_expr (c, &addr, len, &val, &is_register)) < 0)
    return ret;

  if (is_register)
    *locp = DWARF_REG_LOC (c, dwarf_to_unw_regnum (val));
  else
    *locp = DWARF_MEM_LOC (c, val);

  return 0;
}

static int
apply_reg_state (struct dwarf_cursor *c, struct dwarf_reg_state *rs)
{
  unw_word_t regnum, addr, cfa;
  unw_addr_space_t as;
  dwarf_loc_t cfa_loc;
  unw_accessors_t *a;
  int i, ret;
  void *arg;

  as = c->as;
  arg = c->as_arg;
  a = unw_get_accessors (as);

  /* Evaluate the CFA first, because it may be referred to be other
     expressions.  */

  if (rs->reg[DWARF_CFA_REG_COLUMN].where == DWARF_WHERE_REG)
    {
      /* CFA is equal to [reg] + offset: */

      regnum = dwarf_to_unw_regnum (rs->reg[DWARF_CFA_REG_COLUMN].val);
      if ((ret = unw_get_reg ((unw_cursor_t *) c, regnum, &cfa)) < 0)
	return ret;

      cfa += rs->reg[DWARF_CFA_OFF_COLUMN].val;
    }
  else
    {
      /* CFA is equal to EXPR: */

      assert (rs->reg[DWARF_CFA_REG_COLUMN].where == DWARF_WHERE_EXPR);

      addr = rs->reg[DWARF_CFA_REG_COLUMN].val;
      if ((ret = eval_location_expr (c, as, a, addr, &cfa_loc, arg)) < 0)
	return ret;
      if ((ret = dwarf_get (c, cfa_loc, &cfa)) < 0)
	return ret;
    }
  c->cfa = cfa;

  for (i = 0; i < DWARF_NUM_PRESERVED_REGS; ++i)
    {
      switch ((dwarf_where_t) rs->reg[i].where)
	{
	case DWARF_WHERE_UNDEF:
	  c->loc[i] = DWARF_NULL_LOC;
	  break;

	case DWARF_WHERE_SAME:
	  break;

	case DWARF_WHERE_CFAREL:
	  c->loc[i] = DWARF_MEM_LOC (c, cfa + rs->reg[i].val);
	  break;

	case DWARF_WHERE_REG:
	  c->loc[i] = DWARF_REG_LOC (c, dwarf_to_unw_regnum (rs->reg[i].val));
	  break;

	case DWARF_WHERE_EXPR:
	  addr = rs->reg[i].val;
	  if ((ret = eval_location_expr (c, as, a, addr, c->loc + i, arg)) , 0)
	    return ret;
	  break;
	}
    }
  return 0;
}

HIDDEN int
dwarf_find_save_locs (struct dwarf_cursor *c)
{
  dwarf_state_record_t sr;
  int ret;

  if ((ret = fetch_proc_info (c, c->ip, 1)) < 0)
    return ret;

  if ((ret = create_state_record_for (c, &sr, c->ip)) < 0)
    return ret;

  if ((ret = apply_reg_state (c, &sr.rs_current)) < 0)
    return ret;

  put_unwind_info (c, &c->pi);
  return 0;
}

/* The proc-info must be valid for IP before this routine can be
   called.  */
HIDDEN int
dwarf_create_state_record (struct dwarf_cursor *c, dwarf_state_record_t *sr)
{
  return create_state_record_for (c, sr, c->ip);
}

HIDDEN int
dwarf_make_proc_info (struct dwarf_cursor *c)
{
#if 0
  if (c->as->caching_policy == UNW_CACHE_NONE
      || get_cached_proc_info (c) < 0)
#endif
    /* Lookup it up the slow way... */
    return fetch_proc_info (c, c->ip, 0);
  return 0;
}
