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

#include "rse.h"
#include "unwind_i.h"

static inline unw_hash_index_t
hash (unw_word_t ip)
{
# define magic	0x9e3779b97f4a7c16	/* based on (sqrt(5)/2-1)*2^64 */

  return (ip >> 4) * magic >> (64 - IA64_LOG_UNW_HASH_SIZE);
}

static inline long
cache_match (struct ia64_script *script, unw_word_t ip, unw_word_t pr)
{
  /* XXX lock script cache */
  if (ip == script->ip && ((pr ^ script->pr_val) & script->pr_mask) == 0)
    /* keep the lock... */
    return 1;
  /* XXX unlock script cache */
  return 0;
}

inline struct ia64_script *
ia64_script_lookup (struct ia64_cursor *c)
{
  struct ia64_script *script = unw.cache + c->hint;
  unsigned short index;
  unw_word_t ip, pr;

  STAT(++unw.stat.cache.lookups);

  ip = c->ip;
  pr = c->pr;

  if (cache_match (script, ip, pr))
    {
      STAT(++unw.stat.cache.hinted_hits);
      return script;
    }

  index = unw.hash[hash (ip)];
  if (index >= IA64_UNW_CACHE_SIZE)
    return 0;

  script = unw.cache + index;
  while (1)
    {
      if (cache_match (script, ip, pr))
	{
	  /* update hint; no locking needed: single-word writes are atomic */
	  STAT(++unw.stat.cache.normal_hits);
	  c->hint = unw.cache[c->prev_script].hint = script - unw.cache;
	  return script;
	}
      if (script->coll_chain >= IA64_UNW_HASH_SIZE)
	return 0;
      script = unw.cache + script->coll_chain;
      STAT(++unw.stat.cache.collision_chain_traversals);
    }
}

/* On returning, the lock for the SCRIPT is still being held.  */

static inline struct ia64_script *
script_new (unw_word_t ip)
{
  struct ia64_script *script, *prev, *tmp;
  unw_hash_index_t index;
  unsigned short head;

  STAT(++unw.stat.script.news);

  /* XXX lock hash table */
  {
    head = unw.lru_head;
    script = unw.cache + head;
    unw.lru_head = script->lru_chain;
  }
  /* XXX unlock hash table */

  /* XXX We'll deadlock here if we interrupt a thread that is holding
     the script->lock.  */
  /* XXX lock script */

  /* XXX lock unwind data lock */
  {
    /* re-insert script at the tail of the LRU chain: */
    unw.cache[unw.lru_tail].lru_chain = head;
    unw.lru_tail = head;

    /* remove the old script from the hash table (if it's there): */
    if (script->ip)
      {
	index = hash (script->ip);
	tmp = unw.cache + unw.hash[index];
	prev = 0;
	while (1)
	  {
	    if (tmp == script)
	      {
		if (prev)
		  prev->coll_chain = tmp->coll_chain;
		else
		  unw.hash[index] = tmp->coll_chain;
		break;
	      }
	    else
	      prev = tmp;
	    if (tmp->coll_chain >= IA64_UNW_CACHE_SIZE)
	      /* old script wasn't in the hash-table */
	      break;
	    tmp = unw.cache + tmp->coll_chain;
	  }
      }

    /* enter new script in the hash table */
    index = hash (ip);
    script->coll_chain = unw.hash[index];
    unw.hash[index] = script - unw.cache;

    script->ip = ip;		/* set new IP while we're holding the locks */

    STAT(if (script->coll_chain < IA64_UNW_CACHE_SIZE)
	   ++unw.stat.script.collisions);
  }
  /* XXX unlock unwind data lock */

  script->hint = 0;
  script->count = 0;
  return script;
}

static void
script_finalize (struct ia64_script *script, struct ia64_state_record *sr)
{
  script->pr_mask = sr->pr_mask;
  script->pr_val = sr->pr_val;
}

static inline void
script_emit (struct ia64_script *script, struct ia64_script_insn insn)
{
  if (script->count >= IA64_MAX_SCRIPT_LEN)
    {
      dprintf ("%s: script exceeds maximum size of %u instructions!\n",
	       __FUNCTION__, IA64_MAX_SCRIPT_LEN);
      return;
    }
  script->insn[script->count++] = insn;
}

