/* libunwind - a platform-independent unwind library
   Copyright (C) 2003 Hewlett-Packard Co
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

/* Logically, we like to think of the stack as a contiguous region of
memory.  Unfortunately, this logical view doesn't work for the
register backing store, because the RSE is an asynchronous engine and
because UNIX/Linux allow for stack-switching via sigaltstack(2).
Specifically, this means that any given stacked register may or may
not be backed up by memory in the current stack.  If not, then the
backing memory may be found in any of the "more inner" (younger)
stacks.  The routines in this file help manage the discontiguous
nature of the register backing store.  The routines are completely
independent of UNIX/Linux, but each stack frame that switches the
backing store is expected to reserve 4 words for use by libunwind. For
example, in the Linux sigcontext, sc_fr[0] and sc_fr[1] serve this
purpose.  */

#include "unwind_i.h"

HIDDEN int
rbs_record_switch (struct cursor *c,
		   unw_word_t saved_bsp, unw_word_t saved_bspstore,
		   unw_word_t saved_rnat_loc)
{
  unw_word_t bsp, i, j, lo, ndirty, nregs, right_edge = c->rbs_right_edge;

  debug (10, "%s: (left=%u, curr=%u, right=%u)\n\t", __FUNCTION__,
	 c->rbs_left_edge, c->rbs_curr, c->rbs_right_edge);

  bsp = c->bsp;

  /* If other stacks are already pending, we need to convert "bsp" to
     equivalent address on the final stack.  The conversion is easy
     because we know that the low end of a stack corresponds to the
     high end of the next stack. */
  for (i = c->rbs_curr; i != right_edge; i = (i + 1) % NELEMS (c->rbs_area))
    {
      j = (i + 1) % NELEMS (c->rbs_area);
      nregs = ia64_rse_num_regs (c->rbs_area[i].end - c->rbs_area[i].size,
				 bsp);
      bsp = ia64_rse_skip_regs (c->rbs_area[j].end, nregs);
    }

  /* Calculate address "lo" at which the backing store starts:  */
  ndirty = ia64_rse_num_regs (saved_bspstore, saved_bsp);
  lo = ia64_rse_skip_regs (bsp, -ndirty);

  c->rbs_area[right_edge].size = (c->rbs_area[right_edge].end - lo);

  /* If the previously-recorded rbs-area is empty we don't need to
     track it and we can simply overwrite it... */
  if (c->rbs_area[right_edge].size)
    {
      debug (10, "inner=[0x%lx-0x%lx)\n\t",
	     (long) (c->rbs_area[right_edge].end
		     - c->rbs_area[right_edge].size),
	     (long) c->rbs_area[right_edge].end);

      right_edge = (right_edge + 1) % NELEMS (c->rbs_area);
      c->rbs_right_edge = right_edge;

      assert (right_edge != c->rbs_curr);

      if (right_edge == c->rbs_left_edge)
	c->rbs_left_edge = (right_edge + 1) % NELEMS (c->rbs_area);
    }
  c->rbs_area[right_edge].end = saved_bspstore;
  c->rbs_area[right_edge].size = ~(unw_word_t) 0;	/* initial guess... */
  c->rbs_area[right_edge].rnat_loc = saved_rnat_loc;

  debug (10, "outer=[?????????????????\?-0x%lx), rnat@0x%lx\n",
	 (long) c->rbs_area[right_edge].end,
	 (long) c->rbs_area[right_edge].rnat_loc);
  return 0;
}

HIDDEN void
rbs_underflow (struct cursor *c)
{
  size_t num_regs;

  assert (c->rbs_area[c->rbs_curr].end - c->bsp
	  > c->rbs_area[c->rbs_curr].size);

  while (1)
    {
      unw_word_t old_base = (c->rbs_area[c->rbs_curr].end
			     - c->rbs_area[c->rbs_curr].size);

      /* # of register we were short on old rbs-area: */
      num_regs = ia64_rse_num_regs (c->bsp, old_base);

      c->rbs_curr = (c->rbs_curr + 1) % NELEMS (c->rbs_area);
      c->bsp = ia64_rse_skip_regs (c->rbs_area[c->rbs_curr].end, -num_regs);

      if (c->rbs_area[c->rbs_curr].end - c->bsp
	  <= c->rbs_area[c->rbs_curr].size)
	{
	  debug (10, "%s: [0x%016lx-0x%016lx), bsp=0x%lx\n",
		 __FUNCTION__, (long) (c->rbs_area[c->rbs_curr].end
				       - c->rbs_area[c->rbs_curr].size),
		 (long) c->rbs_area[c->rbs_curr].end, c->bsp);
	  return;
	}

      assert (c->rbs_curr != c->rbs_right_edge);
    }
}

