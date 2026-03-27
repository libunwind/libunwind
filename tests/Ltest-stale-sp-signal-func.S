/* Assembly helpers for Ltest-stale-sp-signal.
 *
 * Reproduces the stale loc[SP] bug in the AT_SP_OFFSET fallback path of
 * src/aarch64/Gstep.c (line 719 before the fix).
 *
 * Call chain exercised by the test:
 *
 *   worker_thread_fn  (C, has .eh_frame)
 *     stale_sp_target_func  (asm, NO .eh_frame, signed-offset STP, no FP update)
 *       inner_loop_spin  (asm, proper .cfi frame, spins for N iterations)
 *
 * When SIGUSR1 fires inside inner_loop_spin:
 *   1. unw_init_local2(UNW_INIT_SIGNAL_FRAME) places cursor at inner_loop_spin.
 *   2. unw_step: DWARF succeeds for inner_loop_spin.
 *        c->dwarf.cfa is updated to inner_loop_spin's CFA
 *                     = stale_sp_target_func's SP after its prologue.
 *        loc[SP] is NOT updated (still points to ucontext.sp
 *                     = inner_loop_spin's interrupted SP).
 *   3. unw_step: DWARF fails for stale_sp_target_func (no .eh_frame).
 *        get_frame_state() scans the function body, finds the signed-offset
 *        STP x29,x30,[sp,#0], returns AT_SP_OFFSET with offset=0.
 *        Bug  (before fix): sp = dwarf_get(loc[SP]) = inner_loop_spin's
 *             interrupted SP ≠ stale_sp_target_func's SP → wrong FP/LR.
 *        Fix: sp = c->dwarf.cfa = stale_sp_target_func's SP → correct FP/LR.
 */

    .text

/* -------------------------------------------------------------------------
 * inner_loop_spin(long iters)
 *
 * Spins for `iters` iterations.  Has a full 32-byte frame with proper .cfi
 * annotations so that libunwind can DWARF-step through it.
 * After the step, c->dwarf.cfa == stale_sp_target_func's SP (the SP value
 * right after its "sub sp,sp,#128" — NOT the stale ucontext.sp).
 * -------------------------------------------------------------------------*/
    .globl  inner_loop_spin
    .type   inner_loop_spin, %function
    .p2align 2
inner_loop_spin:
    .cfi_startproc
    stp     x29, x30, [sp, #-32]!
    .cfi_def_cfa_offset 32
    .cfi_offset 29, -32
    .cfi_offset 30, -24
    mov     x29, sp
    .cfi_def_cfa_register 29
    str     x19, [sp, #16]
    .cfi_offset 19, -16
    mov     x19, x0             /* x19 = iteration counter */
.Lspin:
    subs    x19, x19, #1
    b.ne    .Lspin
    ldr     x19, [sp, #16]
    ldp     x29, x30, [sp], #32
    ret
    .cfi_endproc
    .size   inner_loop_spin, . - inner_loop_spin

/* -------------------------------------------------------------------------
 * stale_sp_target_func(long iters)
 *
 * Allocates a 128-byte frame and stores the frame record with a SIGNED-OFFSET
 * STP (not pre-index), deliberately omitting "mov x29,sp" so that
 * get_frame_state() returns AT_SP_OFFSET (not AT_FP).  No .cfi_startproc →
 * no .eh_frame → DWARF step fails → AT_SP_OFFSET fallback fires.
 * -------------------------------------------------------------------------*/
    .globl  stale_sp_target_func
    .type   stale_sp_target_func, %function
    .p2align 2
stale_sp_target_func:
    /* Allocate frame: sub first so that [sp+0] is within the frame. */
    sub     sp, sp, #128
    /* SIGNED-OFFSET STP (encoding 0xa9007bfd for offset=0): frame record
     * is at [sp+0].  This is what get_frame_state() will find and return
     * as AT_SP_OFFSET with fs.offset=0.
     * NO .cfi_startproc here → no .eh_frame for this function.
     * NO "mov x29,sp" → get_frame_state stays AT_SP_OFFSET, not AT_FP. */
    stp     x29, x30, [sp, #0]
    str     x19, [sp, #16]
    mov     x19, x0             /* x19 = iters */
    mov     x0, x19
    bl      inner_loop_spin
    ldr     x19, [sp, #16]
    ldp     x29, x30, [sp, #0]
    add     sp, sp, #128
    ret
    .size   stale_sp_target_func, . - stale_sp_target_func
