/* libunwind - a platform-independent unwind library
   Copyright (C) 2010, 2011 by FERMI NATIONAL ACCELERATOR LABORATORY

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

#include "unwind_i.h"
#include "ucontext_i.h"
#include <signal.h>

#pragma weak pthread_once
#pragma weak pthread_key_create
#pragma weak pthread_getspecific
#pragma weak pthread_setspecific

/* Total maxium hash table size is 16M addresses. HASH_TOP_BITS must
   be a power of four, as the hash expands by four each time. */
#define HASH_LOW_BITS 14
#define HASH_TOP_BITS 10

/* There's not enough space to store RIP's location in a signal
   frame, but we can calculate it relative to RBP's (or RSP's)
   position in mcontext structure.  Note we don't want to use
   the UC_MCONTEXT_GREGS_* directly since we rely on DWARF info. */
#define dRIP (UC_MCONTEXT_GREGS_RIP - UC_MCONTEXT_GREGS_RBP)

typedef struct
{
  unw_tdep_frame_t *frames[1u << HASH_TOP_BITS];
  size_t log_frame_vecs;
  size_t used;
} unw_trace_cache_t;

static const unw_tdep_frame_t empty_frame = { 0, UNW_X86_64_FRAME_OTHER, -1, -1, 0, -1, -1 };
static pthread_mutex_t trace_init_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_once_t trace_cache_once = PTHREAD_ONCE_INIT;
static pthread_key_t trace_cache_key;
static struct mempool trace_cache_pool;
static struct mempool trace_frame_pool;

/* Initialise memory pools for frame tracing. */
static void
trace_pools_init (void)
{
  mempool_init (&trace_cache_pool, sizeof (unw_trace_cache_t), 0);
  mempool_init (&trace_frame_pool, (1u << HASH_LOW_BITS) * sizeof (unw_tdep_frame_t), 0);
}

/* Free memory for a thread's trace cache. */
static void
trace_cache_free (void *arg)
{
  unw_trace_cache_t *cache = arg;
  size_t i;

  for (i = 0; i < (1u << cache->log_frame_vecs); ++i)
    mempool_free (&trace_frame_pool, cache->frames[i]);

  mempool_free (&trace_cache_pool, cache);
  Debug(5, "freed cache %p\n", cache);
}

/* Initialise frame tracing for threaded use. */
static void
trace_cache_init_once (void)
{
  pthread_key_create (&trace_cache_key, &trace_cache_free);
  trace_pools_init ();
}

static unw_tdep_frame_t *
trace_cache_buckets (void)
{
  unw_tdep_frame_t *frames = mempool_alloc(&trace_frame_pool);
  size_t i;

  if (likely (frames != 0))
    for (i = 0; i < (1u << HASH_LOW_BITS); ++i)
      frames[i] = empty_frame;

  return frames;
}

/* Allocate and initialise hash table for frame cache lookups.
   Returns the cache initialised with (1u << HASH_LOW_BITS) hash
   buckets, or NULL if there was a memory allocation problem. */
static unw_trace_cache_t *
trace_cache_create (void)
{
  unw_trace_cache_t *cache;

  if (! (cache = mempool_alloc(&trace_cache_pool)))
  {
    Debug(5, "failed to allocate cache\n");
    return 0;
  }

  memset(cache, 0, sizeof (*cache));
  if (! (cache->frames[0] = trace_cache_buckets()))
  {
    Debug(5, "failed to allocate buckets\n");
    mempool_free(&trace_cache_pool, cache);
    return 0;
  }

  cache->log_frame_vecs = 0;
  cache->used = 0;
  Debug(5, "allocated cache %p\n", cache);
  return cache;
}

/* Expand the hash table in the frame cache if possible. This always
   quadruples the hash size, and clears all previous frame entries. */
