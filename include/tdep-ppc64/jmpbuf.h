/* libunwind - a platform-independent unwind library
   Copyright (C) 2006-2007 IBM
   Contributed by
     Corey Ashford <cjashfor@us.ibm.com>
     Jose Flavio Aguilar Paulino <jflavio@br.ibm.com> <joseflavio@gmail.com>

   Copied from libunwind-x86_64.h, modified slightly for building
   frysk successfully on ppc64, by Wu Zhou <woodzltc@cn.ibm.com>
   Will be replaced when libunwind is ready on ppc64 platform.

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

#if defined __linux__ && !defined __GLIBC__

/* musl: lr, cr, r1, r2, r14-r31, ...  in __jmp_buf[32], 16 bytes each,
   then __fl and __ss; src/setjmp/powerpc64/setjmp.s  */
#define JB_RP           0
#define JB_SP           2
#define JB_MASK_SAVED   64      /* __fl */
#define JB_MASK         65      /* __ss */

/* musl's sigsetjmp() keeps the jmp_buf pointer in a callee-saved register
   while it calls setjmp(), and setjmp() stores that register in slot
   JB_MUSL_SIGSETJMP_SLOT; siglongjmp() has _UI_siglongjmp_cont restore
   it.  */
#define JB_MUSL_SIGSETJMP_SLOT  6       /* r16 */

#else

/* Use glibc's jump-buffer indices; NPTL peeks at SP: */

#define JB_SP           6
#define JB_RP           7
#define JB_MASK_SAVED   8
#define JB_MASK         9

#endif

/* The exception-handling registers start at r12 on ppc64, and r13 is the
   thread pointer, which is not restored on resume.  Pass the arguments of
   _UI_longjmp_cont and _UI_siglongjmp_cont in r3, r4 and r5 instead.  */
#define JB_CONT_ARG(n)  (UNW_PPC64_R3 + (n))
