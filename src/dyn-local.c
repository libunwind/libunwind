/* libunwind - a platform-independent unwind library
   Copyright (C) 2002 Hewlett-Packard Co
	Contributed by David Mosberger-Tang <davidm@hpl.hp.com>

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

/* This gets linked in only when UNW_REMOTE_ONLY is not defined.  It
   must be a separate file to ensure registering dynamic info doesn't
   automatically drag in the lookup code and vice versa.  */

#include <libunwind.h>

/* Note: It might be tempting to use the LSDA to store
	 _U_dyn_info_list, but that wouldn't work because the
	 unwind-info is generally mapped read-only.  */

unw_dyn_info_list_t _U_dyn_info_list;

/* Now create a special unwind-table entry which makes it easy for an
   unwinder to locate the dynamic registration list.  The special
   entry covers address range [0-0) and is therefore guaranteed to
   be the first in the unwind-table.  */
asm (
"	.section \".IA_64.unwind_info\", \"a\"\n"
".info:	data8 (1<<48) | 1\n"	/* v1, length==1 (8-byte word) */
"	data8 0\n"		/* 8 empty .prologue directives (nops) */
"	data8 0\n"		/* personality routine (ignored) */
"	data8 @segrel(_U_dyn_info_list)\n"	/* lsda */
"\n"
"	.section \".IA_64.unwind\", \"a\"\n"
"	data8 0, 0, @segrel(.info)\n"
"	.previous\n");
