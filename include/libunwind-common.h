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

#define UNW_PASTE2(x,y)	x##y
#define UNW_PASTE(x,y)	UNW_PASTE2(x,y)
#define UNW_OBJ(fn)	UNW_PASTE(UNW_PREFIX, fn)
#define UNW_ARCH_OBJ(fn) UNW_PASTE(UNW_PASTE(UNW_PASTE(_U,UNW_TARGET),_), fn)

#ifdef UNW_LOCAL_ONLY
# define UNW_PREFIX	UNW_PASTE(UNW_PASTE(_UL,UNW_TARGET),_)
#else /* !UNW_LOCAL_ONLY */
# define UNW_PREFIX	UNW_PASTE(UNW_PASTE(_U,UNW_TARGET),_)
#endif /* !UNW_LOCAL_ONLY */

typedef unw_tdep_word_t unw_word_t;

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
    UNW_EINVAL,			/* unsupported operation or bad value */
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
    UNW_REG_EH = UNW_TDEP_EH,		/* (rw) exception-handling reg base */
    UNW_REG_LAST = UNW_TDEP_LAST_REG
  }
unw_frame_regnum_t;

/* Number of exception-handler argument registers: */
#define UNW_NUM_EH_REGS		UNW_TDEP_NUM_EH_REGS

typedef enum
  {
    UNW_CACHE_NONE,			/* no caching */
    UNW_CACHE_GLOBAL,			/* shared global cache */
    UNW_CACHE_PER_THREAD		/* per-thread caching */
  }
unw_caching_policy_t;

typedef int unw_regnum_t;

/* The unwind cursor starts at the youngest (most deeply nested) frame
   and is used to track the frame state as the unwinder steps from
   frame to frame.  It is safe to make (shallow) copies of variables
   of this type.  */
typedef struct unw_cursor
  {
    unw_word_t opaque[UNW_TDEP_CURSOR_LEN];
  }
unw_cursor_t;

/* This type encapsulates the entire (preserved) machine-state.  */
typedef unw_tdep_context_t unw_context_t;

/* unw_getcontext() fills the unw_context_t pointed to by UC with the
   machine state as it exists at the call-site.  For implementation
   reasons, this needs to be a target-dependent macro.  It's easiest
   to think of unw_getcontext() as being identical to getcontext(). */
#define unw_getcontext(uc)	unw_tdep_getcontext(uc)

typedef unw_tdep_fpreg_t unw_fpreg_t;

typedef struct unw_addr_space *unw_addr_space_t;

/* This bit is set to indicate */
#define UNW_PI_FLAG_FIRST_TDEP_BIT	16

typedef struct unw_proc_info
  {
    unw_word_t start_ip;	/* first IP covered by this procedure */
    unw_word_t end_ip;		/* first IP NOT covered by this procedure */
    unw_word_t lsda;		/* address of lang.-spec. data area (if any) */
    unw_word_t handler;		/* optional personality routine */
    unw_word_t gp;		/* global-pointer value for this procedure */
    unw_word_t flags;		/* misc. flags */

    int format;			/* unwind-info format (arch-specific) */
    int unwind_info_size;	/* size of the informat (if applicable) */
    void *unwind_info;		/* unwind-info (arch-specific) */
  }
unw_proc_info_t;

/* These are backend callback routines that provide access to the
   state of a "remote" process.  This can be used, for example, to
   unwind another process through the ptrace() interface.  */
