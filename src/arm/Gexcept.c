/* libunwind - a platform-independent unwind library
   ARM EHABI C++ exception handling support.

   Copyright (C) 2025 libunwind contributors

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

/*
 * ARM uses the EHABI (Exception Handling ABI) which differs from the
 * DWARF-based protocol implemented in src/unwind/:
 *
 *  - Personality functions take 3 args  (_Unwind_State, UCB*, context*)
 *    rather than the 5-arg DWARF form.
 *  - The exception object is _Unwind_Control_Block, not _Unwind_Exception.
 *  - CONTINUE_UNWINDING in the personality calls __gnu_unwind_frame()
 *    which applies unwind opcodes via _Unwind_VRS_{Get,Set}.
 *  - Register access uses _Unwind_VRS_{Get,Set} keyed on register class
 *    and number rather than _Unwind_{Get,Set}GR.
 *
 * We implement _Unwind_RaiseException and friends here using libunwind's
 * cursor/proc-info infrastructure for per-frame lookups while calling
 * personalities with the correct 3-argument ARM EHABI convention.
 *
 * The _Unwind_Context* used throughout is the same
 *   struct { unw_cursor_t cursor; int end_of_stack; }
 * defined in src/unwind/unwind-internal.h.  The ARM personality stores the
 * UCB pointer in register r12 (UNWIND_POINTER_REG) via _Unwind_SetGR so
 * that _Unwind_GetLanguageSpecificData / _Unwind_GetRegionStart can
 * recover it.  Those writes land in uc->regs[12] via the null-loc fallback
 * added to tdep_access_reg().
 */

#define UNW_LOCAL_ONLY
#include "unwind_i.h"

#include <stdlib.h>
#include <stdint.h>

/* ARM EHABI types – pulled from GCC's ARM-specific unwind header.
 * We must NOT include libunwind's generic include/unwind.h here because it
 * declares _Unwind_GetGR/SetGR/GetIPInfo with DWARF signatures that conflict
 * with the ARM EHABI equivalents (static inline / macro in this header).  */
#include <unwind-arm-common.h>

/* ALIAS is defined in include/compiler.h via unwind_i.h */

/* ------------------------------------------------------------------ */
/* _Unwind_Context: same layout as unwind-internal.h                  */
/* ------------------------------------------------------------------ */

struct _Unwind_Context {
  unw_cursor_t cursor;
  int end_of_stack;
};

#define _Unwind_InitContext(ctx, uc)                               \
  ((ctx)->end_of_stack = 0,                                        \
   ((unw_getcontext (uc) < 0 ||                                    \
     unw_init_local (&(ctx)->cursor, (uc)) < 0) ? -1 : 0))

/* ARM personality function type – 3-argument EHABI form */
typedef _Unwind_Reason_Code (*_Unwind_ARM_Personality_Fn)
  (_Unwind_State, _Unwind_Control_Block *, struct _Unwind_Context *);

/* ------------------------------------------------------------------ */
/* _Unwind_VRS_Get / _Unwind_VRS_Set                                  */
/* ------------------------------------------------------------------ */

_Unwind_VRS_Result
_Unwind_VRS_Get (struct _Unwind_Context *context,
                 _Unwind_VRS_RegClass    regclass,
                 uint32_t                regno,
                 _Unwind_VRS_DataRepresentation representation,
                 void                   *valuep)
{
  if (regclass == _UVRSC_CORE && representation == _UVRSD_UINT32)
    {
      unw_word_t val;
      if (unw_get_reg (&context->cursor, (unw_regnum_t)(UNW_ARM_R0 + regno),
                       &val) < 0)
        return _UVRSR_FAILED;
      *(uint32_t *)valuep = (uint32_t)val;
      return _UVRSR_OK;
    }
  /* Other register classes (VFP, WMMXD, WMMXC) are not supported. */
  return _UVRSR_NOT_IMPLEMENTED;
}

