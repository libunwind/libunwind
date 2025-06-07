/* libunwind - a platform-independent unwind library
   Copyright (C) 2003-2004 Hewlett-Packard Co
        Contributed by David Mosberger

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
#include "offsets.h"

static int
hppa_handle_signal_frame (unw_cursor_t *cursor)
{
  int ret, i;
  struct cursor *c = (struct cursor *) cursor;
  unw_word_t ip, sc_addr, sp, sp_addr = c->dwarf.cfa;
  struct dwarf_loc sp_loc = DWARF_LOC (sp_addr, 0);

  if ((ret = dwarf_get (&c->dwarf, sp_loc, &sp)) < 0)
    return -UNW_EUNSPEC;

#ifdef __linux__
  unw_word_t insn, offset;
  unw_addr_space_t as;
  unw_accessors_t *a;
  void *arg;

  as = c->dwarf.as;
  a = unw_get_accessors_int (as);
  arg = c->dwarf.as_arg;

  ip = c->dwarf.ip & ~3;
  sp = c->dwarf.ip & ~63;
  
  /* We need to find the address of the signal mcontext structure on
     the stack.  Prior to implementing VDSO, it is found at various
     offsets relative to the start of the stack trampoline.

     sp + offset points to a struct rt_sigframe, which contains a struct
     siginfo and a struct ucontext.  struct ucontext contains a struct
     mcontext with the signal frame context.

     sizeof(struct siginfo) == 128
     offsetof(struct ucontext, uc_mcontext) == 24.

     When we have a VDSO signal frame, the offset of the signal mcontext
     struct is fount at the beginning of the rt_sigframe.  It is relative
     to sp_addr.  */

  switch (ip - sp)
    {
    case 0:
      sc_addr = sp + 4*4 + 128 + 24;
      break;

    case 2*4:
    case 6*4:
      /* Check for nop instruction before trampoline.  */
      if ((ret = (*a->access_mem) (as, ip - 4, &insn, 0, arg)) < 0)
        {
          Debug (1, "failed to read insn at ip - 3 (ret=%d)\n", ret);
          return ret;
        }

      if (insn == 0x08000240)
        {
          /* VDSO trampoline */
          if ((ret = (*a->access_mem) (as, sp, &offset, 0, arg)) < 0)
            {
              Debug (1, "failed to read mcontext stack offset (ret=%d)\n", ret);
              return ret;
            }

          sc_addr = sp_addr + (int)offset;
        }
      else
        {
          /* sigaltstack trampoline */
          sp = ip - 5*4;
          sc_addr = sp + 10*4 + 128 + 24;
        }
      break;

    case 4*4:
    case 5*4:
      sc_addr = sp + 10*4 + 128 + 24;
      break;

    default:
      /* sigaltstack case: we have no way of knowing which offset to
         use in this case; default to new kernel handling.  If this
         is wrong the unwinding will fail.  */
      sp = ip - 5*4;
      sc_addr = sp + 10*4 + 128 + 24;
      break;
    }

  c->sigcontext_format = HPPA_SCF_LINUX_RT_SIGFRAME;
  c->sigcontext_addr = sc_addr;
  c->sigcontext_sp = sp_addr;

  dwarf_loc_t iaoq_loc = DWARF_LOC (sc_addr + LINUX_SC_IAOQ_OFF, 0);
  if ((ret = dwarf_get (&c->dwarf, iaoq_loc, &ip)) < 0)
    {
      Debug (2, "failed to read IAOQ[1] (ret=%d)\n", ret);
      return ret;
    }
  c->dwarf.ip = ip;
#else
  /* Not making any assumption at all - You need to implement this */
  return -UNW_EUNSPEC;
#endif

#ifdef __LP64__
  for (i = 0; i < 32; ++i)
    {
      c->dwarf.loc[UNW_HPPA_GR + i]
        = DWARF_LOC (sc_addr + LINUX_SC_GR_OFF + 8*i, 0);
    }

  for (i = 0; i < 28; ++i)
    {
      c->dwarf.loc[UNW_HPPA_FR + i]
        = DWARF_LOC (sc_addr + LINUX_SC_FR_OFF + 8*4 + 8*i, 0);
    }
#else
  for (i = 0; i < 32; ++i)
    {
      c->dwarf.loc[UNW_HPPA_GR + i]
        = DWARF_LOC (sc_addr + LINUX_SC_GR_OFF + 4*i, 0);
    }

  for (i = 0; i < 56; ++i)
    {
      c->dwarf.loc[UNW_HPPA_FR + i]
        = DWARF_LOC (sc_addr + LINUX_SC_FR_OFF + 8*4 + 4*i, 0);
    }
#endif

  if ((ret = dwarf_get (&c->dwarf, c->dwarf.loc[UNW_HPPA_SP],
                        &c->dwarf.cfa)) < 0)
    {
      Debug (2, "failed to read SP (ret=%d)\n", ret);
      return ret;
    }

#if 0
  /* Set SP/CFA and PC/IP.  */
  dwarf_get (&c->dwarf, c->dwarf.loc[UNW_TDEP_SP], &c->dwarf.cfa);
  dwarf_get (&c->dwarf, c->dwarf.loc[UNW_TDEP_IP], &c->dwarf.ip);
#endif

  return 1;
}

int
unw_step (unw_cursor_t *cursor)
{
  struct cursor *c = (struct cursor *) cursor;
  int validate = c->validate;
  int ret;

  Debug (1, "(cursor=%p, ip=0x%08x)\n", c, (unsigned) c->dwarf.ip);

  /* Validate all addresses before dereferencing. */
  c->validate = 1;

  /* Special handling the signal frame. */
  if (unw_is_signal_frame (cursor) > 0)
    return hppa_handle_signal_frame (cursor);

  /* Restore default memory validation state */
  c->validate = validate;

  /* Try DWARF-based unwinding... */
  ret = dwarf_step (&c->dwarf);

  if (unlikely (ret == -UNW_ESTOPUNWIND))
    return ret;

#if 0
  if (ret < 0 && ret != -UNW_ENOINFO)
    {
      Debug (2, "returning %d\n", ret);
      return ret;
    }
#endif

  if (unlikely (ret < 0))
    {
      /* DWARF failed, let's see if we can follow the frame-chain
         or skip over the signal trampoline.  */

      Debug (13, "dwarf_step() failed (ret=%d), trying fallback\n", ret);

      c->dwarf.ip = 0;
    }
  ret = (c->dwarf.ip == 0) ? 0 : 1;
  Debug (2, "returning %d\n", ret);
  return ret;
}
