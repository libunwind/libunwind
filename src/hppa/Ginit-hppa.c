/* libunwind - a platform-independent unwind library
   Copyright (C) 2002 Hewlett-Packard Co
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
#include <string.h>

#include "unwind_i.h"

#ifdef UNW_REMOTE_ONLY

/* unw_local_addr_space is a NULL pointer in this case.  */
PROTECTED unw_addr_space_t unw_local_addr_space;

#else /* !UNW_REMOTE_ONLY */

static struct unw_addr_space local_addr_space;

PROTECTED unw_addr_space_t unw_local_addr_space = &local_addr_space;

static inline void *
uc_addr (ucontext_t *uc, int reg)
{
  void *addr;

  switch (reg)
    {
    case UNW_X86_EAX: addr = &uc->uc_mcontext.gregs[REG_EAX]; break;
    case UNW_X86_EBX: addr = &uc->uc_mcontext.gregs[REG_EBX]; break;
    case UNW_X86_ECX: addr = &uc->uc_mcontext.gregs[REG_ECX]; break;
    case UNW_X86_EDX: addr = &uc->uc_mcontext.gregs[REG_EDX]; break;
    case UNW_X86_ESI: addr = &uc->uc_mcontext.gregs[REG_ESI]; break;
    case UNW_X86_EDI: addr = &uc->uc_mcontext.gregs[REG_EDI]; break;
    case UNW_X86_EBP: addr = &uc->uc_mcontext.gregs[REG_EBP]; break;
    case UNW_X86_EIP: addr = &uc->uc_mcontext.gregs[REG_EIP]; break;
    case UNW_X86_ESP: addr = &uc->uc_mcontext.gregs[REG_ESP]; break;

    default:
      addr = NULL;
    }
  return addr;
}

# ifdef UNW_LOCAL_ONLY

void *
_Ux86_uc_addr (ucontext_t *uc, int reg)
{
  return uc_addr (uc, reg);
}

# endif /* UNW_LOCAL_ONLY */

HIDDEN unw_dyn_info_list_t _U_dyn_info_list;

/* XXX fix me: there is currently no way to locate the dyn-info list
       by a remote unwinder.  On ia64, this is done via a special
       unwind-table entry.  Perhaps something similar can be done with
       DWARF2 unwind info.  */

static void
put_unwind_info (unw_addr_space_t as, unw_proc_info_t *proc_info, void *arg)
{
  /* it's a no-op */
}

static int
get_dyn_info_list_addr (unw_addr_space_t as, unw_word_t *dyn_info_list_addr,
			void *arg)
{
  *dyn_info_list_addr = (unw_word_t) &_U_dyn_info_list;
  return 0;
}

static int
access_mem (unw_addr_space_t as, unw_word_t addr, unw_word_t *val, int write,
	    void *arg)
{
  if (write)
    {
      debug (100, "%s: mem[%x] <- %x\n", __FUNCTION__, addr, *val);
      *(unw_word_t *) addr = *val;
    }
  else
    {
      *val = *(unw_word_t *) addr;
      debug (100, "%s: mem[%x] -> %x\n", __FUNCTION__, addr, *val);
    }
  return 0;
}

static int
access_reg (unw_addr_space_t as, unw_regnum_t reg, unw_word_t *val, int write,
	    void *arg)
{
  unw_word_t *addr;
  ucontext_t *uc = arg;

#if 0
  if (reg >= UNW_IA64_FR && reg < UNW_IA64_FR + 128)
    goto badreg;
#endif

  addr = uc_addr (uc, reg);
  if (!addr)
    goto badreg;

  if (write)
    {
      *(unw_word_t *) addr = *val;
      debug (100, "%s: %s <- %x\n", __FUNCTION__, unw_regname (reg), *val);
    }
  else
    {
      *val = *(unw_word_t *) addr;
      debug (100, "%s: %s -> %x\n", __FUNCTION__, unw_regname (reg), *val);
    }
  return 0;

 badreg:
  debug (1, "%s: bad register number %u\n", __FUNCTION__, reg);
  return -UNW_EBADREG;
}

static int
access_fpreg (unw_addr_space_t as, unw_regnum_t reg, unw_fpreg_t *val,
	      int write, void *arg)
{
#if 1
  printf ("access_fpreg: screams to get implemented, doesn't it?\n");
  return 0;
#else
  ucontext_t *uc = arg;
  unw_fpreg_t *addr;

  if (reg < UNW_IA64_FR || reg >= UNW_IA64_FR + 128)
    goto badreg;

  addr = uc_addr (uc, reg);
  if (!addr)
    goto badreg;

  if (write)
    {
      debug (100, "%s: %s <- %016lx.%016lx\n", __FUNCTION__,
	     unw_regname (reg), val->raw.bits[1], val->raw.bits[0]);
      *(unw_fpreg_t *) addr = *val;
    }
  else
    {
      *val = *(unw_fpreg_t *) addr;
      debug (100, "%s: %s -> %016lx.%016lx\n", __FUNCTION__,
	     unw_regname (reg), val->raw.bits[1], val->raw.bits[0]);
    }
  return 0;

 badreg:
  debug (1, "%s: bad register number %u\n", __FUNCTION__, reg);
  /* attempt to access a non-preserved register */
  return -UNW_EBADREG;
#endif
}

HIDDEN void
x86_local_addr_space_init (void)
{
  memset (&local_addr_space, 0, sizeof (local_addr_space));
  local_addr_space.caching_policy = UNW_CACHE_GLOBAL;
  local_addr_space.acc.find_proc_info = UNW_ARCH_OBJ (find_proc_info);
  local_addr_space.acc.put_unwind_info = put_unwind_info;
  local_addr_space.acc.get_dyn_info_list_addr = get_dyn_info_list_addr;
  local_addr_space.acc.access_mem = access_mem;
  local_addr_space.acc.access_reg = access_reg;
  local_addr_space.acc.access_fpreg = access_fpreg;
  local_addr_space.acc.resume = x86_local_resume;
}

#endif /* !UNW_REMOTE_ONLY */
