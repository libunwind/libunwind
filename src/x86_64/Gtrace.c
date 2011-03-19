/* libunwind - a platform-independent unwind library
   Copyright (C) 2010 by Lassi Tuura <lat@iki.fi>

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

/* Utility for timing in debug mode.  You'll probably want to
   comment out all unnecessary debugging in this file if you
   use this, otherwise the timings printed will not make sense. */
#if UNW_DEBUG
#define rdtsc(v)				 		\
  do { unsigned lo, hi;						\
    __asm__ volatile ("rdtsc" : "=a" (lo), "=d" (hi));		\
    (v) = ((unsigned long) lo) | ((unsigned long) hi << 32);	\
  } while (0)
#endif

/* There's not enough space to store RIP's location in a signal
   frame, but we can calculate it relative to RBP's (or RSP's)
   position in mcontext structure.  Note we don't want to use
   the UC_MCONTEXT_GREGS_* directly since we rely on DWARF info. */
#define dRIP (UC_MCONTEXT_GREGS_RIP - UC_MCONTEXT_GREGS_RBP)

/* Allocate and initialise hash table for frame cache lookups.
   Client requests size N, which should be 5 to 10 more than expected
   number of unique addresses to trace.  Minimum size of 10000 is
   forced.  Returns the cache, or NULL if there was a memory
   allocation problem. */
unw_tdep_frame_t *
unw_tdep_make_frame_cache (size_t n)
{
  size_t i;
  unw_tdep_frame_t *cache;

  if (n < 10000)
    n = 10000;

  if (! (cache = malloc((n+1) * sizeof(unw_tdep_frame_t))))
    return 0;

  unw_tdep_frame_t empty = { 0, UNW_X86_64_FRAME_OTHER, -1, -1, 0, -1, -1 };
  for (i = 0; i < n; ++i)
    cache[i] = empty;

  cache[0].virtual_address = n;
  return cache+1;
}

/* Free the address cache allocated by unw_tdep_make_frame_cache().
   Returns 0 on success, or -UNW_EINVAL if cache was NULL. */
int
unw_tdep_free_frame_cache (unw_tdep_frame_t *cache)
{
  if (! cache)
    return -UNW_EINVAL;

  free(cache-1);
  return 0;
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
	      unw_tdep_frame_t *cache,
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
  uint64_t cache_size = cache[-1].virtual_address;
  uint64_t probe_steps = (cache_size >> 5);
  uint64_t slot = ((rip * 0x9e3779b97f4a7c16) >> 43) % cache_size;
  uint64_t i;

  for (i = 0; i < probe_steps; ++i)
  {
    uint64_t addr = cache[slot].virtual_address;

    /* Return if we found the address. */
    if (addr == rip)
    {
      Debug (4, "found address after %ld steps\n", i);
      return &cache[slot];
    }

    /* If slot is empty, reuse it. */
    if (! addr)
      break;

    /* Linear probe to next slot candidate, step = 1. */
    if (++slot > cache_size)
      slot -= cache_size;
  }

  /* Fill this slot, whether it's free or hash collision. */
  Debug (4, "updating slot after %ld steps\n", i);
  return trace_init_addr (&cache[slot], cursor, cfa, rip, rbp, rsp);
}

/* Fast stack backtrace for x86-64.

   Intended for use when the application makes frequent queries to the
   current call stack without any desire to unwind.  Somewhat like the
   GLIBC backtrace() function: fills BUFFER with the call tree from
   CURSOR upwards, and SIZE with the number of stack levels so found.
   When called, SIZE should tell the maximum number of entries that
   can be stored in BUFFER.  CACHE is used to accelerate the stack
   queries; no other thread may use the same cache concurrently.

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

   Any other stack layout will cause the routine to give up.  There
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
     unw_context_t    ctx, saved;
     unw_tdep_frame_t *cache = ...;
     void             addrs[128];
     int              depth = 128;
     int              ret;

     unw_getcontext(&ctx);
     memcpy(&saved, &ctx, sizeof(ctx));

     unw_init_local(&cur, &ctx);
     if (! cache || (ret = unw_tdep_trace(&cur, addrs, &depth, cache)) < 0)
     {
       depth = 0;
       unw_init_local(&cur, &saved);
       while (depth < 128)
       {
         unw_word_t ip;
         unw_get_reg(&cur, UNW_REG_IP, &ip);
         addresses[depth++] = (void *) ip;
         if ((ret = unw_step(&cur)) <= 0)
           break;
       }
     }
*/
int
unw_tdep_trace (unw_cursor_t *cursor,
		void **buffer,
		int *size,
		unw_tdep_frame_t *cache)
{
  struct cursor *c = (struct cursor *) cursor;
  struct dwarf_cursor *d = &c->dwarf;
  unw_word_t rbp, rsp, rip, cfa;
  int maxdepth = 0;
  int depth = 0;
  int ret;
#if UNW_DEBUG
  unsigned long start, end;
  rdtsc(start);
#endif

  /* Check input parametres. */
  if (! cursor || ! buffer || ! size || ! cache || (maxdepth = *size) <= 0)
    return -UNW_EINVAL;

  Debug (1, "begin ip 0x%lx cfa 0x%lx\n", d->ip, d->cfa);

  /* Tell core dwarf routines to call back to us. */
  d->stash_frames = 1;

  /* Determine initial register values. */
  rip = d->ip;
  rsp = cfa = d->cfa;
  if ((ret = dwarf_get (d, d->loc[UNW_X86_64_RBP], &rbp)) < 0)
  {
    *size = 0;
    return ret;
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

    /* Record this address in stack trace. */
    buffer[depth++] = (void *) rip;

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
  }

#if UNW_DEBUG
  rdtsc(end);
  Debug (1, "returning %d depth %d, dt=%ld\n", ret, depth, end - start);
#endif
  *size = depth;
  return ret;
}
