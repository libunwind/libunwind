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

#ifndef unwind_i_h
#define unwind_i_h

#include <memory.h>
#include <stdint.h>

#include <libunwind-x86.h>

#include "internal.h"
#include "tdep.h"

#define X86_GET_LOC(l)		((l).val)

#ifdef UNW_LOCAL_ONLY
# define X86_LOC(r, t)		((struct x86_loc) { .val = (r) })
# define X86_REG_LOC(c,r)	(X86_LOC((unw_word_t)			     \
					 tdep_uc_addr((c)->as_arg, (r)), 0))
# define X86_FPREG_FLOC(c,r)	(X86_LOC((unw_word_t)			     \
					 tdep_uc_addr((c)->as_arg, (r)), 0))

static inline int
x86_getfp (struct cursor *c, struct x86_loc loc, unw_fpreg_t *val)
{
  if (!X86_GET_LOC (loc))
    return -1;
  *val = *(unw_fpreg_t *) X86_GET_LOC (loc);
  return 0;
}

static inline int
x86_putfp (struct cursor *c, struct x86_loc loc, unw_fpreg_t *val)
{
  if (!X86_GET_LOC (loc))
    return -1;
  *(unw_fpreg_t *) X86_GET_LOC (loc) = *val;
  return 0;
}

static inline int
x86_get (struct cursor *c, struct x86_loc loc, unw_word_t *val)
{
  if (!X86_GET_LOC (loc))
    return -1;
  *val = *(unw_word_t *) X86_GET_LOC (loc);
  return 0;
}

static inline int
x86_put (struct cursor *c, struct x86_loc loc, unw_word_t val)
{
  if (!X86_GET_LOC (loc))
    return -1;
  *(unw_word_t *) X86_GET_LOC (loc) = val;
  return 0;
}

#else /* !UNW_LOCAL_ONLY */
# define X86_LOC_TYPE_FP	(1 << 0)
# define X86_LOC_TYPE_REG	(1 << 1)
# define X86_LOC(r, t)		((struct x86_loc) { .val = (r), .type = (t) })
# define X86_IS_REG_LOC(l)	(((l).type & X86_LOC_TYPE_REG) != 0)
# define X86_IS_FP_LOC(l)	(((l).type & X86_LOC_TYPE_FP) != 0)
# define X86_REG_LOC(c,r)	X86_LOC((r), X86_LOC_TYPE_REG)
# define X86_FPREG_LOC(c,r)	X86_LOC((r), (X86_LOC_TYPE_REG		\
					     | X86_LOC_TYPE_FP))

static inline int
x86_getfp (struct cursor *c, struct x86_loc loc, unw_fpreg_t *val)
{
  abort ();
}

static inline int
x86_putfp (struct cursor *c, struct x86_loc loc, unw_fpreg_t val)
{
  abort ();
}

static inline int
x86_get (struct cursor *c, struct x86_loc loc, unw_word_t *val)
{
  if (X86_IS_FP_LOC (loc))
    abort ();

  if (X86_IS_REG_LOC (loc))
    return (*c->as->acc.access_reg)(c->as, X86_GET_LOC (loc), val, 0,
				    c->as_arg);
  else
    return (*c->as->acc.access_mem)(c->as, X86_GET_LOC (loc), val, 0,
				    c->as_arg);
}

static inline int
x86_put (struct cursor *c, struct x86_loc loc, unw_word_t val)
{
  if (X86_IS_FP_LOC (loc))
    abort ();

  if (X86_IS_REG_LOC (loc))
    return (*c->as->acc.access_reg)(c->as, X86_GET_LOC (loc), &val, 1,
				    c->as_arg);
  else
    return (*c->as->acc.access_mem)(c->as, X86_GET_LOC (loc), &val, 1,
				    c->as_arg);
}

#endif /* !UNW_LOCAL_ONLY */

#define x86_needs_initialization	UNW_ARCH_OBJ(needs_initialization)
#define x86_init			UNW_ARCH_OBJ(init)
#define x86_access_reg			UNW_OBJ(access_reg)
#define x86_access_fpreg		UNW_OBJ(access_fpreg)
#define x86_local_resume		UNW_OBJ(local_resume)
#define x86_local_addr_space_init	UNW_OBJ(local_addr_space_init)

extern int x86_needs_initialization;

extern void x86_init (void);
extern int x86_access_reg (struct cursor *c, unw_regnum_t reg,
			   unw_word_t *valp, int write);
extern int x86_access_fpreg (struct cursor *c, unw_regnum_t reg,
			     unw_fpreg_t *valp, int write);
extern void x86_local_addr_space_init (void);
extern int x86_local_resume (unw_addr_space_t as, unw_cursor_t *cursor,
			     void *arg);

#endif /* unwind_i_h */