_Unwind_VRS_Result
_Unwind_VRS_Set (struct _Unwind_Context *context,
                 _Unwind_VRS_RegClass    regclass,
                 uint32_t                regno,
                 _Unwind_VRS_DataRepresentation representation,
                 void                   *valuep)
{
  if (regclass == _UVRSC_CORE && representation == _UVRSD_UINT32)
    {
      unw_word_t val = *(uint32_t *)valuep;
      if (unw_set_reg (&context->cursor, (unw_regnum_t)(UNW_ARM_R0 + regno),
                       val) < 0)
        return _UVRSR_FAILED;
      return _UVRSR_OK;
    }
  return _UVRSR_NOT_IMPLEMENTED;
}

/* ------------------------------------------------------------------ */
/* _Unwind_GetGR / _Unwind_SetGR                                      */
/* ------------------------------------------------------------------ */

/* _Unwind_GetGR and _Unwind_SetGR are provided as static inline
 * wrappers around _Unwind_VRS_Get/Set in <unwind-arm-common.h>.
 * We expose the __libunwind_ aliases as standalone functions so that
 * check-namespace and ld.so symbol resolution work correctly.         */

unsigned long
__libunwind_Unwind_GetGR (struct _Unwind_Context *context, int index)
{
  uint32_t val = 0;
  _Unwind_VRS_Get (context, _UVRSC_CORE, (uint32_t)index,
                   _UVRSD_UINT32, &val);
  return (unsigned long)val;
}

void
__libunwind_Unwind_SetGR (struct _Unwind_Context *context, int index,
                          _Unwind_Word val)
{
  uint32_t v = (uint32_t)val;
  _Unwind_VRS_Set (context, _UVRSC_CORE, (uint32_t)index,
                   _UVRSD_UINT32, &v);
}

/* ------------------------------------------------------------------ */
/* _Unwind_GetIP / _Unwind_SetIP / _Unwind_GetIPInfo                  */
/* ------------------------------------------------------------------ */

_Unwind_Ptr
_Unwind_GetIP (struct _Unwind_Context *context)
{
  unw_word_t ip = 0;
  unw_get_reg (&context->cursor, UNW_REG_IP, &ip);
  return (_Unwind_Ptr)(ip & ~(unw_word_t)1);
}

void
_Unwind_SetIP (struct _Unwind_Context *context, _Unwind_Ptr new_value)
{
  Debug (2, "SetIP: new_value=0x%lx\n", (unsigned long)new_value);
  unw_set_reg (&context->cursor, UNW_REG_IP, (unw_word_t)new_value);
}

/* _Unwind_GetIPInfo is a macro in <unwind-arm-common.h>; provide a
 * standalone __libunwind_ symbol for check-namespace / ld.so.          */

unsigned long
__libunwind_Unwind_GetIPInfo (struct _Unwind_Context *context,
                              int *ip_before_insn)
{
  if (ip_before_insn)
    *ip_before_insn = 0;
  return (unsigned long)_Unwind_GetIP (context);
}

_Unwind_Ptr __libunwind_Unwind_GetIP (struct _Unwind_Context *)
     ALIAS (_Unwind_GetIP);
void __libunwind_Unwind_SetIP (struct _Unwind_Context *, _Unwind_Ptr)
     ALIAS (_Unwind_SetIP);

/* ------------------------------------------------------------------ */
/* _Unwind_GetCFA                                                      */
/* ------------------------------------------------------------------ */

_Unwind_Word
_Unwind_GetCFA (struct _Unwind_Context *context)
{
  unw_word_t val = 0;
  unw_get_reg (&context->cursor, UNW_ARM_CFA, &val);
  return (_Unwind_Word)val;
}

_Unwind_Word __libunwind_Unwind_GetCFA (struct _Unwind_Context *)
     ALIAS (_Unwind_GetCFA);

/* ------------------------------------------------------------------ */
/* _Unwind_GetRegionStart / _Unwind_GetLanguageSpecificData            */
/* ------------------------------------------------------------------ */

_Unwind_Ptr
_Unwind_GetRegionStart (struct _Unwind_Context *context)
{
  unw_proc_info_t pi;
  pi.start_ip = 0;
  unw_get_proc_info (&context->cursor, &pi);
  return (_Unwind_Ptr)pi.start_ip;
}