static int
trace_cache_expand (unw_trace_cache_t *cache)
{
  size_t i, j, new_size, old_size;
  if (cache->log_frame_vecs == HASH_TOP_BITS)
  {
    Debug(5, "cache already at maximum size, cannot expand\n");
    return -UNW_ENOMEM;
  }

  old_size = (1u << cache->log_frame_vecs);
  new_size = cache->log_frame_vecs + 2;
  for (i = old_size; i < (1u << new_size); ++i)
    if (unlikely (! (cache->frames[i] = trace_cache_buckets())))
    {
      Debug(5, "failed to expand cache to 2^%lu hash bucket sets\n", new_size);
      for (j = old_size; j < i; ++j)
	mempool_free(&trace_frame_pool, cache->frames[j]);
      return -UNW_ENOMEM;
    }

  for (i = 0; i < old_size; ++i)
    for (j = 0; j < (1u << HASH_LOW_BITS); ++j)
      cache->frames[i][j] = empty_frame;

  Debug(5, "expanded cache from 2^%lu to 2^%lu hash bucket sets\n",
	cache->log_frame_vecs, new_size);
  cache->log_frame_vecs = new_size;
  cache->used = 0;
  return 0;
}

/* Get the frame cache for the current thread. Create it if there is none. */
static unw_trace_cache_t *
trace_cache_get (void)
{
  unw_trace_cache_t *cache;
  if (pthread_once)
  {
    pthread_once(&trace_cache_once, &trace_cache_init_once);
    if (! (cache = pthread_getspecific(trace_cache_key)))
    {
      cache = trace_cache_create();
      pthread_setspecific(trace_cache_key, cache);
    }
    Debug(5, "using cache %p\n", cache);
    return cache;
  }
  else
  {
    intrmask_t saved_mask;
    static unw_trace_cache_t *global_cache = 0;
    lock_acquire (&trace_init_lock, saved_mask);
    if (! global_cache)
    {
      trace_pools_init();
      global_cache = trace_cache_create();
    }
    cache = global_cache;
    lock_release (&trace_init_lock, saved_mask);
    Debug(5, "using cache %p\n", cache);
    return cache;
  }
}

/* Initialise frame properties for address cache slot F at address
   RIP using current CFA, RBP and RSP values.  Modifies CURSOR to
   that location, performs one unw_step(), and fills F with what
   was discovered about the location.  Returns F.

   FIXME: This probably should tell DWARF handling to never evaluate
   or use registers other than RBP, RSP and RIP in case there is
   highly unusual unwind info which uses these creatively. */
static unw_tdep_frame_t *
trace_init_addr (unw_tdep_frame_t *f,
		 unw_cursor_t *cursor,
		 unw_word_t cfa,
		 unw_word_t rip,
		 unw_word_t rbp,
		 unw_word_t rsp)
{
  struct cursor *c = (struct cursor *) cursor;
  struct dwarf_cursor *d = &c->dwarf;
  int ret = -UNW_EINVAL;

  /* Initialise frame properties: unknown, not last. */
  f->virtual_address = rip;
  f->frame_type = UNW_X86_64_FRAME_OTHER;
  f->last_frame = 0;
  f->cfa_reg_rsp = -1;
  f->cfa_reg_offset = 0;
  f->rbp_cfa_offset = -1;
  f->rsp_cfa_offset = -1;

  /* Reinitialise cursor to this instruction - but undo next/prev RIP
     adjustment because unw_step will redo it - and force RIP, RBP
     RSP into register locations (=~ ucontext we keep), then set
     their desired values. Then perform the step. */
  d->ip = rip + d->use_prev_instr;
  d->cfa = cfa;
  d->loc[UNW_X86_64_RIP] = DWARF_REG_LOC (d, UNW_X86_64_RIP);
  d->loc[UNW_X86_64_RBP] = DWARF_REG_LOC (d, UNW_X86_64_RBP);
  d->loc[UNW_X86_64_RSP] = DWARF_REG_LOC (d, UNW_X86_64_RSP);
  c->frame_info = *f;

  if (dwarf_put (d, d->loc[UNW_X86_64_RIP], rip) >= 0
      && dwarf_put (d, d->loc[UNW_X86_64_RBP], rbp) >= 0
      && dwarf_put (d, d->loc[UNW_X86_64_RSP], rsp) >= 0
      && (ret = unw_step (cursor)) >= 0)
    *f = c->frame_info;

  /* If unw_step() stopped voluntarily, remember that, even if it
     otherwise could not determine anything useful.  This avoids
     failing trace if we hit frames without unwind info, which is
     common for the outermost frame (CRT stuff) on many systems.
     This avoids failing trace in very common circumstances; failing
     to unw_step() loop wouldn't produce any better result. */
  if (ret == 0)
    f->last_frame = -1;

  Debug (3, "frame va %lx type %d last %d cfa %s+%d rbp @ cfa%+d rsp @ cfa%+d\n",
	 f->virtual_address, f->frame_type, f->last_frame,
	 f->cfa_reg_rsp ? "rsp" : "rbp", f->cfa_reg_offset,
	 f->rbp_cfa_offset, f->rsp_cfa_offset);

  return f;
}