typedef struct unw_accessors
  {
    /* Look up the unwind info associated with instruction-pointer IP.
       On success, the routine fills in the PROC_INFO structure.  */
    int (*find_proc_info) (unw_addr_space_t as, unw_word_t ip,
			   unw_proc_info_t *proc_info,
			   int need_unwind_info,
			   void *arg);

    /* Release any resources (e.g., memory) that were allocated for
       the unwind info returned in by a previous call to
       find_proc_info() with NEED_UNWIND_INFO set to 1.  */
    void (*put_unwind_info) (unw_addr_space_t as, unw_proc_info_t *proc_info,
			     void *arg);

    /* Return the list-head of the dynamically registered unwind
       info.  */
    int (*get_dyn_info_list_addr) (unw_addr_space_t as,
				   unw_word_t *dyn_info_list_addr,
				   void *arg);

    /* Access aligned word at address ADDR.  The value is returned
       according to the endianness of the host (e.g., if the host is
       little-endian and the target is big-endian, access_mem() needs
       to byte-swap the value before returning it).  */
    int (*access_mem) (unw_addr_space_t as, unw_word_t addr,
		       unw_word_t *val, int write, void *arg);

    /* Access register number REG at address ADDR.  */
    int (*access_reg) (unw_addr_space_t as, unw_regnum_t reg,
		       unw_word_t *val, int write, void *arg);

    /* Access register number REG at address ADDR.  */
    int (*access_fpreg) (unw_addr_space_t as, unw_regnum_t reg,
			 unw_fpreg_t *val, int write, void *arg);

    int (*resume) (unw_addr_space_t as, unw_cursor_t *c, void *arg);

    /* Optional call back to obtain the name of a (static) procedure.
       Dynamically generated procedures are handled automatically by
       libunwind.  This callback is optional and may be set to
       NULL.  */
    int (*get_proc_name) (unw_addr_space_t as, unw_word_t addr,
			  char *buf, size_t buf_len, unw_word_t *offp,
			  void *arg);
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

#include <libunwind-dynamic.h>

/* These routines work both for local and remote unwinding.  */

extern unw_addr_space_t UNW_OBJ(local_addr_space);

extern unw_addr_space_t UNW_OBJ(create_addr_space) (unw_accessors_t *a,
						    int byte_order);
extern void UNW_OBJ(destroy_addr_space) (unw_addr_space_t as);
extern unw_accessors_t *UNW_ARCH_OBJ(get_accessors) (unw_addr_space_t as);
extern void UNW_ARCH_OBJ(flush_cache)(unw_addr_space_t as,
				      unw_word_t lo, unw_word_t hi);
extern int UNW_ARCH_OBJ(set_caching_policy)(unw_addr_space_t as,
					    unw_caching_policy_t policy);
extern const char *UNW_ARCH_OBJ(regname) (unw_regnum_t regnum);

extern int UNW_OBJ(init_local) (unw_cursor_t *c, unw_context_t *u);
extern int UNW_OBJ(init_remote) (unw_cursor_t *c, unw_addr_space_t as,
				 void *as_arg);
extern int UNW_OBJ(step) (unw_cursor_t *c);
extern int UNW_OBJ(resume) (unw_cursor_t *c);
extern int UNW_OBJ(get_proc_info) (unw_cursor_t *c, unw_proc_info_t *pi);
extern int UNW_OBJ(get_reg) (unw_cursor_t *c, int regnum, unw_word_t *valp);
extern int UNW_OBJ(set_reg) (unw_cursor_t *c, int regnum, unw_word_t val);
extern int UNW_OBJ(get_fpreg) (unw_cursor_t *c, int regnum, unw_fpreg_t *val);
extern int UNW_OBJ(set_fpreg) (unw_cursor_t *c, int regnum, unw_fpreg_t val);
extern int UNW_OBJ(get_save_loc) (unw_cursor_t *c, int regnum,
				  unw_save_loc_t *loc);
extern int UNW_OBJ(is_signal_frame) (unw_cursor_t *c);
extern int UNW_OBJ(get_proc_name) (unw_cursor_t *c, char *buf, size_t buf_len,
				   unw_word_t *offsetp);

#define unw_local_addr_space		UNW_OBJ(local_addr_space)

/* Create a new address space (in addition to the default
   local_addr_space).  BYTE_ORDER can be 0 to select the default
   byte-order or one of the byte-order values defined by <endian.h>
   (e.g., __LITTLE_ENDIAN or __BIG_ENDIAN).  The default byte-order is
   either implied by the target architecture (e.g., x86 is always
   little-endian) or is select based on the byte-order of the host.

   This routine is NOT signal-safe.  */
#define unw_create_addr_space(a,b)	UNW_OBJ(create_addr_space)(a,b)

/* Destroy an address space.
   This routine is NOT signal-safe.  */
#define unw_destroy_addr_space(as)	UNW_OBJ(destroy_addr_space)(as)

/* Retrieve a pointer to the accessors structure associated with
   address space AS.
   This routine is signal-safe. */
#define unw_get_accessors(as)		UNW_ARCH_OBJ(get_accessors)(as)

/* Initialize cursor C such that unwinding starts at the point
   represented by the context U.  Returns zero on success, negative
   value on failure.
   This routine is signal-safe.  */
#define unw_init_local(c,u)		UNW_OBJ(init_local)(c, u)

/* Initialize cursor C such that it accesses the unwind target through
   accessors A.
   This routine is signal-safe.  */
#define unw_init_remote(c,a,arg)	UNW_OBJ(init_remote)(c, a, arg)

/* Move cursor up by one step (up meaning toward earlier, less deeply
   nested frames).  Returns positive number if there are more frames
   to unwind, 0 if last frame has been reached, negative number in
   case of an error.
   This routine is signal-safe.  */
#define unw_step(c)			UNW_OBJ(step)(c)

/* Resume execution at the point identified by the cursor.
   This routine is signal-safe.  */
#define unw_resume(c)			UNW_OBJ(resume)(c)

/* Return the proc-info associated with the cursor.
   This routine is signal-safe.  */
#define unw_get_proc_info(c,p)		UNW_OBJ(get_proc_info)(c,p)

/* Register accessor routines.  Return zero on success, negative value
   on failure.
   These routines are signal-safe.  */
#define unw_get_reg(c,r,v)		UNW_OBJ(get_reg)(c,r,v)
#define unw_set_reg(c,r,v)		UNW_OBJ(set_reg)(c,r,v)

/* Floating-point accessor routines.  Return zero on success, negative
   value on failure.
   These routines are signal-safe.  */
#define unw_get_fpreg(c,r,v)		UNW_OBJ(get_fpreg)(c,r,v)
#define unw_set_fpreg(c,r,v)		UNW_OBJ(set_fpreg)(c,r,v)

/* Get the save-location of register R.
   This routine is signal-safe.  */
#define unw_get_save_loc(c,r,l)		UNW_OBJ(get_save_loc)(c,r,l)

/* Return 1 if register number R is a floating-point register, zero
   otherwise.
   This routine is signal-safe.  */
#define unw_is_fpreg(r)			unw_tdep_is_fpreg(r)

/* Returns non-zero value if the cursor points to a signal frame.
   This routine is signal-safe.  */
#define unw_is_signal_frame(c)		UNW_OBJ(is_signal_frame)(c)

/* Return the name of the procedure that created the frame identified
   by the cursor.  The returned string is ASCII NUL terminated. If the
   string buffer is too small to store the entire name, the first
   portion of the string that can fit is stored in the buffer (along
   with a terminating NUL character) and -UNW_ENOMEM is returned.  If
   no name can be determined, -UNW_ENOINFO is returned.
   This routine is NOT signal-safe.  */
#define unw_get_proc_name(c,s,l,o)	UNW_OBJ(get_proc_name)(c, s, l, o)

/* Returns the canonical register name of register R.  R must be in
   the range from 0 to UNW_REG_LAST.  Like all other unwind routines,
   this one is re-entrant (i.e., the returned string must be a string
   constant.
   This routine is signal-safe.  */
#define unw_regname(r)			UNW_ARCH_OBJ(regname)(r)

/* Sets the caching policy of address space AS.  Caching can be
   disabled completely by setting the policy to UNW_CACHE_NONE.  With
   UNW_CACHE_GLOBAL, there is a single cache that is shared across all
   threads.  With UNW_CACHE_PER_THREAD, each thread gets its own
   cache, which can improve performance thanks to less locking and
   better locality.  By default, UNW_CACHE_GLOBAL is in effect.
   This routine is NOT signal-safe.  */
#define unw_set_caching_policy(as, p)	UNW_ARCH_OBJ(set_caching_policy)(as, p)

/* Flush all caches (global, per-thread, or any other caches that
   might exist) in address-space AS of information at least relating
   to the address-range LO to HI (non-inclusive).  LO and HI are only
   a performance hint and the function is allowed to over-flush (i.e.,
   flush more than the requested address-range).  Furthermore, if LO
   and HI are both 0, the entire address-range is flushed.  This
   function must be called if any of unwind information might have
   changed (e.g., because a library might have been removed via a call
   to dlclose()).
   This routine is signal-safe.  */
#define unw_flush_cache(as,lo,hi)	UNW_ARCH_OBJ(flush_cache)(as, lo, hi)

/* Helper routines which make it easy to use libunwind via ptrace().
   They're available only if UNW_REMOTE is _not_ defined and they
   aren't really part of the libunwind API.  They are simple enough
   not to warrant creating a separate library for them.  */

extern void *_UPT_create (pid_t);
extern void _UPT_destroy (void *upt);
extern int _UPT_find_proc_info (unw_addr_space_t as, unw_word_t ip,
				unw_proc_info_t *pi, int need_unwind_info,
				void *arg);
extern void _UPT_put_unwind_info (unw_addr_space_t as, unw_proc_info_t *pi,
				  void *arg);
extern int _UPT_get_dyn_info_list_addr (unw_addr_space_t as,
					unw_word_t *dil_addr, void *arg);
extern int _UPT_access_mem (unw_addr_space_t as, unw_word_t addr,
			    unw_word_t *val, int write, void *arg);
extern int _UPT_access_reg (unw_addr_space_t as, unw_regnum_t reg,
			    unw_word_t *val, int write, void *arg);
extern int _UPT_access_fpreg (unw_addr_space_t as, unw_regnum_t reg,
			      unw_fpreg_t *val, int write, void *arg);
extern int _UPT_get_proc_name (unw_addr_space_t as, unw_word_t addr,
			       char *buf, size_t len, unw_word_t *offp,
			       void *arg);
extern int _UPT_resume (unw_addr_space_t as, unw_cursor_t *c, void *arg);
extern unw_accessors_t _UPT_accessors;
