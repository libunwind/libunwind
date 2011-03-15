/* libunwind - a platform-independent unwind library
   Copyright 2011 Linaro Limited

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

#include "libunwind_i.h"

static int
arm_exidx_unwind_frame (struct arm_stackframe *frame)
{
  Debug (2, "frame: fp=%p, sp=%p, lr=%p, pc=%p\n",
      frame->fp, frame->sp, frame->lr, frame->pc);

  struct arm_exidx_table *table = arm_exidx_table_find (frame->pc);
  if (NULL == table)
    return -1;

  struct arm_exidx_entry *entry = arm_exidx_table_lookup (table, frame->pc);
  if (NULL == entry)
    return -1;

  struct arm_exidx_vrs s;
  arm_exidx_frame_to_vrs(frame, &s);

  uint8_t buf[32], n_buf = arm_exidx_extract (entry, buf);
  if (arm_exidx_decode (buf, n_buf, &arm_exidx_vrs_callback, &s) < 0)
    return -1;

  return arm_exidx_vrs_to_frame(&s, frame);
}

typedef void (*arm_exidx_backtrace_callback_t) (void *, struct arm_stackframe *);

static int
arm_exidx_backtrace (arm_exidx_backtrace_callback_t cb)
{
  register void *current_sp asm ("sp");
  struct arm_stackframe frame = {
    .fp = __builtin_frame_address (0),
    .sp = current_sp,
    .lr = __builtin_return_address (0),
    .pc = &arm_exidx_backtrace,
  };

  while (1)
    {
      void *where = frame.pc;
      if (arm_exidx_unwind_frame (&frame) < 0)
	break;
      (*cb) (where, &frame);
    }
  return 0;
}

static void
b (void *where, struct arm_stackframe *frame)
{
  Debug (2, "%p -> %p (%p)\n", where, frame->pc, frame->sp - 4);
}

static int
a (int foo)
{
  if (foo > 0)
    return a (foo - 1);
  arm_exidx_backtrace (&b);
  return foo;
}

int
main (int argc, char *argv[])
{
  mi_init ();
  arm_exidx_init_local (argv[0]);
  return a (argc);
}