/* Look up and if necessary fill in frame attributes for address RIP
   in CACHE using current CFA, RBP and RSP values.  Uses CURSOR to
   perform any unwind steps necessary to fill the cache.  Returns the
   frame cache slot which describes RIP. */
static unw_tdep_frame_t *
trace_lookup (unw_cursor_t *cursor,
	      unw_trace_cache_t *cache,
	      unw_word_t cfa,
	      unw_word_t rip,
	      unw_word_t rbp,
	      unw_word_t rsp)
{
  /* First look up for previously cached information using cache as
     linear probing hash table with probe step of 1.  Majority of
     lookups should be completed within few steps, but it is very
     important the hash table does not fill up, or performance falls
     off the cliff. */
  uint64_t i, hi, lo, addr;
  uint64_t cache_size = 1u << (HASH_LOW_BITS + cache->log_frame_vecs);
  uint64_t slot = ((rip * 0x9e3779b97f4a7c16) >> 43) & (cache_size-1);
  unw_tdep_frame_t *frame;

  for (i = 0; i < 16; ++i)
  {
    lo = slot & ((1u << HASH_LOW_BITS) - 1);
    hi = slot >> HASH_LOW_BITS;
    frame = &cache->frames[hi][lo];
    addr = frame->virtual_address;

    /* Return if we found the address. */
    if (addr == rip)
    {
      Debug (4, "found address after %ld steps\n", i);
      return frame;
    }

    /* If slot is empty, reuse it. */
    if (! addr)
      break;

    /* Linear probe to next slot candidate, step = 1. */
    if (++slot >= cache_size)
      slot -= cache_size;
  }

  /* If we collided after 16 steps, or if the hash is more than half
     full, force the hash to expand. Fill the selected slot, whether
     it's free or collides. Note that hash expansion drops previous
     contents; further lookups will refill the hash. */
  Debug (4, "updating slot %lu after %ld steps, replacing 0x%lx\n", slot, i, addr);
  if (unlikely (addr || cache->used >= cache_size / 2))
  {
    if (unlikely (trace_cache_expand (cache) < 0))
      return 0;

    cache_size = 1u << (HASH_LOW_BITS + cache->log_frame_vecs);
    slot = ((rip * 0x9e3779b97f4a7c16) >> 43) & (cache_size-1);
    lo = slot & ((1u << HASH_LOW_BITS) - 1);
    hi = slot >> HASH_LOW_BITS;
    frame = &cache->frames[hi][lo];
    addr = frame->virtual_address;
  }

  if (! addr)
    ++cache->used;

  return trace_init_addr (frame, cursor, cfa, rip, rbp, rsp);
}

