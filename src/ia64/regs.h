#include "unwind_i.h"

/* Apply rotation to a general register.  The number REG must be in
   the range of 0-127.  */

static inline int
rotate_gr (struct ia64_cursor *c, int reg)
{
  unsigned int rrb_gr, sor;
  unw_word_t cfm;
  int preg, ret;

  ret = ia64_get (c, c->cfm_loc, &cfm);
  if (ret < 0)
    return ret;

  sor = 8 * ((cfm >> 14) & 0xf);
  rrb_gr = (cfm >> 18) & 0x7f;

  if ((unsigned) (reg - 32) > sor)
    preg = reg;		/* register not part of the rotating partition */
  else
    {
      preg = reg + rrb_gr;	/* apply rotation */
      if (preg > 32 + sor)
	preg -= sor;		/* wrap around */
    }
  debug (100, "%s: sor=%u rrb.gr=%u, r%d -> r%d\n", __FUNCTION__, sor, rrb_gr,
	 reg, preg);
  return preg;
}

/* Apply rotation to a floating-point register.  The number REG must
   be in the range of 0-127.  */

static inline int
rotate_fr (struct ia64_cursor *c, int reg)
{
  unsigned int rrb_fr;
  unw_word_t cfm;
  int preg, ret;

  ret = ia64_get (c, c->cfm_loc, &cfm);
  if (ret < 0)
    return ret;

  rrb_fr = (cfm >> 25) & 0x7f;
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
pr_ltop (struct ia64_cursor *c, unw_word_t pr)
{
  unw_word_t rrb_pr, mask, rot, cfm;
  int ret;

  ret = ia64_get (c, c->cfm_loc, &cfm);
  if (ret < 0)
    return ret;

  rrb_pr = (cfm >> 32) & 0x3f;
  rot = pr >> 16;
  mask = ((unw_word_t) 1 << rrb_pr) - 1;
  rot = ((pr & mask) << (48 - rrb_pr)) | ((pr >> rrb_pr) & mask);
  return (pr & 0xffff) | (rot << 16);
}

/* Apply physical-to-logical rotation.  */

static inline unw_word_t
pr_ptol (struct ia64_cursor *c, unw_word_t pr)
{
  unw_word_t rrb_pr, mask, rot, cfm;
  int ret;

  ret = ia64_get (c, c->cfm_loc, &cfm);
  if (ret < 0)
    return ret;

  rrb_pr = 48 - ((cfm >> 32) & 0x3f);
  rot = pr >> 16;
  mask = ((unw_word_t) 1 << rrb_pr) - 1;
  rot = ((pr & mask) << (48 - rrb_pr)) | ((pr >> rrb_pr) & mask);
  return (pr & 0xffff) | (rot << 16);
}
