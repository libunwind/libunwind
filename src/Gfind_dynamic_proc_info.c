#include <stdio.h>

#include "internal.h"

static inline int
local_find_proc_info (unw_addr_space_t as, unw_word_t ip,
		      unw_proc_info_t *pi,
		      int need_unwind_info, void *arg)
{
  unw_dyn_info_t *di;

  for (di = _U_dyn_info_list.first; di; di = di->next)
    if (ip >= di->start_ip && ip < di->end_ip)
      return unwi_extract_dynamic_proc_info (as, ip, pi, di, need_unwind_info,
					     arg);
  return -UNW_ENOINFO;
}

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

  /* Note: this can't go into dyn-remote.c because that file get's
     compiled exactly once (there are no separate local/general
     versions) and the call to unw_flush_cache() must evaluate to
     either the local or generic version.  */
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
  return local_find_proc_info (as, ip, pi, need_unwind_info, arg);
#else
# ifdef UNW_REMOTE_ONLY
  return remote_find_proc_info (as, ip, pi, need_unwind_info, arg);
# else
  if (as == unw_local_addr_space)
    return local_find_proc_info (as, ip, pi, need_unwind_info, arg);
  else
    return remote_find_proc_info (as, ip, pi, need_unwind_info, arg);
# endif
#endif
}
