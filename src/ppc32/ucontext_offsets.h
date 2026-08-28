/* Numeric ucontext_t offsets for use from getcontext.S.

   ucontext_i.h keeps the same values as C expressions for use from C, but
   an assembler file needs literals.  The _Static_assert()-style checks at
   the bottom tie the two together: if the C library ever changes the
   layout, the build breaks instead of silently writing to the wrong
   place.  */

#ifndef ucontext_offsets_h
#define ucontext_offsets_h

/* Built only where libunwind provides its own getcontext(), the same
   condition src/Makefile.am uses to pick the assembly.  */
#if defined(__linux__) && !UNW_HAS_LIBC_GETCONTEXT

#define UC_REGS_PTR_OFF   48    /* ucontext_t.uc_regs     */
#define UC_MCONTEXT_OFF   192   /* ucontext_t.uc_mcontext */
#define UC_GREG_SIZE      4     /* sizeof(greg_t)         */

/* Same values as ucontext_i.h, repeated because that header cannot be
   included from assembly; kept in sync by the checks below.  */
#define UC_NIP_IDX        32
#define UC_CTR_IDX        35
#define UC_LINK_IDX       36
#define UC_XER_IDX        37
#define UC_CCR_IDX        38

#define UC_FLAGS_OFF      0     /* ucontext_t.uc_flags    */
#define UC_LINK_OFF       4     /* ucontext_t.uc_link     */
#define UC_STACK_OFF      8     /* ucontext_t.uc_stack    */
#define UC_STACK_SIZE     12    /* sizeof(stack_t)        */
#define UC_TOTAL_SIZE     1184  /* sizeof(ucontext_t)     */
#define UC_SIGMASK_OFF    52    /* ucontext_t.uc_sigmask  */
#define UC_SIGMASK_SIZE   128   /* sizeof(sigset_t)       */
#define UC_KERNEL_SIGSET_SIZE 8 /* what rt_sigprocmask writes */
#define UC_FPREGS_OFF     384   /* uc_mcontext.fpregs.fpregs */
#define UC_FPREG_SIZE     8

#define UC_MSR_IDX        33

#define UC_GREG(n)        (UC_MCONTEXT_OFF + (n) * UC_GREG_SIZE)
/* Same register, but relative to uc_regs rather than to the ucontext.  */
#define UC_MREG(n)        ((n) * UC_GREG_SIZE)
/* Same FPR, relative to uc_regs rather than to the ucontext.  */
#define UC_MFPREG(n)      (UC_FPREGS_OFF - UC_MCONTEXT_OFF + (n) * UC_FPREG_SIZE)
#define UC_FPREG(n)       (UC_FPREGS_OFF + (n) * UC_FPREG_SIZE)

#ifndef __ASSEMBLER__

#include <signal.h>
#include <stddef.h>
#include <ucontext.h>

/* The *_IDX names checked below come from here.  */
#include "ucontext_i.h"

#define UC_CHECK(name, expr, want) \
  _Static_assert ((expr) == (want), #name ": " #expr " != " #want)

UC_CHECK (uc_check_regs_ptr, offsetof (ucontext_t, uc_regs),     UC_REGS_PTR_OFF);
UC_CHECK (uc_check_mcontext, offsetof (ucontext_t, uc_mcontext), UC_MCONTEXT_OFF);
UC_CHECK (uc_check_greg_sz,  sizeof (greg_t),                    UC_GREG_SIZE);
UC_CHECK (uc_check_greg0,    offsetof (ucontext_t, uc_mcontext.gregs[0]),  UC_GREG (0));
UC_CHECK (uc_check_greg1,    offsetof (ucontext_t, uc_mcontext.gregs[1]),  UC_GREG (1));
UC_CHECK (uc_check_greglink, offsetof (ucontext_t, uc_mcontext.gregs[LINK_IDX]), UC_GREG (LINK_IDX));

/* ... and that the duplicated indices still agree with ucontext_i.h.  */
UC_CHECK (uc_check_idx_nip,  NIP_IDX,  UC_NIP_IDX);
UC_CHECK (uc_check_idx_ctr,  CTR_IDX,  UC_CTR_IDX);
UC_CHECK (uc_check_idx_link, LINK_IDX, UC_LINK_IDX);
UC_CHECK (uc_check_idx_xer,  XER_IDX,  UC_XER_IDX);
UC_CHECK (uc_check_idx_ccr,  CCR_IDX,  UC_CCR_IDX);
UC_CHECK (uc_check_idx_msr,  MSR_IDX,  UC_MSR_IDX);

UC_CHECK (uc_check_flags,    offsetof (ucontext_t, uc_flags),   UC_FLAGS_OFF);
UC_CHECK (uc_check_link,     offsetof (ucontext_t, uc_link),    UC_LINK_OFF);
UC_CHECK (uc_check_stack,    offsetof (ucontext_t, uc_stack),   UC_STACK_OFF);
UC_CHECK (uc_check_stack_sz, sizeof (stack_t), UC_STACK_SIZE);
UC_CHECK (uc_check_total_sz, sizeof (ucontext_t), UC_TOTAL_SIZE);
UC_CHECK (uc_check_sigmask,  offsetof (ucontext_t, uc_sigmask), UC_SIGMASK_OFF);
UC_CHECK (uc_check_sigmasksz, sizeof (sigset_t), UC_SIGMASK_SIZE);
/* rt_sigprocmask() takes the kernel's sigset, which is narrower than the
   one libc exposes.  */
UC_CHECK (uc_check_ksigsetsz, _NSIG / 8, UC_KERNEL_SIGSET_SIZE);
UC_CHECK (uc_check_fpreg0,   offsetof (ucontext_t, uc_mcontext.fpregs.fpregs[0]), UC_FPREG (0));
UC_CHECK (uc_check_fpreg_sz, sizeof (double), UC_FPREG_SIZE);
UC_CHECK (uc_check_fpregs_sz,
          sizeof (((mcontext_t *) 0)->fpregs.fpregs), 32 * UC_FPREG_SIZE);
/* The whole mcontext is copied into a kernel sigframe, so the assembly
   has to account for what follows the register arrays as well.  */
UC_CHECK (uc_check_mcontext_sz, sizeof (mcontext_t),
          UC_TOTAL_SIZE - UC_MCONTEXT_OFF);

#undef UC_CHECK

#endif /* !__ASSEMBLER__ */
#endif /* __linux__ && !UNW_HAS_LIBC_GETCONTEXT */
#endif /* ucontext_offsets_h */
