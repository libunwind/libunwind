/* Test for the stale loc[SP] bug in the AT_SP_OFFSET fallback path of
 * src/aarch64/Gstep.c.
 *
 * Bug: when DWARF fails for a function whose prologue is a signed-offset
 * "stp x29,x30,[sp,#N]" (without "mov x29,sp"), the AT_SP_OFFSET fallback
 * reads the frame-record base from loc[UNW_AARCH64_SP].  That loc always
 * points into the ucontext saved-register area and is never updated after
 * the first frame, so after one or more successful DWARF steps it holds a
 * stale SP (the interrupted thread's SP) rather than the actual SP of the
 * frame being unwound.  Reading the frame record from the wrong address
 * produces a garbage IP, which eventually causes a SIGSEGV in apply_reg_state.
 *
 * Fix: use c->dwarf.cfa instead of loc[SP].  The CFA is correctly
 * maintained through every DWARF step and equals the actual SP of the
 * current frame at the time the fallback fires.
 *
 * Test strategy:
 *   Worker thread: worker_thread_fn → stale_sp_target_func → inner_loop_spin
 *   SIGUSR1 handler: unw_init_local2(UNW_INIT_SIGNAL_FRAME) + unw_step loop,
 *   verifying that "worker_thread_fn" is found within MAX_FRAMES steps.
 *
 *   Without the fix, the AT_SP_OFFSET fallback uses the wrong SP, producing
 *   a corrupted backtrace that does not contain "worker_thread_fn".
 *   With the fix, the correct FP/LR are recovered and "worker_thread_fn"
 *   is found.
 *
 *   Only signals that actually interrupted inner_loop_spin are counted:
 *   signals landing in a prologue/epilogue or in worker_thread_fn between
 *   calls are skipped (they don't exercise the AT_SP_OFFSET path and are
 *   valid occurrences in any timing environment).
 */

#define UNW_LOCAL_ONLY
#include "libunwind.h"
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdatomic.h>
#include <unistd.h>

#define MAX_FRAMES  48
#define N_SIGNALS   200
#define SPIN_ITERS  10000000L   /* keep thread in inner_loop_spin most of the time */

/* Minimum number of signals that must have landed in inner_loop_spin.
 * If fewer than this land in the target function the test is skipped
 * (SKIP = exit 77) rather than failed, as it indicates an environment
 * constraint (very slow emulator, debug-level overhead, etc.) rather than
 * a regression in the fix itself. */
#define MIN_IN_PLACE 5

static atomic_int signals_verified = 0;
static atomic_int signals_failed   = 0;
static pthread_t  worker_tid;

extern void stale_sp_target_func(long iters);
extern void inner_loop_spin(long iters);

static void handler(int sig, siginfo_t *info, void *uctx)
{
    (void)sig; (void)info;

    unw_cursor_t  c;
    int ret = unw_init_local2(&c, (unw_context_t *)uctx, UNW_INIT_SIGNAL_FRAME);
    if (ret < 0)
        return;

    /* Only test signals that actually interrupted inner_loop_spin.
     * Signals landing in stale_sp_target_func's prologue/epilogue or in
     * worker_thread_fn between calls do not exercise the AT_SP_OFFSET path
     * and are not counted as either verified or failed.
     *
     * Use UNW_AARCH64_PC (not UNW_REG_IP which maps to LR/X30) to get the
     * actual interrupted instruction pointer from the signal ucontext. */
    unw_word_t ip;
    if (unw_get_reg(&c, UNW_AARCH64_PC, &ip) < 0)
        return;
    if (ip < (unw_word_t)inner_loop_spin ||
        ip >= (unw_word_t)stale_sp_target_func)
        return;

    /* Walk up to MAX_FRAMES and look for "worker_thread_fn". */
    char name[256];
    int  found = 0;
    int  steps = 0;
    while (unw_step(&c) > 0 && steps < MAX_FRAMES)
    {
        unw_word_t offset;
        if (unw_get_proc_name(&c, name, sizeof(name), &offset) == 0)
        {
            if (strstr(name, "worker_thread_fn"))
            {
                found = 1;
                break;
            }
        }
        steps++;
    }

    if (found)
        atomic_fetch_add(&signals_verified, 1);
    else
        atomic_fetch_add(&signals_failed, 1);
}

void *worker_thread_fn(void *arg)
{
    (void)arg;
    while (1)
        stale_sp_target_func(SPIN_ITERS);
    return NULL;
}

int main(void)
{
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_sigaction = handler;
    sa.sa_flags     = SA_SIGINFO;
    sigaction(SIGUSR1, &sa, NULL);

    pthread_create(&worker_tid, NULL, worker_thread_fn, NULL);

    /* Give the worker time to enter its spin loop before the first signal. */
    usleep(10000);

    for (int i = 0; i < N_SIGNALS; i++)
    {
        pthread_kill(worker_tid, SIGUSR1);
        usleep(500);
    }

    /* Give the handler time to finish the last signal. */
    usleep(50000);

    int verified = atomic_load(&signals_verified);
    int failed   = atomic_load(&signals_failed);
    int in_place = verified + failed;

    printf("signals sent: %d  in-target: %d  verified: %d  failed: %d\n",
           N_SIGNALS, in_place, verified, failed);

    if (failed > 0)
    {
        fprintf(stderr, "FAIL: %d signal(s) interrupted inner_loop_spin "
                "but produced a bad backtrace\n", failed);
        exit(1);
    }
    if (in_place < MIN_IN_PLACE)
    {
        /* Not enough signals landed in inner_loop_spin to meaningfully test
         * the fix.  This is an environment/timing issue rather than a
         * regression; skip rather than fail so CI stays green. */
        printf("SKIP: only %d/%d signals landed in inner_loop_spin "
               "(need at least %d)\n", in_place, N_SIGNALS, MIN_IN_PLACE);
        return 77;
    }

    printf("PASS\n");
    return 0;
}
