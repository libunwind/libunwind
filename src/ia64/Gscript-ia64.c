/* libunwind - a platform-independent unwind library
   Copyright (C) 2001-2004 Hewlett-Packard Co
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

#include "offsets.h"
#include "regs.h"
#include "rse.h"
#include "unwind_i.h"

#ifdef HAVE___THREAD
static __thread struct ia64_script_cache ia64_per_thread_cache =
  {
#ifdef HAVE_ATOMIC_OPS_H
    .busy = AO_TS_INITIALIZER
#else
    .lock = PTHREAD_MUTEX_INITIALIZER
#endif
  };
#endif

static inline unw_hash_index_t
hash (unw_word_t ip)
{
# define magic	0x9e3779b97f4a7c16	/* based on (sqrt(5)/2-1)*2^64 */

  return (ip >> 4) * magic >> (64 - IA64_LOG_UNW_HASH_SIZE);
}

static inline long
cache_match (struct ia64_script *script, unw_word_t ip, unw_word_t pr)
{
  if (ip == script->ip && ((pr ^ script->pr_val) & script->pr_mask) == 0)
    return 1;
  return 0;
}

static inline void
flush_script_cache (struct ia64_script_cache *cache)
{
  int i;

  cache->lru_head = IA64_UNW_CACHE_SIZE - 1;
  cache->lru_tail = 0;

  for (i = 0; i < IA64_UNW_CACHE_SIZE; ++i)
    {
      if (i > 0)
	cache->buckets[i].lru_chain = (i - 1);
      cache->buckets[i].coll_chain = -1;
      cache->buckets[i].ip = 0;
    }
  for (i = 0; i<IA64_UNW_HASH_SIZE; ++i)
    cache->hash[i] = -1;
}

static inline struct ia64_script_cache *
get_script_cache (unw_addr_space_t as, sigset_t *saved_sigmaskp)
{
  struct ia64_script_cache *cache = &as->global_cache;
  unw_caching_policy_t caching = as->caching_policy;

  if (caching == UNW_CACHE_NONE)
    return NULL;

#ifdef HAVE___THREAD
  if (as->caching_policy == UNW_CACHE_PER_THREAD)
    cache = &ia64_per_thread_cache;
#endif

#ifdef HAVE_ATOMIC_OPS_H
  if (AO_test_and_set (&cache->busy) == AO_TS_SET)
    return NULL;
#else
  sigprocmask (SIG_SETMASK, &unwi_full_sigmask, saved_sigmaskp);
  if (likely (caching == UNW_CACHE_GLOBAL))
    {
      Debug (16, "%s: acquiring lock\n");
      mutex_lock (&cache->lock);
    }
#endif

  if (as->cache_generation != cache->generation)
    {
      flush_script_cache (cache);
      cache->generation = as->cache_generation;
    }
  return cache;
}

static inline void
put_script_cache (unw_addr_space_t as, struct ia64_script_cache *cache,
		  sigset_t *saved_sigmaskp)
{
  assert (as->caching_policy != UNW_CACHE_NONE);

  Debug (16, "unmasking signals/releasing lock\n");
#ifdef HAVE_ATOMIC_OPS_H
  AO_CLEAR (&cache->busy);
#else
  if (likely (as->caching_policy == UNW_CACHE_GLOBAL))
    mutex_unlock (&cache->lock);
  sigprocmask (SIG_SETMASK, saved_sigmaskp, NULL);
#endif
}

static struct ia64_script *
script_lookup (struct ia64_script_cache *cache, struct cursor *c)
{
  struct ia64_script *script = cache->buckets + c->hint;
  unsigned short index;
  unw_word_t ip, pr;

  ip = c->ip;
  pr = c->pr;

  if (cache_match (script, ip, pr))
    return script;

  index = cache->hash[hash (ip)];
  if (index >= IA64_UNW_CACHE_SIZE)
    return 0;

  script = cache->buckets + index;
  while (1)
    {
      if (cache_match (script, ip, pr))
	{
	  /* update hint; no locking needed: single-word writes are atomic */
	  c->hint = cache->buckets[c->prev_script].hint =
	    (script - cache->buckets);
	  return script;
	}
      if (script->coll_chain >= IA64_UNW_HASH_SIZE)
	return 0;
      script = cache->buckets + script->coll_chain;
    }
}

HIDDEN int
ia64_get_cached_proc_info (struct cursor *c)
{
  struct ia64_script_cache *cache;
  struct ia64_script *script;
  sigset_t saved_sigmask;

  cache = get_script_cache (c->as, &saved_sigmask);
  if (!cache)
    return -UNW_ENOINFO;	/* cache is busy */
  {
    script = script_lookup (cache, c);
    if (script)
      c->pi = script->pi;
  }
  put_script_cache (c->as, cache, &saved_sigmask);
  return script ? 0 : -UNW_ENOINFO;
}

