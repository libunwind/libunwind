/* libunwind - a platform-independent unwind library
   Copyright (C) 2012 Tommi Rantala <tt.rantala@gmail.com>

   Modified for or1k by Ali Ahmet Memiş <aliamemis@disroot.org>

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

/* Use glibc's jump-buffer indices; NPTL peeks at SP.  __sigsetjmp in
   sysdeps/or1k/setjmp.S stores, in order:

    0. r1 (sp)
    1. r2
    2. r9 (link register, i.e. the resume address)
    3. r10
    4. r14
    5. r16
    6. r18
    7. r20
    8. r22
    9. r24
   10. r26
   11. r28
   12. r30

   __jmp_buf is 13 words, so __mask_was_saved and __saved_mask of
   struct __jmp_buf_tag follow at word 13 and 14.  */

#define JB_SP           0
#define JB_RP           2
#define JB_MASK_SAVED   13
#define JB_MASK         14
