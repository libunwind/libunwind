/* libunwind - a platform-independent unwind library
   Copyright (C) 2001-2005 Hewlett-Packard Co
        Contributed by David Mosberger-Tang <davidm@hpl.hp.com>
   Copyright (C) 2013 Linaro Limited

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

#ifndef AARCH64_LIBUNWIND_I_H
#define AARCH64_LIBUNWIND_I_H

/* Target-dependent definitions that are internal to libunwind but need
   to be shared with target-independent code.  */

#include <stdlib.h>
#include <libunwind.h>
#include <stdatomic.h>

#include "elf64.h"
#include "mempool.h"
#include "dwarf.h"

typedef enum
  {
    UNW_AARCH64_FRAME_STANDARD = -2,     /* regular fp, sp +/- offset */
    UNW_AARCH64_FRAME_SIGRETURN = -1,    /* special sigreturn frame */
    UNW_AARCH64_FRAME_OTHER = 0,         /* not cacheable (special or unrecognised) */
    UNW_AARCH64_FRAME_GUESSED = 1        /* guessed it was regular, but not known */
  }
unw_tdep_frame_type_t;

typedef struct
  {
    uint64_t virtual_address;
    int64_t frame_type     : 2;  /* unw_tdep_frame_type_t classification */
    int64_t last_frame     : 1;  /* non-zero if last frame in chain */
    int64_t cfa_reg_sp     : 1;  /* cfa dwarf base register is sp vs. fp */
    int64_t cfa_reg_offset : 30; /* cfa is at this offset from base register value */
    int64_t fp_cfa_offset  : 30; /* fp saved at this offset from cfa (-1 = not saved) */
    int64_t lr_cfa_offset  : 30; /* lr saved at this offset from cfa (-1 = not saved) */
    int64_t sp_cfa_offset  : 30; /* sp saved at this offset from cfa (-1 = not saved) */
  }
unw_tdep_frame_t;

#ifdef UNW_LOCAL_ONLY

typedef unw_word_t aarch64_loc_t;

#else /* !UNW_LOCAL_ONLY */

typedef struct aarch64_loc
  {
    unw_word_t w0, w1;
  }
aarch64_loc_t;

#endif /* !UNW_LOCAL_ONLY */

struct unw_addr_space
  {
    struct unw_accessors acc;
    int big_endian;
#ifndef UNW_REMOTE_ONLY
    unw_iterate_phdr_func_t iterate_phdr_function;
#endif
    unw_caching_policy_t caching_policy;
    _Atomic uint32_t cache_generation;
    unw_word_t dyn_generation;          /* see dyn-common.h */
    unw_word_t dyn_info_list_addr;      /* (cached) dyn_info_list_addr */
    struct dwarf_rs_cache global_cache;
    struct unw_debug_frame_list *debug_frames;
   };

struct cursor
  {
    struct dwarf_cursor dwarf;          /* must be first */

    unw_tdep_frame_t frame_info;        /* quick tracing assist info */

    enum
      {
        AARCH64_SCF_NONE,
        AARCH64_SCF_LINUX_RT_SIGFRAME,
        AARCH64_SCF_FREEBSD_RT_SIGFRAME,
        AARCH64_SCF_QNX_RT_SIGFRAME,
      }
    sigcontext_format;
    unw_word_t sigcontext_addr;
    unw_word_t sigcontext_sp;
    unw_word_t sigcontext_pc;
    int validate;
    unw_context_t *uc;
  };

static inline unw_context_t *
dwarf_get_uc(const struct dwarf_cursor *cursor)
{
  const struct cursor *c = (struct cursor *) cursor->as_arg;
  return c->uc;
}

#define DWARF_GET_LOC(l)        ((l).val)

#ifdef UNW_LOCAL_ONLY
# define DWARF_NULL_LOC         DWARF_LOC (0, 0)
# define DWARF_IS_NULL_LOC(l)   (DWARF_GET_LOC (l) == 0)
# define DWARF_LOC(r, t)        ((dwarf_loc_t) { .val = (r) })
# define DWARF_IS_REG_LOC(l)    0
# define DWARF_REG_LOC(c,r)     (DWARF_LOC((unw_word_t)                      \
                                 tdep_uc_addr(dwarf_get_uc(c), (r)), 0))
# define DWARF_MEM_LOC(c,m)     DWARF_LOC ((m), 0)
# define DWARF_FPREG_LOC(c,r)   (DWARF_LOC((unw_word_t)                      \
                                 tdep_uc_addr(dwarf_get_uc(c), (r)), 0))

static inline int
dwarf_getfp (struct dwarf_cursor *c UNUSED, dwarf_loc_t loc, unw_fpreg_t *val)
{
  if (!DWARF_GET_LOC (loc))
    return -1;
  *val = *(unw_fpreg_t *) DWARF_GET_LOC (loc);
  return 0;
}

