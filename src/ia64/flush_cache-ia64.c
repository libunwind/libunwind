/* libunwind - a platform-independent unwind library
   Copyright (C) 2002-2003 Hewlett-Packard Co
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

#include "unwind_i.h"

#ifdef HAVE_IA64INTRIN_H
# include <ia64intrin.h>
#endif

void
unw_flush_cache (unw_addr_space_t as, unw_word_t lo, unw_word_t hi)
{
  /* This lets us flush caches lazily.  The implementation currently
     ignores the flush range arguments (lo-hi).  This is OK because
     unw_flush_cache() is allowed to flush more than the requested
     range. */

#ifdef HAVE_IA64INTRIN_H
  __sync_fetch_and_add(&as->cache_generation, 1);
#else
# warning unw_flush_cache(): need a way to atomically increment an integer.
  ++as->cache_generation;
#endif
}