/*
 * For ARM EHABI, the personality stores the UCB pointer in r12
 * (_Unwind_SetGR(context, UNWIND_POINTER_REG=12, ucbp)).
 * _Unwind_GetLanguageSpecificData must reconstruct the LSDA pointer
 * from ucb->pr_cache.ehtp the same way GCC's pr-support.c does:
 *   ptr = ehtp;       // personality-function word
 *   ptr++;            // skip personality
 *   ptr += ((*ptr >> 24) & 0xff) + 1;  // skip unwind-opcode words
 *   return ptr;       // → LSDA
 */
void *
_Unwind_GetLanguageSpecificData (struct _Unwind_Context *context)
{
  /* Recover UCB pointer stored by personality in r12.  */
  _Unwind_Control_Block *ucbp =
    (_Unwind_Control_Block *)(uintptr_t)_Unwind_GetGR (context, 12);
  if (!ucbp || !ucbp->pr_cache.ehtp)
    {
      /* Fall back to pi.lsda if pr_cache not yet initialized. */
      unw_proc_info_t pi;
      pi.lsda = 0;
      unw_get_proc_info (&context->cursor, &pi);
      return (void *)(uintptr_t)pi.lsda;
    }
  const uint32_t *ptr = (const uint32_t *)ucbp->pr_cache.ehtp;
  ptr++;                              /* skip personality-function word */
  ptr += ((*ptr >> 24) & 0xff) + 1;  /* skip unwind-opcode words      */
  return (void *)ptr;
}

_Unwind_Ptr __libunwind_Unwind_GetRegionStart (struct _Unwind_Context *)
     ALIAS (_Unwind_GetRegionStart);
void * __libunwind_Unwind_GetLanguageSpecificData (struct _Unwind_Context *)
     ALIAS (_Unwind_GetLanguageSpecificData);

/* ------------------------------------------------------------------ */
/* _Unwind_GetDataRelBase / _Unwind_GetTextRelBase                    */
/* ------------------------------------------------------------------ */

_Unwind_Ptr
_Unwind_GetDataRelBase (struct _Unwind_Context *context UNUSED)
{
  return 0;
}

_Unwind_Ptr
_Unwind_GetTextRelBase (struct _Unwind_Context *context UNUSED)
{
  return 0;
}

_Unwind_Ptr __libunwind_Unwind_GetDataRelBase (struct _Unwind_Context *)
     ALIAS (_Unwind_GetDataRelBase);
_Unwind_Ptr __libunwind_Unwind_GetTextRelBase (struct _Unwind_Context *)
     ALIAS (_Unwind_GetTextRelBase);

/* ------------------------------------------------------------------ */
/* _Unwind_GetBSP                                                      */
/* ------------------------------------------------------------------ */

_Unwind_Word
_Unwind_GetBSP (struct _Unwind_Context *context UNUSED)
{
  return 0;
}

_Unwind_Word __libunwind_Unwind_GetBSP (struct _Unwind_Context *)
     ALIAS (_Unwind_GetBSP);

/* ------------------------------------------------------------------ */
/* _Unwind_DeleteException                                             */
/* ------------------------------------------------------------------ */

void
_Unwind_DeleteException (_Unwind_Exception *exception_object)
{
  if (exception_object->exception_cleanup)
    (*exception_object->exception_cleanup) (_URC_FOREIGN_EXCEPTION_CAUGHT,
                                            exception_object);
}

void __libunwind_Unwind_DeleteException (_Unwind_Exception *)
     ALIAS (_Unwind_DeleteException);

/* ------------------------------------------------------------------ */
/* _Unwind_FindEnclosingFunction                                       */
/* ------------------------------------------------------------------ */

void *
_Unwind_FindEnclosingFunction (void *pc)
{
  unw_proc_info_t pi;

  if (unw_get_proc_info_by_ip (unw_local_addr_space,
                               (unw_word_t)(uintptr_t)pc, &pi, NULL) < 0)
    return NULL;
  return (void *)(uintptr_t)pi.start_ip;
}

void * __libunwind_Unwind_FindEnclosingFunction (void *)
     ALIAS (_Unwind_FindEnclosingFunction);

