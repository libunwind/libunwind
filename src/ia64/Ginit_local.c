/* libunwind - a platform-independent unwind library
   Copyright (C) 2001-2003, 2005 Hewlett-Packard Co
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

#include "init.h"
#include "unwind_i.h"

#ifdef UNW_REMOTE_ONLY

int
unw_init_local (unw_cursor_t *cursor, unw_context_t *uc)
{
  return -UNW_EINVAL;
}

#else /* !UNW_REMOTE_ONLY */

static inline void
set_as_arg (struct cursor *c, unw_context_t *uc)
{
#if defined(__linux__) && defined(__KERNEL__)
  c->task = current;
  c->as_arg = &uc->sw;
#else
  c->as_arg = uc;
#endif
}

static inline int
get_initial_stack_pointers (struct cursor *c, unw_context_t *uc,
                            int signal_frame, unw_word_t *sp, unw_word_t *bsp)
{
#if defined(__linux__)
  unw_word_t nregs, bspstore;

#ifdef __KERNEL__
  nregs = (uc->sw.ar_pfs >> 7) & 0x7f;
  bspstore = uc->sw.ar_bspstore;
  *sp = uc->ksp;
# else
  if (signal_frame)
    /* The kernel saved ar.bsp after covering the interrupted frame.  */
    nregs = uc->uc_mcontext.sc_cfm & 0x7f;
  else
    nregs = (uc->uc_mcontext.sc_ar_pfs >> 7) & 0x7f;
  bspstore = uc->uc_mcontext.sc_ar_bsp;
  *sp = uc->uc_mcontext.sc_gr[12];
# endif
  *bsp = rse_skip_regs (bspstore, -nregs);
#elif defined(__hpux)
  int ret;

  if ((ret = ia64_get (c, IA64_REG_LOC (c, UNW_IA64_GR + 12), sp)) < 0
      || (ret = ia64_get (c, IA64_REG_LOC (c, UNW_IA64_AR_BSP), bsp)) < 0)
    return ret;
#else
# error Fix me.
#endif
  return 0;
}

#if defined(__linux__) && !defined(__KERNEL__)

/* UC is the context the kernel passed to a signal handler: the IP and
   frame marker are in sc_ip and sc_cfm rather than b0 and ar.pfs.  Leave
   the cursor as unw_step() does after stepping out of the trampoline,
   except for ar.ec: that was saved in the trampoline's frame, which is
   out of reach here, so sc_ar_pfs has to do.  */
static inline int
init_signal_frame (struct cursor *c, unw_context_t *uc)
{
  mcontext_t *sc = &uc->uc_mcontext;
  int ret;

  c->loc[IA64_REG_IP] = IA64_LOC_ADDR ((unw_word_t) &sc->sc_ip, 0);
  c->cfm_loc = IA64_LOC_ADDR ((unw_word_t) &sc->sc_cfm, 0);
  c->loc[IA64_REG_PFS] = IA64_LOC_ADDR ((unw_word_t) &sc->sc_ar_pfs, 0);
  c->ec_loc = c->loc[IA64_REG_PFS];

  if ((ret = ia64_get (c, c->loc[IA64_REG_IP], &c->ip)) < 0
      || (ret = ia64_get (c, c->cfm_loc, &c->cfm)) < 0)
    return ret;

  c->sigcontext_addr = (unw_word_t) sc;
  c->sigcontext_off = c->sigcontext_addr - c->sp;
  c->last_abi_marker = ABI_MARKER_LINUX_SIGTRAMP;
  ia64_set_abi (c, ABI_LINUX);
  return 0;
}

#endif

static int
unw_init_local_common (unw_cursor_t *cursor, unw_context_t *uc,
                       int signal_frame)
{
  struct cursor *c = (struct cursor *) cursor;
  unw_word_t sp, bsp;
  int ret;

  if (!atomic_load(&tdep_init_done))
    tdep_init ();

  Debug (1, "(cursor=%p)\n", c);

  c->as = unw_local_addr_space;
  set_as_arg (c, uc);

  if ((ret = get_initial_stack_pointers (c, uc, signal_frame, &sp, &bsp)) < 0)
    return ret;

  Debug (4, "initial bsp=%lx, sp=%lx\n", bsp, sp);

  if ((ret = common_init (c, sp, bsp)) < 0)
    return ret;

#if defined(__linux__) && !defined(__KERNEL__)
  if (signal_frame)
    return init_signal_frame (c, uc);
#endif
#ifdef __hpux
  /* On HP-UX, the context created by getcontext() points to the
     getcontext() system call stub.  Step over it: */
  ret = unw_step (cursor);
#endif
  return ret;
}

int
unw_init_local (unw_cursor_t *cursor, unw_context_t *uc)
{
  return unw_init_local_common (cursor, uc, 0);
}

int
unw_init_local2 (unw_cursor_t *cursor, unw_context_t *uc, int flag)
{
  if (!flag)
    return unw_init_local_common (cursor, uc, 0);
  else if (flag == UNW_INIT_SIGNAL_FRAME)
    return unw_init_local_common (cursor, uc, 1);
  else
    return -UNW_EINVAL;
}

#endif /* !UNW_REMOTE_ONLY */
