/* libunwind - a platform-independent unwind library
   Copyright (C) 2001-2002 Hewlett-Packard Co
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

#include <stdio.h>

#include "internal.h"
#include "tdep.h"

#ifndef UNW_REMOTE_ONLY

static inline int
local_find_proc_info (unw_dyn_info_list_t *list,
		      unw_addr_space_t as, unw_word_t ip, unw_proc_info_t *pi,
		      int need_unwind_info, void *arg)
{
  unw_dyn_info_t *di;

  for (di = list->first; di; di = di->next)
    if (ip >= di->start_ip && ip < di->end_ip)
      return unwi_extract_dynamic_proc_info (as, ip, pi, di, need_unwind_info,
					     arg);
  return -UNW_ENOINFO;
}

#endif /* UNW_REMOTE_ONLY */

static inline int
remote_find_proc_info (unw_addr_space_t as, unw_word_t ip, unw_proc_info_t *pi,
		       int need_unwind_info, void *arg)
{
  unw_word_t generation;
  int ret;

  ret = unwi_dyn_remote_find_proc_info (as, ip, pi, &generation,
					need_unwind_info, arg);
  if (ret < 0)
    return ret;

  /* XXX fix me; this checks/flushes the cache at the wrong time.  */
  if (as->dyn_generation != generation)
    {
      unw_flush_cache (as, 0, 0);
      as->dyn_generation = generation;
    }
  return 0;
}

HIDDEN int
unwi_find_dynamic_proc_info (unw_addr_space_t as, unw_word_t ip,
			     unw_proc_info_t *pi, int need_unwind_info,
			     void *arg)
{
#ifdef UNW_LOCAL_ONLY
  return local_find_proc_info (&_U_dyn_info_list,
			       as, ip, pi, need_unwind_info, arg);
#else
# ifdef UNW_REMOTE_ONLY
  return remote_find_proc_info (as, ip, pi, need_unwind_info, arg);
# else
  if (as == unw_local_addr_space)
    return local_find_proc_info ((unw_dyn_info_t *) _U_dyn_info_list_addr (),
				 as, ip, pi, need_unwind_info, arg);
  else
    return remote_find_proc_info (as, ip, pi, need_unwind_info, arg);
# endif
#endif
}