static inline void
script_init (struct ia64_script *script, unw_word_t ip)
{
  script->ip = ip;
  script->hint = 0;
  script->count = 0;
  script->abi_marker = 0;
}

static inline struct ia64_script *
script_new (struct ia64_script_cache *cache, unw_word_t ip)
{
  struct ia64_script *script, *prev, *tmp;
  unw_hash_index_t index;
  unsigned short head;

  head = cache->lru_head;
  script = cache->buckets + head;
  cache->lru_head = script->lru_chain;

  /* re-insert script at the tail of the LRU chain: */
  cache->buckets[cache->lru_tail].lru_chain = head;
  cache->lru_tail = head;

  /* remove the old script from the hash table (if it's there): */
  if (script->ip)
    {
      index = hash (script->ip);
      tmp = cache->buckets + cache->hash[index];
      prev = 0;
      while (1)
	{
	  if (tmp == script)
	    {
	      if (prev)
		prev->coll_chain = tmp->coll_chain;
	      else
		cache->hash[index] = tmp->coll_chain;
	      break;
	    }
	  else
	    prev = tmp;
	  if (tmp->coll_chain >= IA64_UNW_CACHE_SIZE)
	    /* old script wasn't in the hash-table */
	    break;
	  tmp = cache->buckets + tmp->coll_chain;
	}
    }

  /* enter new script in the hash table */
  index = hash (ip);
  script->coll_chain = cache->hash[index];
  cache->hash[index] = script - cache->buckets;

  script_init (script, ip);
  return script;
}

static inline void
script_finalize (struct ia64_script *script, struct cursor *c,
		 struct ia64_state_record *sr)
{
  script->pr_mask = sr->pr_mask;
  script->pr_val = sr->pr_val;
  script->pi = c->pi;
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
      opc = IA64_INSN_SET_REG;
      val = r->val;
      break;

    case IA64_WHERE_FR:
      break;				/* value doesn't matter... */

    case IA64_WHERE_BR:
      /* val==0 results in IA64_LOC_NULL, i.e., no NaT bit */
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
  insn.dst = i + (UNW_IA64_NAT - UNW_IA64_GR);
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
	  val = rval;
	}
      else if (rval >= 4 && rval <= 7)
	/* register got spilled to a preserved register */
	val = IA64_REG_R4 + (rval - 4);
      else
	{
	  /* register got spilled to a scratch register */
	  opc = IA64_INSN_MOVE_SCRATCH;
	  val = UNW_IA64_GR + rval;
	}
      break;

    case IA64_WHERE_FR:
      if (rval <= 5)
	val = IA64_REG_F2 + (rval - 1);
      else if (rval >= 16 && rval <= 31)
	val = IA64_REG_F16 + (rval - 16);
      else
	{
	  opc = IA64_INSN_MOVE_SCRATCH;
	  val = UNW_IA64_FR + rval;
	}
      break;

    case IA64_WHERE_BR:
      if (rval >= 1 && rval <= 5)
	val = IA64_REG_B1 + (rval - 1);
      else
	{
	  opc = IA64_INSN_MOVE_SCRATCH;
	  val = UNW_IA64_BR + rval;
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
  insn.dst = i;
  insn.val = val;
  script_emit (script, insn);
  if (is_preserved_gr)
    emit_nat_info (sr, i, script);

  if (i == IA64_REG_PSP)
    {
      /* c->psp must contain the _value_ of the previous sp, not it's
	 save-location.  We get this by dereferencing the value we
	 just stored in loc[IA64_REG_PSP]: */
      insn.opc = IA64_INSN_LOAD_PSP;
      script_emit (script, insn);
    }
}

/* Build an unwind script that unwinds from state OLD_STATE to the
   entrypoint of the function that called OLD_STATE.  */

static inline int
build_script (struct cursor *c, struct ia64_script *script)
{
  struct ia64_state_record sr;
  struct ia64_script_insn insn;
  int i, ret;

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
      insn.opc = IA64_INSN_INC_PSP;
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

  /* Note: it's important here that IA64_REG_PSP gets compiled first
     because later save-locations may depend on it's correct (updated)
     value.  Fixed-size frames are handled speciall (see above), but
     variable-size frames get handled as part of the normal
     compile_reg().  */
  for (i = IA64_REG_PSP; i < IA64_NUM_PREGS; ++i)
    compile_reg (&sr, i, script);

  script->abi_marker = sr.abi_marker;
  script_finalize (script, c, &sr);

  ia64_free_state_record (&sr);
  return 0;
}

/* Apply the unwinding actions represented by OPS and update SR to
   reflect the state that existed upon entry to the function that this
   unwinder represents.  */