static inline void
emit_nat_info (struct ia64_state_record *sr, int i, struct ia64_script *script)
{
  struct ia64_reg_info *r = sr->curr.reg + i;
  struct ia64_script_insn insn;
  enum ia64_script_insn_opcode opc = IA64_INSN_SET;
  unsigned long val = 0;

  switch (r->where)
    {
    case IA64_WHERE_GR:
      val = IA64_LOC (r->val, 0);
      break;

    case IA64_WHERE_FR:
      val = 0;			/* value doesn't matter... */
      break;

    case IA64_WHERE_BR:
      val = IA64_LOC (0, 0);	/* no NaT bit */
      break;

    case IA64_WHERE_PSPREL:
    case IA64_WHERE_SPREL:
      opc = IA64_INSN_SETNAT_MEMSTK;
      break;

    default:
      dprintf ("%s: don't know how to emit nat info for where = %u\n",
	       __FUNCTION__, r->where);
      return;
    }
  insn.opc = opc;
  insn.dst = unw.preg_index[i];
  insn.val = val;
  script_emit (script, insn);
}

static void
compile_reg (struct ia64_state_record *sr, int i, struct ia64_script *script)
{
  struct ia64_reg_info *r = sr->curr.reg + i;
  enum ia64_script_insn_opcode opc;
  unsigned long val, rval;
  struct ia64_script_insn insn;
  long is_preserved_gr;

  if (r->where == IA64_WHERE_NONE || r->when >= sr->when_target)
    return;

  opc = IA64_INSN_MOVE;
  val = rval = r->val;
  is_preserved_gr = (i >= IA64_REG_R4 && i <= IA64_REG_R7);

  switch (r->where)
    {
    case IA64_WHERE_GR:
      if (rval >= 32)
	{
	  /* register got spilled to a stacked register */
	  opc = IA64_INSN_MOVE_STACKED;
	  val = rval - 32;
	}
      else if (rval >= 4 && rval <= 7)
	/* register got spilled to a preserved register */
	val = unw.preg_index[IA64_REG_R4 + (rval - 4)];
      else
	{
	  /* register got spilled to a scratch register */
	  opc = IA64_INSN_MOVE_SIGCONTEXT;
	  val = struct_offset (struct sigcontext, sc_gr[rval]);
	}
      break;

    case IA64_WHERE_FR:
      if (rval <= 5)
	val = unw.preg_index[IA64_REG_F2 + (rval - 1)];
      else if (rval >= 16 && rval <= 31)
	val = unw.preg_index[IA64_REG_F16 + (rval - 16)];
      else
	{
	  opc = IA64_INSN_MOVE_SIGCONTEXT;
	  val = struct_offset (struct sigcontext, sc_fr[rval]);
	}
      break;

    case IA64_WHERE_BR:
      if (rval >= 1 && rval <= 5)
	val = unw.preg_index[IA64_REG_B1 + (rval - 1)];
      else
	{
	  opc = IA64_INSN_MOVE_SIGCONTEXT;
	  val = struct_offset (struct sigcontext, sc_br[rval]);
	}
      break;

    case IA64_WHERE_SPREL:
      opc = IA64_INSN_ADD_SP;
      if (i >= IA64_REG_F2 && i <= IA64_REG_F31)
	val |= IA64_LOC_TYPE_FP;
      break;

    case IA64_WHERE_PSPREL:
      opc = IA64_INSN_ADD_PSP;
      if (i >= IA64_REG_F2 && i <= IA64_REG_F31)
	val |= IA64_LOC_TYPE_FP;
      break;

    default:
      dprintf ("%s: register %u has unexpected `where' value of %u\n",
	       __FUNCTION__, i, r->where);
      break;
    }
  insn.opc = opc;
  insn.dst = unw.preg_index[i];
  insn.val = val;
  script_emit (script, insn);
  if (is_preserved_gr)
    emit_nat_info (sr, i, script);

  if (i == IA64_REG_PSP)
    {
      /* info->psp must contain the _value_ of the previous sp, not
	 it's save location.  We get this by dereferencing the value
	 we just stored in info->psp: */
      insn.opc = IA64_INSN_LOAD;
      insn.dst = insn.val = unw.preg_index[IA64_REG_PSP];
      script_emit (script, insn);
    }
}

/* Build an unwind script that unwinds from state OLD_STATE to the
   entrypoint of the function that called OLD_STATE.  */

