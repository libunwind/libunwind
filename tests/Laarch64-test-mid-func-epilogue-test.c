/* Deterministic test for the get_frame_state RET-detection fix.
 *
 * Construct a fake ucontext positioned in body part 2 of
 * target_mid_func_epilogue (defined in Laarch64-test-mid-func-epilogue.S).
 * That function has its epilogue (LDP+RET) placed mid-function, with a
 * body-part-2 region that follows it and whose last instruction branches
 * back to the epilogue. While the IP is in body part 2 the frame record
 * is still intact on the stack — the LDP/RET haven't executed.
 *
 * unw_step must read the return address from the frame record at FP+8
 * (AT_FP path), not from x30 (LR fallback). To make the two paths
 * distinguishable we set ucontext.regs[30] to a value that DIFFERS from
 * the saved return address at FP+8:
 *
 *   AT_FP path  (with fix):    unwound IP = FAKE_RET_ADDR
 *   LR fallback (without fix): unwound IP = LR_GARBAGE
 */

#include "libunwind.h"
#include <ucontext.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>

extern int  target_mid_func_epilogue(int);
extern char target_mid_func_epilogue_in_body2[];

#define LR_GARBAGE      ((uint64_t)0xDEADBEEFCAFEBABEULL)
#define FAKE_CALLER_X29 ((uint64_t)0x1111111111111111ULL)
#define FAKE_RET_ADDR   ((uint64_t)0x2222222222222222ULL)

int main(void)
{
    /* Build a fake stack hosting the frame record that
     * target_mid_func_epilogue's prologue would have created.
     * After 'stp x29,x30,[sp,#-16]!' and 'mov x29,sp':
     *   x29 = SP = original_sp - 16
     *   memory[x29 + 0] = saved caller x29
     *   memory[x29 + 8] = saved return address (caller's x30)               */
    static uint64_t fake_stack[64];
    memset(fake_stack, 0, sizeof(fake_stack));

    uint64_t *frame_record = &fake_stack[16];   /* arbitrary 16-byte-aligned slot */
    frame_record[0] = FAKE_CALLER_X29;
    frame_record[1] = FAKE_RET_ADDR;

    uint64_t fp_x29  = (uint64_t)frame_record;        /* x29 in body part 2  */
    uint64_t mid_sp  = fp_x29;                         /* SP not yet popped   */

    ucontext_t uc;
    memset(&uc, 0, sizeof(uc));
    uc.uc_mcontext.pc       = (uint64_t)(void *)target_mid_func_epilogue_in_body2;
    uc.uc_mcontext.sp       = mid_sp;
    uc.uc_mcontext.regs[29] = fp_x29;
    uc.uc_mcontext.regs[30] = LR_GARBAGE;              /* distinct from FP+8  */

    unw_cursor_t c;
    int ret = unw_init_local2(&c, (unw_context_t *)&uc, UNW_INIT_SIGNAL_FRAME);
    if (ret < 0)
    {
        fprintf(stderr, "unw_init_local2 failed: %d\n", ret);
        return 1;
    }

    ret = unw_step(&c);
    if (ret < 0)
    {
        fprintf(stderr, "unw_step failed: %d\n", ret);
        return 1;
    }

    unw_word_t ip = 0;
    unw_get_reg(&c, UNW_REG_IP, &ip);

    printf("mid-func-epilogue test:\n");
    printf("  PC at body 2          = %p\n", (void *)target_mid_func_epilogue_in_body2);
    printf("  unwound IP            = 0x%lx\n",  (unsigned long)ip);
    printf("  expected (AT_FP)      = 0x%lx\n",  (unsigned long)FAKE_RET_ADDR);
    printf("  LR fallback would give  0x%lx\n",  (unsigned long)LR_GARBAGE);

    if ((uint64_t)ip == FAKE_RET_ADDR)
    {
        printf("PASS\n");
        return 0;
    }

    if ((uint64_t)ip == LR_GARBAGE)
        fprintf(stderr,
                "FAIL: unw_step took the LR fallback path "
                "(get_frame_state RET-detection fix missing)\n");
    else
        fprintf(stderr,
                "FAIL: unw_step returned IP 0x%lx, expected 0x%lx\n",
                (unsigned long)ip, (unsigned long)FAKE_RET_ADDR);
    return 1;
}