static inline int
dwarf_putfp (struct dwarf_cursor *c UNUSED, dwarf_loc_t loc, unw_fpreg_t val)
{
  if (!DWARF_GET_LOC (loc))
    return -1;
  *(unw_fpreg_t *) DWARF_GET_LOC (loc) = val;
  return 0;
}

static inline int
dwarf_get (struct dwarf_cursor *c UNUSED, dwarf_loc_t loc, unw_word_t *val)
{
  if (!DWARF_GET_LOC (loc))
    return -1;
  *val = *(unw_word_t *) DWARF_GET_LOC (loc);
  return 0;
}

static inline int
dwarf_put (struct dwarf_cursor *c UNUSED, dwarf_loc_t loc, unw_word_t val)
{
  if (!DWARF_GET_LOC (loc))
    return -1;
  *(unw_word_t *) DWARF_GET_LOC (loc) = val;
  return 0;
}

#else /* !UNW_LOCAL_ONLY */
# define DWARF_LOC_TYPE_FP      (1 << 0)
# define DWARF_LOC_TYPE_REG     (1 << 1)
# define DWARF_NULL_LOC         DWARF_LOC (0, 0)

static inline int
dwarf_is_null_loc(dwarf_loc_t l)
{
  return l.val == 0 && l.type == 0;
}

# define DWARF_IS_NULL_LOC(l)   dwarf_is_null_loc(l)
# define DWARF_LOC(r, t)        ((dwarf_loc_t) { .val = (r), .type = (t) })
# define DWARF_IS_REG_LOC(l)    (((l).type & DWARF_LOC_TYPE_REG) != 0)
# define DWARF_IS_FP_LOC(l)     (((l).type & DWARF_LOC_TYPE_FP) != 0)
# define DWARF_REG_LOC(c,r)     DWARF_LOC((r), DWARF_LOC_TYPE_REG)
# define DWARF_MEM_LOC(c,m)     DWARF_LOC ((m), 0)
# define DWARF_FPREG_LOC(c,r)   DWARF_LOC((r), (DWARF_LOC_TYPE_REG      \
                                                | DWARF_LOC_TYPE_FP))

static inline int
dwarf_getfp (struct dwarf_cursor *c, dwarf_loc_t loc, unw_fpreg_t *val)
{
  char *valp = (char *) &val;
  unw_word_t addr;
  int ret;

  if (DWARF_IS_NULL_LOC (loc))
    return -UNW_EBADREG;

  if (DWARF_IS_REG_LOC (loc))
    return (*c->as->acc.access_fpreg) (c->as, DWARF_GET_REG_LOC (loc),
                                       val, 0, c->as_arg);

  addr = DWARF_GET_MEM_LOC (loc);
  if ((ret = (*c->as->acc.access_mem) (c->as, addr + 0, (unw_word_t *) valp,
                                       0, c->as_arg)) < 0)
    return ret;

  return (*c->as->acc.access_mem) (c->as, addr + 4, (unw_word_t *) valp + 1, 0,
                                   c->as_arg);
}

static inline int
dwarf_putfp (struct dwarf_cursor *c, dwarf_loc_t loc, unw_fpreg_t val)
{
  char *valp = (char *) &val;
  unw_word_t addr;
  int ret;

  if (DWARF_IS_NULL_LOC (loc))
    return -UNW_EBADREG;

  if (DWARF_IS_REG_LOC (loc))
    return (*c->as->acc.access_fpreg) (c->as, DWARF_GET_REG_LOC (loc),
                                       &val, 1, c->as_arg);

  addr = DWARF_GET_MEM_LOC (loc);
  if ((ret = (*c->as->acc.access_mem) (c->as, addr + 0, (unw_word_t *) valp,
                                       1, c->as_arg)) < 0)
    return ret;

  return (*c->as->acc.access_mem) (c->as, addr + 4, (unw_word_t *) valp + 1,
                                   1, c->as_arg);
}

static inline int
dwarf_get (struct dwarf_cursor *c, dwarf_loc_t loc, unw_word_t *val)
{
  if (DWARF_IS_NULL_LOC (loc))
    return -UNW_EBADREG;

  /* If a code-generator were to save a value of type unw_word_t in a
     floating-point register, we would have to support this case.  I
     suppose it could happen with MMX registers, but does it really
     happen?  */
  assert (!DWARF_IS_FP_LOC (loc));

  if (DWARF_IS_REG_LOC (loc))
    return (*c->as->acc.access_reg) (c->as, DWARF_GET_REG_LOC (loc), val,
                                     0, c->as_arg);
  else
    return (*c->as->acc.access_mem) (c->as, DWARF_GET_MEM_LOC (loc), val,
                                     0, c->as_arg);
}

