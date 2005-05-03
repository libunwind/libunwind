	/* Dummy implementation for now.  */

	.globl _UI_siglongjmp_cont
	.globl _UI_longjmp_cont

_UI_siglongjmp_cont:
_UI_longjmp_cont:
	.proc
	.callinfo
#warning fix me
	bv	%r0(%rp)
	.procend
