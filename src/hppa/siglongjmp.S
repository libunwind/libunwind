	/* Dummy implementation for now.  */

	.globl _UI_siglongjmp_cont
	.globl _UI_longjmp_cont

_UI_siglongjmp_cont:
_UI_longjmp_cont:
	.proc
	.callinfo
	bv,n	%r0(%rp)
	.procend
#ifdef __linux__
	/* We do not need executable stack.  */
	.section	.note.GNU-stack,"",@progbits
#endif
