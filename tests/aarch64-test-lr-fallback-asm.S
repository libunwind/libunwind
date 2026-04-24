/* libunwind - a platform-independent unwind library

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

/*
 * Assembly functions for the AArch64 LR fallback end-to-end test.
 *
 * Each function uses a different prologue pattern and has NO .cfi directives,
 * so DWARF unwinding will fail and libunwind's LR fallback path is exercised.
 * Each function spins on an atomic bool until it becomes false.
 *
 * The test sends a signal while the thread is inside one of these functions,
 * then verifies the unwinder can produce a complete backtrace.
 */

    .text

/*
 * void no_eh_frame_sub_sp(std::atomic<bool> *flag)
 *
 * Prologue pattern: SUB SP, SP, #imm
 *   sub sp, sp, #0x20
 */
    .global no_eh_frame_sub_sp
    .type no_eh_frame_sub_sp, %function
no_eh_frame_sub_sp:
    sub sp, sp, #0x20
1:
    ldarb w1, [x0]
    cbnz  w1, 1b
    add sp, sp, #0x20
    ret
    .size no_eh_frame_sub_sp, .-no_eh_frame_sub_sp

/*
 * void no_eh_frame_stp_pre_index(std::atomic<bool> *flag)
 *
 * Prologue pattern: STP Xt1, Xt2, [SP, #-imm]!
 *   stp x20, x19, [sp, #-32]!
 */
    .global no_eh_frame_stp_pre_index
    .type no_eh_frame_stp_pre_index, %function
no_eh_frame_stp_pre_index:
    stp x20, x19, [sp, #-32]!
1:
    ldarb w1, [x0]
    cbnz  w1, 1b
    ldp x20, x19, [sp], #32
    ret
    .size no_eh_frame_stp_pre_index, .-no_eh_frame_stp_pre_index

/*
 * void no_eh_frame_str_pre_index(std::atomic<bool> *flag)
 *
 * Prologue pattern: STR Xt, [SP, #-imm]!
 *   str x19, [sp, #-16]!
 */
    .global no_eh_frame_str_pre_index
    .type no_eh_frame_str_pre_index, %function
no_eh_frame_str_pre_index:
    mov x9, x19
    str x19, [sp, #-16]!
1:
    ldarb w1, [x0]
    cbnz  w1, 1b
    ldr x19, [sp], #16
    mov x19, x9
    ret
    .size no_eh_frame_str_pre_index, .-no_eh_frame_str_pre_index