/* ------------------------------------------------------------------ */
/* _Unwind_Backtrace                                                   */
/* ------------------------------------------------------------------ */

_Unwind_Reason_Code
_Unwind_Backtrace (_Unwind_Trace_Fn trace, void *trace_argument)
{
  struct _Unwind_Context context;
  unw_context_t uc;

  if (_Unwind_InitContext (&context, &uc) < 0)
    return _URC_FAILURE;

  while (1)
    {
      int ret = unw_step (&context.cursor);
      if (ret < 0)
        return _URC_FAILURE;
      _Unwind_Reason_Code reason = (*trace) (&context, trace_argument);
      if (reason != _URC_NO_REASON)
        return reason;
      if (ret == 0)
        return _URC_END_OF_STACK;
    }
}

_Unwind_Reason_Code __libunwind_Unwind_Backtrace (_Unwind_Trace_Fn, void *)
     ALIAS (_Unwind_Backtrace);

/* ------------------------------------------------------------------ */
/* Internal helpers for phase 2                                        */
/* ------------------------------------------------------------------ */

static void
arm_setup_frame (unw_proc_info_t *pi, _Unwind_Control_Block *ucbp)
{
  ucbp->pr_cache.fnstart = (uint32_t)pi->start_ip;
  /* lsda holds the .ARM.extab base (personality-function word).  */
  ucbp->pr_cache.ehtp    = (_Unwind_EHT_Header *)(uintptr_t)pi->lsda;
}

/*
 * Heap-allocated cursor snapshot for _Unwind_Resume to continue phase 2.
 *
 * The cleanup landing pad for frame N runs with the CALLER's SP (= N's CFA).
 * This corrupts N's saved-register area on the stack, making cursor-based
 * stack walking from inside _Unwind_Resume impossible.  We solve this by
 * snapshotting the cursor AFTER advancing past N (i.e., pointing at N's
 * caller) and storing it in ucbp->unwinder_cache.reserved4.
 * _Unwind_Resume retrieves and resumes from that snapshot.
 */
struct arm_phase2_snapshot {
  struct _Unwind_Context ctx; /* cursor state at the frame after cleanup */
  unw_context_t uc;           /* machine context; c->uc is redirected here */
};

/*
 * Save a snapshot of the phase-2 cursor advanced one step past the current
 * (cleanup) frame so _Unwind_Resume can resume from the correct point.
 * Stored in ucbp->unwinder_cache.reserved4 as a heap pointer.
 */
static void
arm_snapshot_phase2 (_Unwind_Control_Block *ucbp,
                     struct _Unwind_Context *context)
{
  struct arm_phase2_snapshot *snap = malloc (sizeof *snap);
  if (!snap)
    return;

  /* Advance past the current cleanup frame to its caller. */
  snap->ctx = *context;
  if (unw_step (&snap->ctx.cursor) <= 0)
    {
      free (snap);
      return;
    }

  /* Redirect c->uc and c->dwarf.as_arg to the snapshot's own copies so
     the cursor is self-contained.  The original uc and cursor live on a
     stack that will be reused by the landing pad; as_arg must always
     point to the live cursor (access_reg uses it to reach c->uc).  */
  struct cursor *c = (struct cursor *) &snap->ctx.cursor;
  snap->uc = *c->uc;
  c->uc          = &snap->uc;
  c->dwarf.as_arg = c;

  ucbp->unwinder_cache.reserved4 = (uint32_t)(uintptr_t) snap;
  Debug (2, "phase2: snapshot saved at %p\n", snap);
}

/*
 * Phase-2 loop.  step_first=1 means advance the cursor before the first
 * personality call (used when called from _Unwind_RaiseException, where the
 * cursor starts inside the unwinder itself).  step_first=0 means call
 * personality for the current cursor position first (used by _Unwind_Resume,
 * where the cursor is restored from a snapshot already pointing at the next
 * frame to handle).
 *
 * For cleanup frames (personality returns _URC_INSTALL_CONTEXT but
 * cur_sp != barrier_sp) we snapshot the cursor — advanced one step to the
 * cleanup frame's caller — so _Unwind_Resume can resume after the cleanup.
 */
