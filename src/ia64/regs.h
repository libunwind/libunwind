#include "unwind_i.h"

/* Apply rotation to a general register.  The number REG must be in
   the range of 0-127.  */

static inline int
rotate_gr (struct cursor *c, int reg)
{
  unsigned int rrb_gr, sor, sof;
  int preg;

  sof = (c->cfm & 0x7f);
  sor = 8 * ((c->cfm >> 14) & 0xf);
  rrb_gr = (c->cfm >> 18) & 0x7f;

  if ((unsigned) (reg - 32) > sof)
    return reg;	/* not a valid stacked register: just return original value */
  else if ((unsigned) (reg - 32) > sor)
    preg = reg;	/* register not part of the rotating partition */
  else
    {
      preg = reg + rrb_gr;	/* apply rotation */
      if (preg > 32 + (int) sor)
	preg -= sor;		/* wrap around */
    }
  debug (100, "%s: sor=%u rrb.gr=%u, r%d -> r%d\n",
	 __FUNCTION__, sor, rrb_gr, reg, preg);
  return preg;
}

/* Apply rotation to a floating-point register.  The number REG must
   be in the range of 0-127.  */

static inline int
rotate_fr (struct cursor *c, int reg)
{
  unsigned int rrb_fr;
  int preg;

  rrb_fr = (c->cfm >> 25) & 0x7f;
  if (reg < 32)
    preg = reg;		/* register not part of the rotating partition */
  else
    {
      preg = reg + rrb_fr;	/* apply rotation */
      if (preg > 127)
	preg -= 96;		/* wrap around */
    }
  debug (100, "%s: rrb.fr=%u, f%d -> f%d\n", __FUNCTION__, rrb_fr, reg, preg);
  return preg;
}

/* Apply logical-to-physical rotation.  */

static inline unw_word_t
pr_ltop (struct cursor *c, unw_word_t pr)
{
  unw_word_t rrb_pr, mask, rot;

  rrb_pr = (c->cfm >> 32) & 0x3f;
  rot = pr >> 16;
  mask = ((unw_word_t) 1 << rrb_pr) - 1;
  rot = ((pr & mask) << (48 - rrb_pr)) | ((pr >> rrb_pr) & mask);
  return (pr & 0xffff) | (rot << 16);
}

/* Apply physical-to-logical rotation.  */

static inline unw_word_t
pr_ptol (struct cursor *c, unw_word_t pr)
{
  unw_word_t rrb_pr, mask, rot;

  rrb_pr = 48 - ((c->cfm >> 32) & 0x3f);
  rot = pr >> 16;
  mask = ((unw_word_t) 1 << rrb_pr) - 1;
  rot = ((pr & mask) << (48 - rrb_pr)) | ((pr >> rrb_pr) & mask);
  return (pr & 0xffff) | (rot << 16);
}
