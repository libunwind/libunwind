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

#ifndef LIBUNWIND_DYNAMIC_H
#define LIBUNWIND_DYNAMIC_H

/* This file defines the runtime-support routines for dynamically
   generated code.  Even though it is implemented as part of
   libunwind, it is logically separate from the interface to perform
   the actual unwinding.  In particular, this interface is always used
   in the context of the unwind target, whereas the rest of the unwind
   API is used in context of the process that is doing the unwind
   (which may be a debugger running on another machine, for
   example).  */

typedef enum
  {
    UNW_OP_SAVE_REG,		/* save register to another register */
    UNW_OP_SPILL_FP_REL,	/* frame-pointer-relative register spill */
    UNW_OP_SPILL_SP_REL,	/* stack-pointer-relative register spill */
    UNW_OP_ADD,			/* add constant value to a register */
    UNW_OP_POP_STACK,		/* drop one or more stack frames */
    UNW_OP_LABEL_STATE,		/* name the current state */
    UNW_OP_COPY_STATE,		/* set the region's entry-state */
    UNW_OP_ALIAS,		/* get unwind info from an alias */
    UNW_OP_STOP			/* end-of-unwind-info marker */
  }
unw_operator_t;

typedef struct unw_proc_info
  {
    unsigned long private[4];	/* reserved for use by libunwind */
    void *proc_start;		/* first text-address of procedure */
    void *proc_end;		/* first text-address beyond the procedure */
    unsigned long flags;
    const char *proc_name;	/* unique & human-readable procedure name */
    void *creator_hook;		/* hook for whoever created this procedure */
    unsigned long reserved[7];	/* reserved for future extensions */
  }
unw_proc_info_t;

typedef struct unw_region_info
  {
    struct unw_region_info *next;	/* NULL-terminated list of regions */
    void *creator_hook;
    unsigned int insn_count;		/* region length (# of instructions) */
    unsigned int op_count;		/* length of op-array */
    unsigned long reserved[5];
    struct unw_op_t
      {
	unsigned int tag : 16;		/* what operation? */
	unsigned int reg : 16;		/* what register */
	unsigned int when : 32;		/* when does it take effect? */
	unsigned long val;		/* auxiliary value */
      }
    op[1];
  }
unw_region_info_t;

/* Return the size (in bytes) of an unw_region_info_t structure that can
   hold OP_COUNT ops.  */
#define unw_region_info_size(op_count)					   \
	(sizeof (unw_region_info_t)					   \
	 + (op_count > 0) ? ((op_count) - 1) * sizeof (struct unw_op) : 0)

/* Register the unwind info for a single procedure.
   This routine is NOT signal-safe.  */
extern int unw_register_proc (unw_proc_info_t *proc);

/* Cancel the unwind info for a single procedure.
   This routine is NOT signal-safe.  */
extern int unw_cancel_proc (unw_proc_info_t *proc);


/* Convenience routines.  */

#define unw_op(tag, when, reg, val)		\
	(struct unw_op_t) {			\
	  .tag	= (tag),			\
	  .when	= (when),			\
	  .reg	= (reg),			\
	  .val	= (val)				\
	}

#define unw_op_save_reg(op, when, reg, dst)			\
	unw_op(UNW_OP_SAVE_REG, (when), (reg), (dst))

#define unw_op_spill_fp_rel(when, reg, offset)			\
	unw_op(UNW_OP_SPILL_FP_REL, (when), (reg), (offset))

#define unw_op_spill_sp_rel(when, reg, offset)			\
	unw_op(UNW_OP_SPILL_SP_REL, (when), (reg), (offset))

#define unw_op_add(when, reg, value)				\
	unw_op(UNW_OP_ADD, (when), (reg), (value))

#define unw_op_pop_stack(op, num_frames)			\
	unw_op(UNW_OP_POP_STACK, (when), 0, (num_frames))

#define unw_op_label_state(op, when, label)			\
	unw_op(UNW_OP_LABEL_STATE, (when), 0, (label))

#define unw_op_copy_state(op, when, label)			\
	unw_op(UNW_OP_COPY_STATE, (when), 0, (label))

#define unw_op_alias(op, when, delta)				\
	unw_op(UNW_OP_ALIAS, (when), 0, (label))

#endif /* LIBUNWIND_DYNAMIC_H */
