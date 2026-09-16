/* libunwind - a platform-independent unwind library
   Copyright (C) 2026 Matt Turner <mattst88@gmail.com>

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

/* Unwind from a signal that arrives on the first instruction of a function
   placed directly after another one. The interrupted IP must not be looked
   up as IP - 1, or it is attributed to the preceding function.

   The handler then resumes the victim's caller, after adding SIGUSR2 to the
   signal mask saved in the signal frame so that a resume through sigreturn
   can be detected. x86 and x86_64 resume every frame above a signal frame
   that way; other architectures must not. */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#define UNW_LOCAL_ONLY
#include <libunwind.h>
#include <signal.h>
#include <stdio.h>
#include <ucontext.h>
#include <string.h>
#include <unistd.h>

#include "unw_test.h"

#define PAD_NAME    "unw_test_sig_pad"
#define VICTIM_NAME "unw_test_sig_victim"
#define CALLER_NAME "unw_test_sig_caller"

/* Each architecture provides an instruction that faults when called with a
   NULL argument, and the return sequence. The test is skipped elsewhere. */
#if defined(__x86_64__)
# define LOAD "movq (%rdi), %rax"
# define RET  "ret"
#elif defined(__i386__)
# define LOAD "movl 0, %eax"
# define RET  "ret"
#elif defined(__aarch64__)
# define LOAD "ldr x0, [x0]"
# define RET  "ret"
#elif defined(__arm__)
# define LOAD "ldr r0, [r0]"
# define RET  "bx lr"
#elif defined(__powerpc64__) && defined(_CALL_ELF) && _CALL_ELF == 2
# define LOAD "ld 3, 0(3)"
# define RET  "blr"
#elif defined(__powerpc__) && !defined(__powerpc64__)
# define LOAD "lwz 3, 0(3)"
# define RET  "blr"
#elif defined(__s390x__)
# define LOAD "lg %r2, 0(%r2)"
# define RET  "br %r14"
#elif defined(__mips__)
# define LOAD "lw $2, 0($4)"
# define RET  "jr $31\n nop"
#elif defined(__loongarch64)
# define LOAD "ld.d $a0, $a0, 0"
# define RET  "jr $ra"
#elif defined(__alpha__)
# define LOAD "ldq $0, 0($16)"
# define RET  "ret $31, ($26), 1"
#elif defined(__hppa__)
# define LOAD "ldw 0(%r26), %r28"
# define RET  "bv %r0(%r2)\n nop"
#elif defined(__sparc__) && defined(__arch64__)
# define LOAD "ldx [%o0], %o0"
# define RET  "retl\n nop"
#elif defined(__sh__)
# define LOAD "mov.l @r4, r0"
# define RET  "rts\n nop"
#elif defined(__riscv)
# define LOAD "lw a0, 0(a0)"
# define RET  "ret"
#endif

#ifdef LOAD

#if defined(__arm__)
# define FNSTART ".arm\n .fnstart\n"
# define FNEND   " .fnend\n"
#else
# define FNSTART ""
# define FNEND   ""
#endif

#if defined(__mips__)
# define SET_NOREORDER ".set push\n .set noreorder\n"
# define SET_POP       ".set pop\n"
#else
# define SET_NOREORDER ""
# define SET_POP       ""
#endif

/* No alignment between the two functions, so the victim's first
   instruction directly follows the pad's last one. */
__asm__ (
  ".text\n"
  SET_NOREORDER
  ".globl " PAD_NAME "\n"
  ".type " PAD_NAME ", %function\n"
  PAD_NAME ":\n"
  FNSTART
  " .cfi_startproc\n"
  " " RET "\n"
  " .cfi_endproc\n"
  FNEND
  ".size " PAD_NAME ", . - " PAD_NAME "\n"
  ".globl " VICTIM_NAME "\n"
  ".type " VICTIM_NAME ", %function\n"
  VICTIM_NAME ":\n"
  FNSTART
  " .cfi_startproc\n"
  " " LOAD "\n"
  " " RET "\n"
  " .cfi_endproc\n"
  FNEND
  ".size " VICTIM_NAME ", . - " VICTIM_NAME "\n"
  SET_POP
);

void unw_test_sig_victim (void *);
void unw_test_sig_caller (void);