static inline int
dwarf_put (struct dwarf_cursor *c, dwarf_loc_t loc, unw_word_t val)
{
  if (DWARF_IS_NULL_LOC (loc))
    return -UNW_EBADREG;

  /* If a code-generator were to save a value of type unw_word_t in a
     floating-point register, we would have to support this case.  I
     suppose it could happen with MMX registers, but does it really
     happen?  */
  assert (!DWARF_IS_FP_LOC (loc));

  if (DWARF_IS_REG_LOC (loc))
    return (*c->as->acc.access_reg) (c->as, DWARF_GET_REG_LOC (loc), &val,
                                     1, c->as_arg);
  else
    return (*c->as->acc.access_mem) (c->as, DWARF_GET_MEM_LOC (loc), &val,
                                     1, c->as_arg);
}

#endif /* !UNW_LOCAL_ONLY */



#define tdep_getcontext_trace           UNW_ARCH_OBJ(getcontext_trace)
#define tdep_init_done                  UNW_OBJ(init_done)
#define tdep_init                       UNW_OBJ(init)
/* Platforms that support UNW_INFO_FORMAT_TABLE need to define
   tdep_search_unwind_table.  */
#define tdep_search_unwind_table        dwarf_search_unwind_table
#define tdep_find_unwind_table          dwarf_find_unwind_table
#define tdep_uc_addr                    UNW_OBJ(uc_addr)
#define tdep_get_elf_image              UNW_ARCH_OBJ(get_elf_image)
#define tdep_get_exe_image_path         UNW_ARCH_OBJ(get_exe_image_path)
#define tdep_access_reg                 UNW_OBJ(access_reg)
#define tdep_access_fpreg               UNW_OBJ(access_fpreg)
#define tdep_fetch_frame(c,ip,n)        do {} while(0)
#define tdep_cache_frame(c)             0
#define tdep_reuse_frame(c,frame)       do {} while(0)
#define tdep_stash_frame                UNW_OBJ(tdep_stash_frame)
#define tdep_trace                      UNW_OBJ(tdep_trace)
#define tdep_strip_ptrauth_insn_mask    UNW_OBJ(tdep_strip_ptrauth_insn_mask)

#ifdef UNW_LOCAL_ONLY
# define tdep_find_proc_info(c,ip,n)                            \
        dwarf_find_proc_info((c)->as, (ip), &(c)->pi, (n),      \
                                       (c)->as_arg)
# define tdep_put_unwind_info(as,pi,arg)                \
        dwarf_put_unwind_info((as), (pi), (arg))
#else
# define tdep_find_proc_info(c,ip,n)                                    \
        (*(c)->as->acc.find_proc_info)((c)->as, (ip), &(c)->pi, (n),    \
                                       (c)->as_arg)
# define tdep_put_unwind_info(as,pi,arg)                \
        (*(as)->acc.put_unwind_info)((as), (pi), (arg))
#endif

#define tdep_get_as(c)                  ((c)->dwarf.as)
#define tdep_get_as_arg(c)              ((c)->dwarf.as_arg)
#define tdep_get_ip(c)                  ((c)->dwarf.ip)
#define tdep_big_endian(as)             ((as)->big_endian)

extern atomic_bool tdep_init_done;

extern void tdep_init (void);
extern int tdep_search_unwind_table (unw_addr_space_t as, unw_word_t ip,
                                     unw_dyn_info_t *di, unw_proc_info_t *pi,
                                     int need_unwind_info, void *arg);
extern void *tdep_uc_addr (unw_context_t *uc, int reg);
extern int tdep_get_elf_image (struct elf_image *ei, pid_t pid, unw_word_t ip,
                               unsigned long *segbase, unsigned long *mapoff,
                               char *path, size_t pathlen);
extern void tdep_get_exe_image_path (char *path);
extern int tdep_access_reg (struct cursor *c, unw_regnum_t reg,
                            unw_word_t *valp, int write);
extern int tdep_access_fpreg (struct cursor *c, unw_regnum_t reg,
                              unw_fpreg_t *valp, int write);
extern int tdep_trace (unw_cursor_t *cursor, void **addresses, int *n);
extern void tdep_stash_frame (struct dwarf_cursor *c,
                              struct dwarf_reg_state *rs);
extern int tdep_getcontext_trace (unw_context_t *);
extern unw_word_t tdep_strip_ptrauth_insn_mask (unw_cursor_t *cursor, unw_word_t ip);

typedef enum frame_record_location
  {
    NONE,           /* frame record creation has not been detected, use LR */
    AT_SP_OFFSET,   /* frame record creation has been detected, but FP
                       update not detected */
    AT_FP,          /* frame record creation and FP update detected */
  } frame_record_location_t;

typedef struct frame_state
  {
    frame_record_location_t loc;
    int32_t offset;
  } frame_state_t;

#define get_frame_state UNW_OBJ(get_frame_state)
HIDDEN frame_state_t get_frame_state (unw_cursor_t *cursor);

#endif /* AARCH64_LIBUNWIND_I_H */
