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

/* This file defines the runtime-support routines for dynamically
generated code.  Even though it is implemented as part of libunwind,
it is logically separate from the interface to perform the actual
unwinding.  In particular, this interface is always used in the
context of the unwind target, whereas the rest of the unwind API is
used in context of the process that is doing the unwind (which may be
a debugger running on another machine, for example).

Note that the data-structures declared here server a dual purpose:
when a program registers a dynamically generated procedure, it uses
these structures directly.  On the other hand, with remote-unwinding,
the data-structures are read from the remote process's memory and
translated into internalized versions.  Because of this, care needs to
be taken when choosing the types of structure members: use a pointer
only if the member can be translated into an internalized equivalent
(such as a string).  Similarly, for members that need to hold an
address in the unwindee, unw_word_t needs to be used.  */

typedef enum
  {
    UNW_DYN_STOP = 0,		/* end-of-unwind-info marker */
    UNW_DYN_SAVE_REG,		/* save register to another register */
    UNW_DYN_SPILL_FP_REL,	/* frame-pointer-relative register spill */
    UNW_DYN_SPILL_SP_REL,	/* stack-pointer-relative register spill */
    UNW_DYN_ADD,		/* add constant value to a register */
    UNW_DYN_POP_FRAMES,		/* drop one or more stack frames */
    UNW_DYN_LABEL_STATE,	/* name the current state */
    UNW_DYN_COPY_STATE,		/* set the region's entry-state */
    UNW_DYN_ALIAS		/* get unwind info from an alias */
  }
unw_dyn_operation_t;

typedef struct unw_dyn_op
  {
    uint16_t tag;			/* what operation? */
    int16_t reg;			/* what register */
    int16_t qp;				/* qualifying predicate register */
    int16_t pad0;
    int32_t when;			/* when does it take effect? */
    unw_word_t val;			/* auxiliary value */
  }
unw_dyn_op_t;

typedef struct unw_dyn_region_info
  {
    struct unw_dyn_region_info *next;	/* linked list of regions */
    uint32_t insn_count;		/* region length (# of instructions) */
    uint32_t op_count;			/* length of op-array */
    unw_dyn_op_t op[1];			/* variable-length op-array */
  }
unw_dyn_region_info_t;

typedef struct unw_dyn_proc_info
  {
    const char *name;		/* unique & human-readable procedure name */
    unw_word_t handler;		/* address of personality routine */
    uint32_t flags;
    unw_dyn_region_info_t *regions;
  }
unw_dyn_proc_info_t;

typedef struct unw_dyn_table_info
  {
    const char *name;		/* table name (e.g., name of library) */
    unw_word_t segbase;		/* segment base */
    unw_word_t table_size;
    void *table_data;
  }
unw_dyn_table_info_t;

typedef struct unw_dyn_info
  {
    struct unw_dyn_info *next;	/* linked list of dyn-info structures */
    unw_word_t start_ip;	/* first IP covered by this entry */
    unw_word_t end_ip;		/* first IP NOT covered by this entry */
    unw_word_t gp;		/* global-pointer in effect for this entry */
    enum
      {
	UNW_INFO_FORMAT_DYNAMIC,	/* unw_dyn_proc_info_t */
	UNW_INFO_FORMAT_TABLE		/* unw_dyn_table_t */
      }
    format;
    union
      {
	unw_dyn_proc_info_t pi;
	unw_dyn_table_info_t ti;
      }
    u;
  }
unw_dyn_info_t;

typedef struct unw_dyn_info_list
  {
    unsigned long generation;
    unw_dyn_info_t *first;
  }
unw_dyn_info_list_t;

/* Return the size (in bytes) of an unw_dyn_region_info_t structure that can
   hold OP_COUNT ops.  */
#define _U_dyn_region_info_size(op_count)				   \
	(sizeof (unw_dyn_region_info_t)					   \
	 + (op_count > 0) ? ((op_count) - 1) * sizeof (unw_dyn_op_t) : 0)

/* Register the unwind info for a single procedure.
   This routine is NOT signal-safe.  */
extern int _U_dyn_register (unw_dyn_info_t *di);

/* Cancel the unwind info for a single procedure.
   This routine is NOT signal-safe.  */
extern int _U_dyn_cancel (unw_dyn_info_t *di);


/* Convenience routines.  */

#define _U_dyn_op(_t, _q, _w, _r, _v)		\
	((unw_dyn_op_t) {			\
	  .tag	= (_t),				\
	  .qp	= (_q),				\
	  .when	= (_w),				\
	  .reg	= (_r),				\
	  .val	= (_v)				\
	})

#define _U_dyn_op_save_reg(op, qp, when, reg, dst)			\
	(*(op) = _U_dyn_op (UNW_DYN_SAVE_REG, (qp), (when), (reg), (dst)))

#define _U_dyn_op_spill_fp_rel(op, qp, when, reg, offset)		\
	(*(op) = _U_dyn_op (UNW_DYN_SPILL_FP_REL, (qp), (when), (reg),	\
			    (offset)))

#define _U_dyn_op_spill_sp_rel(op, qp, when, reg, offset)		\
	(*(op) = _U_dyn_op (UNW_DYN_SPILL_SP_REL, (qp), (when), (reg),	\
			    (offset)))

#define _U_dyn_op_add(op, qp, when, reg, value)				\
	(*(op) = _U_dyn_op (UNW_DYN_ADD, (qp), (when), (reg), (value)))

#define _U_dyn_op_pop_frames(op, qp, when, num_frames)			\
	(*(op) = _U_dyn_op (UNW_DYN_POP_frames, (qp), (when), 0, (num_frames)))

#define _U_dyn_op_label_state(op, qp, when, label)			\
	(*(op) = _U_dyn_op (UNW_DYN_LABEL_STATE, (qp), (when), 0, (label)))

#define _U_dyn_op_copy_state(op, qp, when, label)			\
	(*(op) = _U_dyn_op (UNW_DYN_COPY_STATE, (qp), (when), 0, (label)))

#define _U_dyn_op_alias(op, qp, when, addr)				\
	(*(op) = _U_dyn_op (UNW_DYN_ALIAS, (qp), (when), 0, (addr)))

#define _U_dyn_op_stop(op)						\
	((op)->tag = UNW_DYN_STOP)
