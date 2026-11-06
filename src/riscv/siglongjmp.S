        .globl _UI_siglongjmp_cont
        .globl _UI_longjmp_cont

_UI_siglongjmp_cont:
#if defined(__linux__) && !defined(__GLIBC__)
        /* musl, see siglongjmp.c: a0 - return address, a1 - return value,
           a2 - value for s0 */
        mv      ra, a0
        mv      s0, a2
        mv      a0, a1
#endif
        /* Restoring the signal mask is not implemented otherwise.  */
        ret

        /* a0 - return address, a1 - return value */
_UI_longjmp_cont:
        mv      ra, a0
        mv      a0, a1
        ret

        /* We do not need executable stack.  */
        .section  .note.GNU-stack,"",%progbits
