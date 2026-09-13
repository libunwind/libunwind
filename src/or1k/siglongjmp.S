	/* Dummy implementation for now.  */

	.globl _UI_siglongjmp_cont
	.globl _UI_longjmp_cont

_UI_siglongjmp_cont:
_UI_longjmp_cont:
	l.jr	r9
	 l.nop

	.section .note.GNU-stack,"",@progbits