HIDDEN int
rbs_find_stacked (struct cursor *c, unw_word_t regs_to_skip,
		  unw_word_t *locp, unw_word_t *rnat_locp)
{
  unw_word_t nregs, bsp = c->bsp, curr = c->rbs_curr;
  unw_word_t left_edge = c->rbs_left_edge;
  int reg = 32 + regs_to_skip;

  while (1)
    {
      nregs = ia64_rse_num_regs (bsp, c->rbs_area[curr].end);

      if (regs_to_skip < nregs)
	{
	  /* found it: */
	  *locp = ia64_rse_skip_regs (bsp, regs_to_skip);
	  if (rnat_locp)
	    {
	      *rnat_locp = ia64_rse_rnat_addr (*locp);
	      if (*rnat_locp >= c->rbs_area[curr].end)
		*rnat_locp = c->rbs_area[curr].rnat_loc;
	    }
	  return 0;
	}

      if (curr == left_edge)
	{
	  debug (1, "%s: could not find register r%d!\n", __FUNCTION__, reg);
	  return -UNW_EBADREG;
	}

      regs_to_skip -= nregs;

      curr = (curr + NELEMS (c->rbs_area) - 1) % NELEMS (c->rbs_area);
      bsp = c->rbs_area[curr].end - c->rbs_area[curr].size;
    }
}

static inline int
get_rnat (struct cursor *c, struct rbs_area *rbs, unw_word_t bsp,
	  unw_word_t *__restrict rnat_locp, unw_word_t *__restrict rnatp)
{
  *rnat_locp = ia64_rse_rnat_addr (bsp);
  if (*rnat_locp >= rbs->end)
    *rnat_locp = rbs->rnat_loc;
  return ia64_get (c, *rnat_locp, rnatp);
}

/* Ensure that the first "nregs" stacked registers are on the current
   register backing store area.  This effectively simulates the effect
   of a "cover" followed by a "flushrs" for the current frame.

   Note: This does not modify the rbs_area[] structure in any way.  */
HIDDEN int
rbs_cover_and_flush (struct cursor *c, unw_word_t nregs)
{
  unw_word_t src_bsp, dst_bsp, src_rnat_loc, dst_rnat_loc, src_rnat, dst_rnat;
  unw_word_t src_mask, dst_mask, curr, val, left_edge = c->rbs_left_edge;
  unw_word_t final_bsp;
  struct rbs_area *dst_rbs;
  int ret;

  if (nregs < 1)
    return 0;		/* nothing to do... */

  /* Handle the common case quickly: */

  curr = c->rbs_curr;
  dst_rbs = c->rbs_area + curr;
  final_bsp = ia64_rse_skip_regs (c->bsp, nregs);	/* final bsp */
  if (likely (final_bsp <= dst_rbs->end))
    {
      c->bsp = final_bsp;
      return 0;
    }

  /* Skip over regs that are already on the destination rbs-area: */

  dst_bsp = src_bsp = dst_rbs->end;

  if ((ret = get_rnat (c, dst_rbs, dst_bsp, &dst_rnat_loc, &dst_rnat)) < 0)
    return ret;

  while (dst_bsp < final_bsp)
    {
      while (src_bsp >= c->rbs_area[curr].end)
	{
	  /* switch to next rbs-area, adjust src_bsp accordingly: */
	  if (curr == left_edge)
	    {
	      debug (1, "%s: rbs-underflow while flushing %lu regs, "
		     "src_bsp=0x%lx, dst_bsp=0x%lx\n",
		     __FUNCTION__, nregs, src_bsp, dst_bsp);
	      return -UNW_EBADREG;
	    }
	  curr = (curr + NELEMS (c->rbs_area) - 1) % NELEMS (c->rbs_area);
	  src_bsp = c->rbs_area[curr].end - c->rbs_area[curr].size;
	}

      /* OK, found the right rbs-area.  Now copy both the register
	 value and its NaT bit:  */

      if ((ret = get_rnat (c, c->rbs_area + curr, src_bsp,
			   &src_rnat_loc, &src_rnat)) < 0)
	return ret;

      src_mask = ((unw_word_t) 1) << ia64_rse_slot_num (src_bsp);
      dst_mask = ((unw_word_t) 1) << ia64_rse_slot_num (dst_bsp);

      if (src_rnat & src_mask)
	dst_rnat |= dst_mask;
      else
	dst_rnat &= ~dst_mask;

      if ((ret = ia64_get (c, src_bsp, &val)) < 0
	  || (ret = ia64_put (c, dst_bsp, val)) < 0)
	return ret;

      /* advance src_bsp & dst_bsp to next slot: */

      src_bsp += 8;
      if (ia64_rse_is_rnat_slot (src_bsp))
	src_bsp += 8;

      dst_bsp += 8;
      if (ia64_rse_is_rnat_slot (dst_bsp))
	{
	  if ((ret = ia64_put (c, dst_rnat_loc, dst_rnat)) < 0)
	    return ret;

	  dst_bsp += 8;

	  if ((ret = get_rnat (c, dst_rbs, dst_bsp,
			       &dst_rnat_loc, &dst_rnat)) < 0)
	    return ret;
	}
    }
  c->bsp = dst_bsp;
  return ia64_put (c, dst_rnat_loc, dst_rnat);
}
