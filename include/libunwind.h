/* libunwind - a platform-independent unwind library
   Copyright (C) 2001-2002 Hewlett-Packard Co
	Contributed by David Mosberger-Tang <davidm@hpl.hp.com>

This file is part of libunwind.

libunwind is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2, or (at your option)
any later version.

libunwind is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

As a special exception, if you link this library with other files to
produce an executable, this library does not by itself cause the
resulting executable to be covered by the GNU General Public License.
This exception does not however invalidate any other reasons why the
executable file might be covered by the GNU General Public
License.  */

#ifndef LIBUNWIND_H
#define LIBUNWIND_H

#include <stdint.h>

#ifdef UNW_LOCAL_ONLY
# define UNW_PREFIX	_Ul_
#else /* !UNW_LOCAL_ONLY */
# define UNW_PREFIX	_U_
#endif /* !UNW_LOCAL_ONLY */

#define UNW_PASTE2(x,y)	x##y
#define UNW_PASTE(x,y)	UNW_PASTE2(x,y)
#define UNW_OBJ(fn)	UNW_PASTE(UNW_PREFIX, fn)

#include "libunwind-tdep.h"

typedef unw_tdep_word_t unw_word_t;

/* This needs to be big enough to accommodate the unwind state of any
   architecture, while leaving some slack for future expansion.
   Changing this value will require recompiling all users of this
   library.  */
#define UNW_STATE_LEN	127

/* Error codes.  The unwind routines return the *negated* values of
   these error codes on error and a non-negative value on success.  */
typedef enum
  {
    UNW_ESUCCESS = 0,		/* no error */
    UNW_EUNSPEC,		/* unspecified (general) error */
    UNW_ENOMEM,			/* out of memory */
    UNW_EBADREG,		/* bad register number */
    UNW_EREADONLYREG,		/* attempt to write read-only register */
    UNW_ESTOPUNWIND,		/* stop unwinding */
    UNW_EINVALIDIP,		/* invalid IP */
    UNW_EBADFRAME,		/* bad frame */
    UNW_EINVAL,			/* unsupported operation */
    UNW_EBADVERSION,		/* unwind info has unsupported version */
    UNW_ENOINFO			/* no unwind info found */
  }
unw_error_t;

/* The following enum defines the indices for a couple of
   (pseudo-)registers which have the same meaning across all
   platforms.  (RO) means read-only.  (RW) means read-write.  General
   registers (aka "integer registers") are expected to start with
   index 0.  The number of such registers is architecture-dependent.
   The remaining indices can be used as an architecture sees fit.  The
   last valid register index is given by UNW_REG_LAST.  */
typedef enum
  {
    UNW_REG_IP = UNW_TDEP_IP,		/* (rw) instruction pointer (pc) */
    UNW_REG_SP = UNW_TDEP_SP,		/* (ro) stack pointer */
    UNW_REG_PROC_START = UNW_TDEP_PROC_START,	/* (ro) proc startaddr */
    UNW_REG_HANDLER = UNW_TDEP_HANDLER,	/* (ro) addr. of personality routine */
    UNW_REG_LSDA = UNW_TDEP_LSDA,	/* (ro) addr. of lang.-specific data */
    UNW_REG_LAST = UNW_TDEP_LAST_REG
  }
unw_frame_regnum_t;

typedef int unw_regnum_t;

/* The unwind cursor starts at the youngest (most deeply nested) frame
   and is used to track the frame state as the unwinder steps from
   frame to frame.  It is safe to make (shallow) copies of variables
   of this type.  */
typedef struct unw_cursor
  {
    unw_word_t opaque[UNW_STATE_LEN];
  }
unw_cursor_t;

/* This type encapsulates the entire (preserved) machine-state.  */
typedef unw_tdep_context_t unw_context_t;

/* unw_getcontext() fills the unw_context_t pointed to by UC with the
   machine state as it exists at the call-site.  For implementation
   reasons, this needs to be a target-dependent macro.  It's easiest
   to think of unw_getcontext() as being identical to getcontext(). */
#define unw_getcontext(uc)	unw_tdep_getcontext(uc)

/* We will assume that "long double" is sufficiently large and aligned
   to hold the contents of a floating-point register.  Note that the
   fp register format is not usually the same format as a "long
   double".  Instead, the content of unw_fpreg_t should be manipulated
   only through the "raw.bits" member. */
typedef union
  {
    struct { unw_word_t bits[1]; } raw;
    long double dummy;	/* dummy to force 16-byte alignment */
  }
unw_fpreg_t;