static int verbose;
static volatile int resumed;

__attribute__((noinline)) void
unw_test_sig_caller (void)
{
  unw_test_sig_victim (NULL);
  resumed = 1;
}

static void
handler (int sig, siginfo_t *si, void *ucontext)
{
  unw_context_t uc;
  unw_cursor_t c;
  unw_word_t ip, off;
  char name[128];
  int found_victim = 0;
  int depth = 0;
  int ret;

  (void) sig;
  (void) si;

  sigaddset (&((ucontext_t *) ucontext)->uc_sigmask, SIGUSR2);

  /* unw_getcontext() does not save the signal mask, and several
     architectures resume through a setcontext() that installs uc_sigmask,
     so clear it rather than resume with whatever was on the stack. See
     also Gtest-resume-sig. */
  memset (&uc, 0, sizeof (uc));
  unw_getcontext (&uc);
  UNW_TEST_ASSERT (unw_init_local (&c, &uc) == 0, "unw_init_local failed\n");

  do
    {
      unw_get_reg (&c, UNW_REG_IP, &ip);
      if (unw_get_proc_name (&c, name, sizeof (name), &off) != 0)
        {
          name[0] = '\0';
          off = 0;
        }
      if (verbose)
        printf ("%2d: ip=%#lx %s+%#lx\n", depth, (long) ip, name, (long) off);

      if (strcmp (name, VICTIM_NAME) == 0)
        {
          UNW_TEST_ASSERT (off == 0,
                           "signal at %s+%#lx, expected offset 0\n",
                           name, (long) off);
          found_victim = 1;
        }
      else if (strcmp (name, CALLER_NAME) == 0)
        {
          UNW_TEST_ASSERT (found_victim,
                           "reached " CALLER_NAME " without seeing "
                           VICTIM_NAME " (misidentified as " PAD_NAME "?)\n");
          ret = unw_resume (&c);
          UNW_TEST_ASSERT (0, "unw_resume returned %d\n", ret);
        }

      ++depth;
      ret = unw_step (&c);
    }
  while (ret > 0 && depth < 64);

  UNW_TEST_ASSERT (0, "did not reach " CALLER_NAME " (found_victim=%d, "
                   "unw_step returned %d)\n", found_victim, ret);
}

#if defined(__x86_64__) || defined(__i386__)
# define RESUME_THROUGH_TRAMPOLINE 1
#else
# define RESUME_THROUGH_TRAMPOLINE 0
#endif

static void
run (const char *what)
{
  sigset_t mask;

  if (verbose)
    printf ("%s:\n", what);

  sigemptyset (&mask);
  sigaddset (&mask, SIGSEGV);
  sigaddset (&mask, SIGUSR2);
  sigprocmask (SIG_UNBLOCK, &mask, NULL);

  resumed = 0;
  unw_test_sig_caller ();

  UNW_TEST_ASSERT (resumed, "%s: " CALLER_NAME " was not resumed\n", what);

  sigprocmask (SIG_BLOCK, NULL, &mask);
  UNW_TEST_ASSERT (sigismember (&mask, SIGUSR2) == RESUME_THROUGH_TRAMPOLINE,
                   "%s: unw_resume %s the signal trampoline\n", what,
                   RESUME_THROUGH_TRAMPOLINE ? "did not return through"
                                             : "returned through");
}

int
main (int argc, char **argv)
{
  struct sigaction sa;

  verbose = argc > 1;

  memset (&sa, 0, sizeof (sa));
  sa.sa_sigaction = handler;
  sa.sa_flags = SA_SIGINFO;
  sigemptyset (&sa.sa_mask);
  UNW_TEST_ASSERT (sigaction (SIGSEGV, &sa, NULL) == 0, "sigaction failed\n");

  run ("cold cache");
  run ("warm cache");
  UNW_TEST_ASSERT (unw_set_caching_policy (unw_local_addr_space,
                                           UNW_CACHE_NONE) == 0,
                   "unw_set_caching_policy failed\n");
  run ("no cache");

  if (verbose)
    printf ("SUCCESS\n");
  return UNW_TEST_EXIT_PASS;
}

#else /* !LOAD */

int
main (void)
{
  return UNW_TEST_EXIT_SKIP;
}

#endif /* !LOAD */