static inline int
run_script (struct ia64_script *script, struct cursor *c)
{
  struct ia64_script_insn *ip, *limit, next_insn;
  unw_word_t val, unat_addr;
  unsigned long opc, dst;
  ia64_loc_t loc;
  int ret;

  c->pi = script->pi;
  ip = script->insn;
  limit = script->insn + script->count;
  next_insn = *ip;
  c->abi_marker = script->abi_marker;

  while (ip++ < limit)
    {
      opc = next_insn.opc;
      dst = next_insn.dst;
      val = next_insn.val;
      next_insn = *ip;

      /* This is by far the most common operation: */
      if (likely (opc == IA64_INSN_MOVE_STACKED))
	{
	  val = rotate_gr (c, val);
	  if ((ret = ia64_get_stacked (c, val, &loc, NULL)) < 0)
	    return ret;
	}
      else
	switch (opc)
	  {
	  case IA64_INSN_SET:
	    loc = IA64_LOC_ADDR (val, 0);
	    break;

	  case IA64_INSN_SET_REG:
	    loc = IA64_LOC_REG (val, 0);
	    break;

	  case IA64_INSN_ADD_PSP:
	    loc = IA64_LOC_ADDR (c->psp + val, 0);
	    break;

	  case IA64_INSN_ADD_SP:
	    loc = IA64_LOC_ADDR (c->sp + val, 0);
	    break;

	  case IA64_INSN_MOVE:
	    loc = c->loc[val];
	    break;

	  case IA64_INSN_MOVE_SCRATCH:
	    loc = ia64_scratch_loc (c, val);
	    break;

	  case IA64_INSN_SETNAT_MEMSTK:
	    if ((ret = ia64_get (c, c->loc[IA64_REG_PRI_UNAT_MEM],
				 &unat_addr)) < 0)
	      return ret;
	    loc = IA64_LOC_ADDR (unat_addr, IA64_LOC_TYPE_MEMSTK_NAT);
	    break;

	  case IA64_INSN_INC_PSP:
	    c->psp += val;
	    continue;

	  case IA64_INSN_LOAD_PSP:
	    if ((ret = ia64_get (c, c->loc[IA64_REG_PSP], &c->psp)) < 0)
	      return ret;
	    continue;
	  }
      c->loc[dst] = loc;
    }
  return 0;
}

static int
uncached_find_save_locs (struct cursor *c)
{
  struct ia64_script script;
  int ret = 0;

  if ((ret = ia64_fetch_proc_info (c, c->ip, 1)) < 0)
    return ret;

  script_init (&script, c->ip);
  if ((ret = build_script (c, &script)) < 0)
    {
      if (ret != -UNW_ESTOPUNWIND)
	dprintf ("%s: failed to build unwind script for ip %lx\n",
		 __FUNCTION__, (long) c->ip);
      return ret;
    }
  return run_script (&script, c);
}

HIDDEN int
ia64_find_save_locs (struct cursor *c)
{
  struct ia64_script_cache *cache = NULL;
  struct ia64_script *script = NULL;
  sigset_t saved_sigmask;
  int ret = 0;

  if (c->as->caching_policy == UNW_CACHE_NONE)
    return uncached_find_save_locs (c);

  cache = get_script_cache (c->as, &saved_sigmask);
  if (!cache)
    {
      Debug (1, "contention on script-cache; doing uncached lookup\n");
      return uncached_find_save_locs (c);
    }
  {
    script = script_lookup (cache, c);
    Debug (8, "ip %lx %s in script cache\n", (long) c->ip,
	   script ? "hit" : "missed");
    if (!script)
      {
	if ((ret = ia64_fetch_proc_info (c, c->ip, 1)) < 0)
	  goto out;

	script = script_new (cache, c->ip);
	if (!script)
	  {
	    dprintf ("%s: failed to create unwind script\n", __FUNCTION__);
	    ret = -UNW_EUNSPEC;
	    goto out;
	  }
	cache->buckets[c->prev_script].hint = script - cache->buckets;

	ret = build_script (c, script);
      }
    c->hint = script->hint;
    c->prev_script = script - cache->buckets;

    if (ret < 0)
      {
	if (ret != -UNW_ESTOPUNWIND)
	  dprintf ("%s: failed to locate/build unwind script for ip %lx\n",
		   __FUNCTION__, (long) c->ip);
	goto out;
      }

    ret = run_script (script, c);
  }
 out:
  put_script_cache (c->as, cache, &saved_sigmask);
  return ret;
}

HIDDEN void
ia64_validate_cache (unw_addr_space_t as, void *arg)
{
#ifndef UNW_REMOTE_ONLY
  if (ia64_local_validate_cache (as, arg) == 1)
    return;
#endif

#ifndef UNW_LOCAL_ONLY
  /* local info is up-to-date, check dynamic info.  */
  unwi_dyn_validate_cache (as, arg);
#endif
}
