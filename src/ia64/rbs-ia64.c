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
  unw_word_t ndirty, prev, last = c->rbs_wridx;

  debug (10, "%s: ", __FUNCTION__);

  ndirty = ia64_rse_num_regs (saved_bspstore, saved_bsp);

  if (ndirty == 0)
    /* The last-recorded rbs-area is empty.  No point in tracking it... */
    prev = last;
  else
    {
      c->rbs_area[last].size = (c->rbs_area[last].end
				- ia64_rse_skip_regs (c->bsp, -ndirty));
      debug (10, "\n\tinner=[0x%lx-0x%lx),\n\t",
	     (long) (c->rbs_area[last].end - c->rbs_area[last].size),
	     (long) c->rbs_area[last].end);

      last = ((last + NELEMS (c->rbs_area) - 1) % NELEMS (c->rbs_area));

      ++c->rbs_nvalid;
      if (c->rbs_nvalid > NELEMS (c->rbs_area))
	c->rbs_nvalid = NELEMS (c->rbs_area);

      assert (last != c->rbs_curr);
    }
  c->rbs_area[last].end = saved_bspstore;
  c->rbs_area[last].size = ~(unw_word_t) 0;	/* initial guess... */
  c->rbs_area[last].rnat_loc = saved_rnat_loc;

  debug (10, "outer=[??????????????????-0x%lx), rnat @ 0x%lx\n",
	 (long) c->rbs_area[last].end, (long) c->rbs_area[last].rnat_loc);
  return 0;
}

HIDDEN void
rbs_underflow (struct cursor *c)
{
  size_t num_regs;

  while (c->rbs_area[c->rbs_curr].end - c->bsp
	 > c->rbs_area[c->rbs_curr].size)
    {
      unw_word_t old_base = (c->rbs_area[c->rbs_curr].end
			     - c->rbs_area[c->rbs_curr].size);

      /* # of register we were short on old rbs-area: */
      num_regs = ia64_rse_num_regs (c->bsp, old_base);

      c->rbs_curr = ((c->rbs_curr + NELEMS (c->rbs_area) - 1)
		     % NELEMS (c->rbs_area));
      c->bsp = ia64_rse_skip_regs (c->rbs_area[c->rbs_curr].end, -num_regs);
    }
}

HIDDEN int
rbs_find_stacked (struct cursor *c, unw_word_t regs_to_skip,
		  unw_word_t *locp, unw_word_t *rnat_locp)
{
  unw_word_t curr, nregs, bsp = c->bsp;
  int i;

  curr = c->rbs_curr;

  for (i = 0; i < c->rbs_nvalid; ++i)
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

      regs_to_skip -= nregs;

      curr = (curr + 1) % NELEMS (c->rbs_area);
      bsp = c->rbs_area[curr].end - c->rbs_area[curr].size;
    }
  debug (1, "%s: could not find register r%d!\n",
	 __FUNCTION__, 32 + regs_to_skip);
  return -UNW_EINVAL;
}
