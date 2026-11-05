        .globl _UI_siglongjmp_cont
        .globl _UI_longjmp_cont

        /* Restoring the signal mask is not implemented yet.  */
_UI_siglongjmp_cont:
        ret

        /* a0 - return address, a1 - return value */
_UI_longjmp_cont:
        mv      ra, a0
        mv      a0, a1
        ret

        /* We do not need executable stack.  */
        .section  .note.GNU-stack,"",%progbits
