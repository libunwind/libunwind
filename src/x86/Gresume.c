/* libunwind - a platform-independent unwind library
   Copyright (c) 2002-2004 Hewlett-Packard Development Company, L.P.
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

#include "unwind_i.h"

#ifndef UNW_REMOTE_ONLY

HIDDEN inline int
x86_local_resume (unw_addr_space_t as, unw_cursor_t *cursor, void *arg)
{
#if defined(__linux)
  struct cursor *c = (struct cursor *) cursor;
  ucontext_t *uc = c->dwarf.as_arg;

  /* Ensure c->pi is up-to-date.  On x86, it's relatively common to be
     missing DWARF unwind info.  We don't want to fail in that case,
     because the frame-chain still would let us do a backtrace at
     least.  */
  dwarf_make_proc_info (&c->dwarf);

  if (unlikely (c->sigcontext_format != X86_SCF_NONE))
    {
      struct sigcontext *sc = (struct sigcontext *) c->sigcontext_addr;

#if 0
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
	 scratch registers.  For example, the Linux fsyscall
	 convention does this with r11 (to save ar.pfs) and b6 (to
	 save "rp"). */

      sc->sc_gr[12] = c->psp;
      c->psp = c->sigcontext_addr - c->sigcontext_off;

      /* Clear the "in-syscall" flag, because in general we won't be
	 returning to the interruption-point and we need all registers
	 restored.  */
      sc->sc_flags &= ~IA64_SC_FLAG_IN_SYSCALL;
      sc->sc_ip = c->ip;
      sc->sc_cfm = c->cfm & (((unw_word_t) 1 << 38) - 1);
      sc->sc_pr = (c->pr & ~PR_SCRATCH) | (sc->sc_pr & ~PR_PRESERVED);
      if ((ret = ia64_get (c, c->loc[IA64_REG_PFS], &sc->sc_ar_pfs)) < 0
	  || (ret = ia64_get (c, c->loc[IA64_REG_FPSR], &sc->sc_ar_fpsr)) < 0
	  || (ret = ia64_get (c, c->loc[IA64_REG_UNAT], &sc->sc_ar_unat)) < 0)
	return ret;

      sc->sc_gr[1] = c->pi.gp;
      if (c->eh_valid_mask & 0x1) sc->sc_gr[15] = c->eh_args[0];
      if (c->eh_valid_mask & 0x2) sc->sc_gr[16] = c->eh_args[1];
      if (c->eh_valid_mask & 0x4) sc->sc_gr[17] = c->eh_args[2];
      if (c->eh_valid_mask & 0x8) sc->sc_gr[18] = c->eh_args[3];
#endif

      Debug (8, "resuming at ip=%x via sigreturn(%p)\n", c->dwarf.ip, sc);
      sigreturn (sc);
    }
  else
    {
      Debug (8, "resuming at ip=%x via setcontext()\n", c->dwarf.ip);
      setcontext (uc);
    }
#else
# warning Implement me!
#endif
  return -UNW_EINVAL;
}

#endif /* !UNW_REMOTE_ONLY */

/* This routine is responsible for copying the register values in
   cursor C and establishing them as the current machine state. */

static inline int
establish_machine_state (struct cursor *c)
{
  int (*access_reg) (unw_addr_space_t, unw_regnum_t, unw_word_t *,
		     int write, void *);
  int (*access_fpreg) (unw_addr_space_t, unw_regnum_t, unw_fpreg_t *,
		       int write, void *);
  unw_addr_space_t as = c->dwarf.as;
  void *arg = c->dwarf.as_arg;
  unw_fpreg_t fpval;
  unw_word_t val;
  int reg;

  access_reg = as->acc.access_reg;
  access_fpreg = as->acc.access_fpreg;

  Debug (8, "copying out cursor state\n");

  for (reg = 0; reg < UNW_REG_LAST; ++reg)
    {
Debug (16, "copying %s\n", unw_regname (reg));
      if (unw_is_fpreg (reg))
	{
	  if (tdep_access_fpreg (c, reg, &fpval, 0) >= 0)
	    (*access_fpreg) (as, reg, &fpval, 1, arg);
	}
      else
	{
	  if (tdep_access_reg (c, reg, &val, 0) >= 0)
	    (*access_reg) (as, reg, &val, 1, arg);
	}
    }
  return 0;
}

PROTECTED int
unw_resume (unw_cursor_t *cursor)
{
  struct cursor *c = (struct cursor *) cursor;
  int ret;

  Debug (1, "(cursor=%p)\n", c);

  if ((ret = establish_machine_state (c)) < 0)
    return ret;

  return (*c->dwarf.as->acc.resume) (c->dwarf.as, (unw_cursor_t *) c,
				     c->dwarf.as_arg);
}
