/* Target function for the get_frame_state RET-detection fix.
 *
 * The epilogue (LDP+RET) is positioned mid-function: a body-part-1
 * region precedes it, and a body-part-2 region follows it whose last
 * instruction branches back to the epilogue. This is a perfectly normal
 * compiler layout — e.g. clang/gcc may move a hot path so the common
 * case falls through and a less common case branches forward into a
 * tail of code that joins the epilogue with a backward branch.
 *
 * Layout (instructions are 4 bytes each):
 *
 *   target_mid_func_epilogue:      ; T+0
 *      stp    x29, x30, [sp,-16]!  ; T+0   prologue
 *      mov    x29, sp              ; T+4   prologue
 *      cbnz   w0, .Lpart2          ; T+8   body part 1: jump to body part 2
 *      mov    w0, #1               ; T+12  body part 1
 *   .Lepilogue:
 *      ldp    x29, x30, [sp], #16  ; T+16  epilogue (mid-function)
 *      ret                         ; T+20  epilogue
 *   .Lpart2:                       ; T+24  body part 2
 *   target_mid_func_epilogue_in_body2:
 *      mov    w0, #2               ; T+24  body part 2
 *      b      .Lepilogue           ; T+28  body part 2: goto epilogue
 *
 * No .cfi_* directives: the function has no DWARF unwind info, so
 * dwarf_step() returns -UNW_ENOINFO and unw_step() falls through to the
 * get_frame_state() path that the fix targets.
 *
 * The bug condition is exposed when execution is in body part 2 (IP at
 * or after T+24): the frame record on the stack is still intact because
 * the LDP/RET at T+16/T+20 has not been executed. But get_frame_state
 * walks the bytes from T to the current IP and sees both the LDP and
 * the RET; without the fix it concludes fs.loc=NONE and unw_step takes
 * the LR fallback (no CFA/FP update). With the fix, the RET-detection
 * block restores fs.loc=AT_FP and unw_step uses the (still-valid)
 * frame record at x29.
 */

    .text
    .globl  target_mid_func_epilogue
    .type   target_mid_func_epilogue, %function
    .p2align 2

target_mid_func_epilogue:
    /* === Prologue === */
    stp     x29, x30, [sp, #-16]!
    mov     x29, sp

    /* === Body part 1 === */
    cbnz    w0, .Lpart2
    mov     w0, #1

    /* === Epilogue (mid-function) === */
.Lepilogue:
    ldp     x29, x30, [sp], #16
    ret

    /* === Body part 2 ===
     * Reached via the cbnz above. While IP is in this region the
     * LDP/RET have not run: x29 still equals (original_sp - 16) and
     * the frame record [caller_x29, return_addr] is still on the stack.
     *
     * Exported as a plain label (no %function type) so get_proc_name
     * still resolves the enclosing target_mid_func_epilogue for any IP
     * in body part 2. */
    .globl  target_mid_func_epilogue_in_body2
target_mid_func_epilogue_in_body2:
.Lpart2:
    mov     w0, #2
    b       .Lepilogue          /* last instruction: goto epilogue */

    .size   target_mid_func_epilogue, . - target_mid_func_epilogue
