/* libunwind - a platform-independent unwind library

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

#ifdef __linux__
# include <features.h>
#endif

	.globl _UI_siglongjmp_cont

#if defined(__powerpc64__) && defined(__linux__) && !defined(__GLIBC__)
	/* Only used without glibc, see siglongjmp.c: r3 - return address,
	   r4 - return value, r5 - value for r16; see JB_CONT_ARG.  */
	.type _UI_siglongjmp_cont, @function
_UI_siglongjmp_cont:
	mtlr	3
	mr	16, 5
	mr	3, 4
	blr
	.size _UI_siglongjmp_cont, .-_UI_siglongjmp_cont
#else
	/* Unused, but part of the ABI as a plain label.  */
_UI_siglongjmp_cont:
#endif

#ifdef __linux__
        /* We do not need executable stack.  */
        .section        .note.GNU-stack,"",@progbits
#endif