/* Fast stack backtrace for x86-64.

   This is used by backtrace() implementation to accelerate frequent
   queries for current stack, without any desire to unwind. It fills
   BUFFER with the call tree from CURSOR upwards for at most SIZE
   stack levels. The first frame, backtrace itself, is omitted. When
   called, SIZE should give the maximum number of entries that can be
   stored into BUFFER. Uses an internal thread-specific cache to
   accelerate queries.

   The caller should fall back to a unw_step() loop if this function
   fails by returning -UNW_ESTOPUNWIND, meaning the routine hit a
   stack frame that is too complex to be traced in the fast path.

   This function is tuned for clients which only need to walk the
   stack to get the call tree as fast as possible but without any
   other details, for example profilers sampling the stack thousands
   to millions of times per second.  The routine handles the most
   common x86-64 ABI stack layouts: CFA is RBP or RSP plus/minus
   constant offset, return address is at CFA-8, and RBP and RSP are
   either unchanged or saved on stack at constant offset from the CFA;
   the signal return frame; and frames without unwind info provided
   they are at the outermost (final) frame or can conservatively be
   assumed to be frame-pointer based.

   Any other stack layout will cause the routine to give up. There
   are only a handful of relatively rarely used functions which do
   not have a stack in the standard form: vfork, longjmp, setcontext
   and _dl_runtime_profile on common linux systems for example.

   On success BUFFER and *SIZE reflect the trace progress up to *SIZE
   stack levels or the outermost frame, which ever is less.  It may
   stop short of outermost frame if unw_step() loop would also do so,
   e.g. if there is no more unwind information; this is not reported
   as an error.

   The function returns a negative value for errors, -UNW_ESTOPUNWIND
   if tracing stopped because of an unusual frame unwind info.  The
   BUFFER and *SIZE reflect tracing progress up to the error frame.

   Callers of this function would normally look like this:

     unw_cursor_t     cur;
     unw_context_t    ctx;
     void             addrs[128];
     int              depth = 128;
     int              ret;

     unw_getcontext(&ctx);
     unw_init_local(&cur, &ctx);
     if ((ret = unw_tdep_trace(&cur, addrs, &depth)) < 0)
     {
       depth = 0;
       unw_getcontext(&ctx);
       unw_init_local(&cur, &ctx);
       while ((ret = unw_step(&cur)) > 0 && depth < 128)
       {
         unw_word_t ip;
         unw_get_reg(&cur, UNW_REG_IP, &ip);
         addresses[depth++] = (void *) ip;
       }
     }
*/
HIDDEN int
tdep_trace (unw_cursor_t *cursor, void **buffer, int *size)
{
  struct cursor *c = (struct cursor *) cursor;
  struct dwarf_cursor *d = &c->dwarf;
  unw_trace_cache_t *cache;
  unw_word_t rbp, rsp, rip, cfa;
  int maxdepth = 0;
  int depth = 0;
  int ret;

  /* Check input parametres. */
  if (! cursor || ! buffer || ! size || (maxdepth = *size) <= 0)
    return -UNW_EINVAL;

  Debug (1, "begin ip 0x%lx cfa 0x%lx\n", d->ip, d->cfa);

  /* Tell core dwarf routines to call back to us. */
  d->stash_frames = 1;

  /* Determine initial register values. */
  rip = d->ip;
  rsp = cfa = d->cfa;
  if ((ret = dwarf_get (d, d->loc[UNW_X86_64_RBP], &rbp)) < 0)
  {
    Debug (1, "returning %d, rbp value not found\n", ret);
    *size = 0;
    d->stash_frames = 0;
    return ret;
  }

  /* Get frame cache. */
  if (! (cache = trace_cache_get()))
  {
    Debug (1, "returning %d, cannot get trace cache\n", -UNW_ENOMEM);
    *size = 0;
    d->stash_frames = 0;
    return -UNW_ENOMEM;
  }

  /* Trace the stack upwards, starting from current RIP.  Adjust
     the RIP address for previous/next instruction as the main
     unwinding logic would also do.  We undo this before calling
     back into unw_step(). */
  while (depth < maxdepth)
  {
    rip -= d->use_prev_instr;
    Debug (2, "depth %d cfa 0x%lx rip 0x%lx rsp 0x%lx rbp 0x%lx\n",
	   depth, cfa, rip, rsp, rbp);

    /* See if we have this address cached.  If not, evaluate enough of
       the dwarf unwind information to fill the cache line data, or to
       decide this frame cannot be handled in fast trace mode.  We
       cache negative results too to prevent unnecessary dwarf parsing
       for common failures. */
    unw_tdep_frame_t *f = trace_lookup (cursor, cache, cfa, rip, rbp, rsp);

    /* If we don't have information for this frame, give up. */
    if (! f)
    {
      ret = -UNW_ENOINFO;
      break;
    }

    Debug (3, "frame va %lx type %d last %d cfa %s+%d rbp @ cfa%+d rsp @ cfa%+d\n",
           f->virtual_address, f->frame_type, f->last_frame,
           f->cfa_reg_rsp ? "rsp" : "rbp", f->cfa_reg_offset,
           f->rbp_cfa_offset, f->rsp_cfa_offset);

    assert (f->virtual_address == rip);

    /* Stop if this was the last frame.  In particular don't evaluate
       new register values as it may not be safe - we don't normally
       run with full validation on, and do not want to - and there's
       enough bad unwind info floating around that we need to trust
       what unw_step() previously said, in potentially bogus frames. */
    if (f->last_frame)
      break;

    /* Evaluate CFA and registers for the next frame. */
    switch (f->frame_type)
    {
    case UNW_X86_64_FRAME_GUESSED:
      /* Fall thru to standard processing after forcing validation. */
      c->validate = 1;

    case UNW_X86_64_FRAME_STANDARD:
      /* Advance standard traceable frame. */
      cfa = (f->cfa_reg_rsp ? rsp : rbp) + f->cfa_reg_offset;
      ret = dwarf_get (d, DWARF_MEM_LOC (d, cfa - 8), &rip);
      if (ret >= 0 && f->rbp_cfa_offset != -1)
	ret = dwarf_get (d, DWARF_MEM_LOC (d, cfa + f->rbp_cfa_offset), &rbp);

      /* Don't bother reading RSP from DWARF, CFA becomes new RSP. */
      rsp = cfa;

      /* Next frame needs to back up for unwind info lookup. */
      d->use_prev_instr = 1;
      break;

    case UNW_X86_64_FRAME_SIGRETURN:
      /* Advance standard signal frame, whose CFA points above saved
         registers (ucontext) among other things.  We know the info
	 is stored at some unknown constant offset off inner frame's
	 CFA.  We determine the actual offset from DWARF unwind info. */
      d->use_prev_instr = 0;
      cfa = cfa + f->cfa_reg_offset;
      ret = dwarf_get (d, DWARF_MEM_LOC (d, cfa + f->rbp_cfa_offset + dRIP), &rip);
      if (ret >= 0)
	ret = dwarf_get (d, DWARF_MEM_LOC (d, cfa + f->rbp_cfa_offset), &rbp);
      if (ret >= 0)
	ret = dwarf_get (d, DWARF_MEM_LOC (d, cfa + f->rsp_cfa_offset), &rsp);

      /* Resume stack at signal restoration point. The stack is not
         necessarily continuous here, especially with sigaltstack(). */
      cfa = rsp;

      /* Next frame should not back up. */
      d->use_prev_instr = 0;
      break;

    default:
      /* We cannot trace through this frame, give up and tell the
	 caller we had to stop.  Data collected so far may still be
	 useful to the caller, so let it know how far we got.  */
      ret = -UNW_ESTOPUNWIND;
      break;
    }

    Debug (4, "new cfa 0x%lx rip 0x%lx rsp 0x%lx rbp 0x%lx\n",
	   cfa, rip, rsp, rbp);

    /* If we failed on ended up somewhere bogus, stop. */
    if (ret < 0 || rip < 0x4000)
      break;

    /* Record this address in stack trace. We skipped the first address. */
    buffer[depth++] = (void *) (rip - d->use_prev_instr);
  }

#if UNW_DEBUG
  Debug (1, "returning %d, depth %d\n", ret, depth);
#endif
  *size = depth;
  return ret;
}