static void __attribute__((noreturn))
arm_phase2 (_Unwind_Control_Block *ucbp, struct _Unwind_Context *context,
            int step_first)
{
  unw_proc_info_t pi;
  _Unwind_ARM_Personality_Fn personality;
  _Unwind_Reason_Code reason;

  while (1)
    {
      if (step_first)
        {
          if (unw_step (&context->cursor) <= 0)
            abort ();
        }
      step_first = 1;  /* every subsequent iteration must step */

      if (unw_get_proc_info (&context->cursor, &pi) < 0)
        continue;

      personality = (_Unwind_ARM_Personality_Fn)(uintptr_t)pi.handler;
      if (!personality)
        continue;

      arm_setup_frame (&pi, ucbp);
      /* Store UCB in r12 so _Unwind_GetLanguageSpecificData etc. can find it. */
      _Unwind_SetGR (context, 12, (_Unwind_Word)(uintptr_t)ucbp);

      Debug (2, "phase2: calling personality for start_ip=0x%lx\n",
             (unsigned long)pi.start_ip);
      reason = (*personality) (_US_UNWIND_FRAME_STARTING, ucbp, context);
      Debug (2, "phase2: personality returned %d\n", (int)reason);

      if (reason == _URC_INSTALL_CONTEXT)
        {
          unw_word_t cur_sp = 0;
          unw_get_reg (&context->cursor, UNW_ARM_CFA, &cur_sp);
          if (cur_sp != (unw_word_t) ucbp->barrier_cache.sp)
            {
              /* Cleanup frame: snapshot cursor at caller so _Unwind_Resume
                 can continue from there after the landing pad finishes.  */
              Debug (1, "phase2: cleanup at SP=0x%lx; saving snapshot\n",
                     (unsigned long)cur_sp);
              arm_snapshot_phase2 (ucbp, context);
            }
          else
            Debug (1, "phase2: handler frame at SP=0x%lx; installing\n",
                   (unsigned long)cur_sp);
          unw_resume (&context->cursor);
          abort ();
        }
      if (reason != _URC_CONTINUE_UNWIND)
        abort ();
    }
}

/* ------------------------------------------------------------------ */
/* _Unwind_RaiseException                                              */
/* ------------------------------------------------------------------ */

_Unwind_Reason_Code
_Unwind_RaiseException (_Unwind_Control_Block *ucbp)
{
  struct _Unwind_Context context;
  unw_context_t uc;
  unw_proc_info_t pi;
  _Unwind_ARM_Personality_Fn personality;
  _Unwind_Reason_Code reason;
  unw_word_t handler_sp;

  Debug (1, "(ucbp=%p)\n", ucbp);

  if (_Unwind_InitContext (&context, &uc) < 0)
    return _URC_FAILURE;

  /* ---- Phase 1: search for a handler ---- */

  while (1)
    {
      int ret = unw_step (&context.cursor);
      if (ret <= 0)
        {
          if (ret == 0)
            {
              Debug (1, "no handler found\n");
              return _URC_END_OF_STACK;
            }
          return _URC_FAILURE;
        }

      if (unw_get_proc_info (&context.cursor, &pi) < 0)
        continue;

      personality = (_Unwind_ARM_Personality_Fn)(uintptr_t)pi.handler;
      if (!personality)
        continue;

      arm_setup_frame (&pi, ucbp);
      /* Store the UCB pointer in r12 so helper functions can find it. */
      _Unwind_SetGR (&context, 12, (_Unwind_Word)(uintptr_t)ucbp);

      reason = (*personality) (_US_VIRTUAL_UNWIND_FRAME, ucbp, &context);

      if (reason == _URC_HANDLER_FOUND)
        break;
      if (reason != _URC_CONTINUE_UNWIND)
        {
          Debug (1, "personality returned %d in phase 1\n", reason);
          return _URC_FAILURE;
        }
    }

  /* Record the SP of the handler frame for phase-2 identification.  */
  handler_sp = 0;
  unw_get_reg (&context.cursor, UNW_ARM_CFA, &handler_sp);
  ucbp->barrier_cache.sp = (uint32_t)handler_sp;

  Debug (1, "handler found (SP=0x%lx); starting phase 2\n",
         (unsigned long)handler_sp);

  /* ---- Phase 2: unwind to the handler ---- */

  if (_Unwind_InitContext (&context, &uc) < 0)
    return _URC_FAILURE;

  arm_phase2 (ucbp, &context, 1 /* step_first */);
  __builtin_unreachable ();
}

