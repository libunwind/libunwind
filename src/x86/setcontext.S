/* libunwind - a platform-independent unwind library
   Copyright (C) 2026 libunwind contributors

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

#include "offsets.h"

/*  int _Ux86_setcontext (const ucontext_t *ucp)

  Restores the machine context provided.
  Unlike the libc implementation, doesn't clobber %eax (exception object
  pointer) or %edx (exception selector).
*/

	.global _Ux86_setcontext
	.type _Ux86_setcontext, @function
_Ux86_setcontext:
	.cfi_startproc

	/* EAX = ucp (argument) */
	mov	4(%esp),%eax

	/* Restore FP state */
	mov	(LINUX_UC_MCONTEXT_OFF+LINUX_SC_FPSTATE_OFF)(%eax),%ecx
	fldenv	(%ecx)

	/* Stash new EIP at new_ESP - 4 so we can 'ret' to it after
	   switching stacks.  Use ECX/EDX as scratch (caller-saved). */
	mov	(LINUX_UC_MCONTEXT_OFF+LINUX_SC_ESP_OFF)(%eax),%ecx
	mov	(LINUX_UC_MCONTEXT_OFF+LINUX_SC_EIP_OFF)(%eax),%edx
	mov	%edx,-4(%ecx)

	/* Restore callee-saved registers */
	mov	(LINUX_UC_MCONTEXT_OFF+LINUX_SC_EBX_OFF)(%eax),%ebx
	mov	(LINUX_UC_MCONTEXT_OFF+LINUX_SC_EBP_OFF)(%eax),%ebp
	mov	(LINUX_UC_MCONTEXT_OFF+LINUX_SC_ESI_OFF)(%eax),%esi
	mov	(LINUX_UC_MCONTEXT_OFF+LINUX_SC_EDI_OFF)(%eax),%edi

	/* Restore EDX (exception selector) */
	mov	(LINUX_UC_MCONTEXT_OFF+LINUX_SC_EDX_OFF)(%eax),%edx

	/* Switch to new stack (EIP waiting at new_ESP - 4) */
	mov	(LINUX_UC_MCONTEXT_OFF+LINUX_SC_ESP_OFF)(%eax),%esp
	sub	$4,%esp

	/* Restore EAX (exception object pointer) last — clobbers ucp */
	mov	(LINUX_UC_MCONTEXT_OFF+LINUX_SC_EAX_OFF)(%eax),%eax

	ret
	.cfi_endproc

	.size	_Ux86_setcontext, . - _Ux86_setcontext

	/* We do not need executable stack.  */
	.section	.note.GNU-stack,"",@progbits
