/* libunwind - a platform-independent unwind library
   Copyright (C) 2001-2003 Hewlett-Packard Co
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

#include <stdlib.h>

#include "rse.h"
#include "unwind_i.h"
#include "offsets.h"

#ifndef UNW_REMOTE_ONLY

HIDDEN inline int
ia64_local_resume (unw_addr_space_t as, unw_cursor_t *cursor, void *arg)
{
  struct cursor *c = (struct cursor *) cursor;
  long do_sigreturn = 0;
  unw_fpreg_t fpval;
  ucontext_t *uc = arg;
  unw_word_t val, sol;
  int i, ret;
# define SET_NAT(n)						\
  do								\
    {								\
      ret = ia64_access_reg (c, UNW_IA64_NAT + (n), &val, 0);	\
      if (ret < 0)						\
	return ret;						\
      if (val)							\
	uc->uc_mcontext.sc_nat |= (unw_word_t) 1 << n;		\
    }								\
  while (0)
# define SET_REG(f, r)				\
  do						\
    {						\
      ret = ia64_get (c, c->r, &val);		\
      if (ret < 0)				\
	return ret;				\
      uc->uc_mcontext.f = val;			\
    }						\
  while (0)
# define SET_FPREG(f, r)				\
  do							\
    {							\
      ret = ia64_getfp (c, c->r, &fpval);		\
      if (ret < 0)					\
	return ret;					\
      uc->uc_mcontext.f.u.bits[0] = fpval.raw.bits[0];	\
      uc->uc_mcontext.f.u.bits[1] = fpval.raw.bits[1];	\
    }							\
  while (0)

  /* ensure c->pi is up-to-date: */
  if ((ret = ia64_make_proc_info (c)) < 0)
    return ret;

  SET_REG (sc_ar_pfs, pfs_loc);
  SET_REG (sc_br[0], ip_loc);
  SET_REG (sc_pr, pr_loc);
  SET_REG (sc_ar_rnat, rnat_loc);
  SET_REG (sc_ar_lc, lc_loc);
  SET_REG (sc_ar_fpsr, fpsr_loc);

  SET_REG (sc_gr[4], r4_loc); SET_REG(sc_gr[5], r5_loc);
  SET_REG (sc_gr[6], r6_loc); SET_REG(sc_gr[7], r7_loc);
  uc->uc_mcontext.sc_nat = 0;
  SET_NAT (4); SET_NAT(5);
  SET_NAT (6); SET_NAT(7);

  SET_REG (sc_br[1], b1_loc);
  SET_REG (sc_br[2], b2_loc);
  SET_REG (sc_br[3], b3_loc);
  SET_REG (sc_br[4], b4_loc);
  SET_REG (sc_br[5], b5_loc);
  SET_FPREG (sc_fr[2], f2_loc);
  SET_FPREG (sc_fr[3], f3_loc);
  SET_FPREG (sc_fr[4], f4_loc);
  SET_FPREG (sc_fr[5], f5_loc);
  for (i = 16; i < 32; ++i)
    SET_FPREG (sc_fr[i], fr_loc[i - 16]);

  uc->uc_mcontext.sc_flags = 0;
  uc->uc_mcontext.sc_gr[1] = c->pi.gp;
  uc->uc_mcontext.sc_gr[12] = c->psp;

  if (unlikely (c->sigcontext_loc))
    {
      struct sigcontext *sc = (struct sigcontext *) c->sigcontext_loc;

      /* We're returning to a frame that was (either directly or
	 indirectly) interrupted by a signal.  We have to restore
	 _both_ "preserved" and "scratch" registers.  That doesn't
	 leave us any registers to work with, and the only way we can
	 achieve this is by doing a sigreturn().

	 Note: it might be tempting to think that we don't have to
	 restore the scratch registers when returning to a frame that
	 was indirectly interrupted by a signal.  However, that is not
	 safe because that frame and its descendants could have been
	 using a special convention that stores "preserved" state in
	 scratch registers.  The fsyscall convention does this with
	 r11 (to save ar.pfs) and b6 (to save "rp"), for example. */
      sc->sc_ip = uc->uc_mcontext.sc_br[0];
      sc->sc_gr[12] = c->psp;
      uc->uc_mcontext.sc_gr[12] = (c->sigcontext_loc - c->sigcontext_off);
      do_sigreturn = 1;

      /* Account for the fact that sigreturn will decrement bsp by
	 size-of-frame.  */
#if 0
      sof = (uc->uc_mcontext.sc_ar_pfs >> 0) & 0x7f;
      uc->uc_mcontext.sc_ar_bsp = ia64_rse_skip_regs (c->bsp, sof);
#else
      uc->uc_mcontext.sc_ar_bsp = sc->sc_ar_bsp;
#endif
printf("uc->bsp=%p, sc->bsp=%p\n", uc->uc_mcontext.sc_ar_bsp, sc->sc_ar_bsp);
    }
  else
    {
      /* Account for the fact that __ia64_install_context() returns
	 via br.ret, which will decrement bsp by size-of-locals.  */
      sol = (uc->uc_mcontext.sc_ar_pfs >> 7) & 0x7f;
printf("this needs fixing\n");
      uc->uc_mcontext.sc_ar_bsp = ia64_rse_skip_regs (c->bsp, sol);
    }
  __ia64_install_context (uc, c->eh_args[0], c->eh_args[1], c->eh_args[2],
			  c->eh_args[3], do_sigreturn);
}

#endif /* !UNW_REMOTE_ONLY */

int
unw_resume (unw_cursor_t *cursor)
{
  struct cursor *c = (struct cursor *) cursor;

#ifdef UNW_LOCAL_ONLY
  return ia64_local_resume (c->as, cursor, c->as_arg);
#else
  return (*c->as->acc.resume) (c->as, cursor, c->as_arg);
#endif
}