/* These are backend callback routines that provide access to the
   state of a "remote" process.  This can be used, for example, to
   unwind another process through the ptrace() interface.  */
typedef struct unw_accessors
  {
    /* Lock for unwind info for address IP.  The architecture specific
       UNWIND_INFO is updated as necessary.  */
    int (*acquire_unwind_info) (unw_word_t ip, void *unwind_info, void *arg);
    int (*release_unwind_info) (void *unwind_info, void *arg);

    /* Access aligned word at address ADDR.  */
    int (*access_mem) (unw_word_t addr, unw_word_t *val, int write, void *arg);

    /* Access register number REG at address ADDR.  */
    int (*access_reg) (unw_regnum_t reg, unw_word_t *val, int write,
		       void *arg);

    /* Access register number REG at address ADDR.  */
    int (*access_fpreg) (unw_regnum_t reg, unw_fpreg_t *val, int write,
			 void *arg);

    int (*resume) (unw_cursor_t *c, void *arg);

    void *arg;		/* application-specific data */
  }
unw_accessors_t;

typedef enum unw_save_loc_type
  {
    UNW_SLT_NONE,	/* register is not saved ("not an l-value") */
    UNW_SLT_MEMORY,	/* register has been saved in memory */
    UNW_SLT_REG		/* register has been saved in (another) register */
  }
unw_save_loc_type_t;

typedef struct unw_save_loc
  {
    unw_save_loc_type_t type;
    union
      {
	unw_word_t addr;	/* valid if type==UNW_SLT_MEMORY */
	unw_regnum_t regnum;	/* valid if type==UNW_SLT_REG */
      }
    u;
    unw_tdep_save_loc_t extra;	/* target-dependent additional information */
  }
unw_save_loc_t;

/* These routines work both for local and remote unwinding.  */

extern int UNW_OBJ(init_local) (unw_cursor_t *c, ucontext_t *u);
extern int UNW_OBJ(init_remote) (unw_cursor_t *c, unw_accessors_t *a);
extern int UNW_OBJ(step) (unw_cursor_t *c);
extern int UNW_OBJ(resume) (unw_cursor_t *c);
extern int UNW_OBJ(get_reg) (unw_cursor_t *c, int regnum, unw_word_t *valp);
extern int UNW_OBJ(set_reg) (unw_cursor_t *c, int regnum, unw_word_t val);
extern int UNW_OBJ(get_fpreg) (unw_cursor_t *c, int regnum, unw_fpreg_t *val);
extern int UNW_OBJ(set_fpreg) (unw_cursor_t *c, int regnum, unw_fpreg_t val);
extern int UNW_OBJ(get_save_loc) (unw_cursor_t *c, int regnum,
				  unw_save_loc_t *loc);
extern int UNW_OBJ(is_signal_frame) (unw_cursor_t *c);

/* Initialize cursor C such that unwinding starts at the point
   represented by the context U.  Returns zero on success, negative
   value on failure.  */
#define unw_init_local(c,u)	UNW_OBJ(init_local)(c, u)

/* Initialize cursor C such that it accesses the unwind target through
   accessors A.  */
#define unw_init_remote(c,a)	UNW_OBJ(init_remote)(c, a)

/* Move cursor up by one step (up meaning toward earlier, less deeply
   nested frames).  Returns positive number if there are more frames
   to unwind, 0 if last frame has been reached, negative number in
   case of an error.  */
#define unw_step(c)		UNW_OBJ(step)(c)

/* Resume execution at the point identified by the cursor.  */
#define unw_resume(c)		UNW_OBJ(resume)(c)

/* Register accessor routines.  Return zero on success, negative value
   on failure.  */
#define unw_get_reg(c,r,v)	UNW_OBJ(get_reg)(c,r,v)
#define unw_set_reg(c,r,v)	UNW_OBJ(set_reg)(c,r,v)

/* Floating-point accessor routines.  Return zero on success, negative
   value on failure.  */
#define unw_get_fpreg(c,r,v)	UNW_OBJ(get_fpreg)(c,r,v)
#define unw_set_fpreg(c,r,v)	UNW_OBJ(set_fpreg)(c,r,v)

#define unw_get_save_loc(c,r,l)	UNW_OBJ(get_save_loc)(c,r,l)

/* Return 1 if register number R is a floating-point register, zero
   otherwise.  */
#define unw_is_fpreg(r)		unw_tdep_is_fpreg(r)

/* Returns non-zero value if the cursor points to a signal frame.  */
#define unw_is_signal_frame(c)	UNW_OBJ(is_signal_frame)(c)

#endif /* LIBUNWIND_H */