static inline int
build_script (struct ia64_cursor *c, struct ia64_script **scriptp)
{
  struct ia64_script *script;
  struct ia64_state_record sr;
  struct ia64_script_insn insn;
  int i, ret;
  STAT(unsigned long start, parse_start;)
  STAT(++unw.stat.script.builds; start = ia64_get_itc ());
  STAT(unw.stat.script.parse_time += ia64_get_itc () - parse_start);

  script = script_new (c->ip);
  if (!script)
    {
      dprintf ("%s: failed to create unwind script\n", __FUNCTION__);
      STAT(unw.stat.script.build_time += ia64_get_itc() - start);
      return -UNW_EUNSPEC;
    }
  unw.cache[c->prev_script].hint = script - unw.cache;

  ret = ia64_create_state_record (c, &sr);
  if (ret < 0)
    return ret;

  /* First, set psp if we're dealing with a fixed-size frame;
     subsequent instructions may depend on this value.  */
  if (sr.when_target > sr.curr.reg[IA64_REG_PSP].when
      && (sr.curr.reg[IA64_REG_PSP].where == IA64_WHERE_NONE)
      && sr.curr.reg[IA64_REG_PSP].val != 0)
    {
      /* new psp is psp plus frame size */
      insn.opc = IA64_INSN_ADD;
      insn.dst = struct_offset (struct ia64_cursor, psp) / 8;
      insn.val = sr.curr.reg[IA64_REG_PSP].val;	/* frame size */
      script_emit (script, insn);
    }

  /* determine where the primary UNaT is: */
  if (sr.when_target < sr.curr.reg[IA64_REG_PRI_UNAT_GR].when)
    i = IA64_REG_PRI_UNAT_MEM;
  else if (sr.when_target < sr.curr.reg[IA64_REG_PRI_UNAT_MEM].when)
    i = IA64_REG_PRI_UNAT_GR;
  else if (sr.curr.reg[IA64_REG_PRI_UNAT_MEM].when >
	   sr.curr.reg[IA64_REG_PRI_UNAT_GR].when)
    i = IA64_REG_PRI_UNAT_MEM;
  else
    i = IA64_REG_PRI_UNAT_GR;

  compile_reg (&sr, i, script);

  for (i = IA64_REG_BSP; i < IA64_NUM_PREGS; ++i)
    compile_reg (&sr, i, script);

  script_finalize (script, &sr);
  script->pi = c->pi;

  ia64_free_state_record (&sr);

  STAT(unw.stat.script.build_time += ia64_get_itc () - start);
  *scriptp = script;
  return 0;
}

/* Apply the unwinding actions represented by OPS and update SR to
   reflect the state that existed upon entry to the function that this
   unwinder represents.  */

static inline int
run_script (struct ia64_script *script, struct ia64_cursor *c)
{
  struct ia64_script_insn *ip, *limit, next_insn;
  unw_word_t val, unat_addr, *s = (unw_word_t *) c;
  unsigned long opc, dst;
  int ret;
  STAT(unsigned long start;)

  STAT(++unw.stat.script.runs; start = ia64_get_itc ());
  c->pi = script->pi;
  ip = script->insn;
  limit = script->insn + script->count;
  next_insn = *ip;

  while (ip++ < limit)
    {
      opc = next_insn.opc;
      dst = next_insn.dst;
      val = next_insn.val;
      next_insn = *ip;

      switch (opc)
	{
	case IA64_INSN_SET:
	  s[dst] = val;
	  break;

	case IA64_INSN_ADD:
	  s[dst] += val;
	  break;

	case IA64_INSN_ADD_PSP:
	  s[dst] = c->psp + val;
	  break;

	case IA64_INSN_ADD_SP:
	  s[dst] = c->sp + val;
	  break;

	case IA64_INSN_MOVE:
	  s[dst] = s[val];
	  break;

	case IA64_INSN_MOVE_SIGCONTEXT:
	  s[dst] = ia64_get_sigcontext_addr (c);
	  break;

	case IA64_INSN_MOVE_STACKED:
	  s[dst] = (unsigned long) ia64_rse_skip_regs ((unsigned long *)
						       c->bsp, val);
	  break;

	case IA64_INSN_SETNAT_MEMSTK:
	  ret = ia64_get (c, c->pri_unat_loc, &unat_addr);
	  if (ret < 0)
	    return ret;
	  s[dst] = IA64_LOC (unat_addr >> 3, IA64_LOC_TYPE_MEMSTK_NAT);
	  break;

	case IA64_INSN_LOAD:
	  ret = ia64_get (c, s[val], &s[dst]);
	  if (ret < 0)
	    return ret;
	  break;
	}
    }
  STAT(unw.stat.script.run_time += ia64_get_itc () - start);
  return 0;
}

int
ia64_find_save_locs (struct ia64_cursor *c)
{
  struct ia64_script *script = ia64_script_lookup (c);
  int ret;

  if (!script)
    {
      ret = build_script (c, &script);
      if (ret < 0)
	{
	  if (ret != UNW_ESTOPUNWIND)
	    dprintf ("%s: failed to locate/build unwind script for ip %lx\n",
		     __FUNCTION__, c->ip);
	  return ret;
	}
    }
  c->hint = script->hint;
  c->prev_script = script - unw.cache;

  run_script (script, c);
  return 0;
}