_Unwind_Reason_Code
__libunwind_Unwind_RaiseException (_Unwind_Control_Block *)
     ALIAS (_Unwind_RaiseException);

/* ------------------------------------------------------------------ */
/* _Unwind_Resume                                                      */
/* ------------------------------------------------------------------ */

void __attribute__((noreturn))
_Unwind_Resume (_Unwind_Control_Block *ucbp)
{
  Debug (1, "(ucbp=%p)\n", ucbp);

  /* Retrieve the cursor snapshot saved by arm_snapshot_phase2() before
     the preceding cleanup landing pad was installed.  */
  struct arm_phase2_snapshot *snap =
    (struct arm_phase2_snapshot *)(uintptr_t) ucbp->unwinder_cache.reserved4;

  if (!snap)
    abort ();

  ucbp->unwinder_cache.reserved4 = 0;

  /* Copy context and uc to the local stack, then free the snapshot.
     The cursor's c->uc pointer was already redirected to snap->uc in
     arm_snapshot_phase2; after copying, we redirect it to the local copy.  */
  struct _Unwind_Context context = snap->ctx;
  unw_context_t uc = snap->uc;
  free (snap);

  /* Fix up the two pointers that referenced the (now-freed) snapshot memory.
     as_arg must point to the live cursor so access_reg can reach c->uc.  */
  struct cursor *c = (struct cursor *) &context.cursor;
  c->uc           = &uc;
  c->dwarf.as_arg = c;

  /* Continue phase 2 from the caller of the last cleanup frame.
     The cursor is already positioned at that frame; don't step again.  */
  arm_phase2 (ucbp, &context, 0 /* step_first=false */);
}

void __attribute__((noreturn)) __libunwind_Unwind_Resume (_Unwind_Control_Block *)
     ALIAS (_Unwind_Resume);

/* ------------------------------------------------------------------ */
/* _Unwind_Resume_or_Rethrow                                           */
/* ------------------------------------------------------------------ */

_Unwind_Reason_Code
_Unwind_Resume_or_Rethrow (_Unwind_Control_Block *ucbp)
{
  /* If a phase-2 snapshot exists (reserved4 != 0), continue that unwind.
     Otherwise re-raise from scratch.  */
  if (ucbp->unwinder_cache.reserved4)
    {
      _Unwind_Resume (ucbp);
      abort ();
    }
  return _Unwind_RaiseException (ucbp);
}

_Unwind_Reason_Code
__libunwind_Unwind_Resume_or_Rethrow (_Unwind_Control_Block *)
     ALIAS (_Unwind_Resume_or_Rethrow);

/* ------------------------------------------------------------------ */
/* _Unwind_ForcedUnwind                                                */
/* ------------------------------------------------------------------ */

/* Force unwinding support is not provided.  The ARM EHABI defines the
   interface but implementing it correctly requires integrating the stop
   function into every frame of the phase-2 loop, including after cleanup
   landing pads via _Unwind_Resume.  Return _URC_FAILURE so that callers
   (e.g. pthread_cancel) fail visibly rather than silently misbehave.
   This matches GCC libgcc's own ARM EHABI stub.  */
_Unwind_Reason_Code
_Unwind_ForcedUnwind (_Unwind_Control_Block *ucbp UNUSED,
                      _Unwind_Stop_Fn stop UNUSED, void *stop_parameter UNUSED)
{
  return _URC_FAILURE;
}

_Unwind_Reason_Code
__libunwind_Unwind_ForcedUnwind (_Unwind_Control_Block *,
                                  _Unwind_Stop_Fn, void *)
     ALIAS (_Unwind_ForcedUnwind);
