#include <stdio.h>

#include <libunwind.h>

#include "dyn-common.h"

int
unw_find_dynamic_proc_info (unw_addr_space_t as, unw_word_t ip,
			    unw_proc_info_t *pi, void *arg)
{
  extern unw_dyn_info_list_t _U_dyn_info_list;

#ifdef UNW_LOCAL_ONLY
  return find_proc_info (as, ip, pi, arg, &_U_dyn_info_list);
#else
# ifdef UNW_REMOTE_ONLY
  return remote_find_proc_info (as, ip, pi, arg);
# else
  if (as == unw_local_addr_space)
    return find_proc_info (as, ip, pi, arg, &_U_dyn_info_list);
  else
    return remote_find_proc_info (as, ip, pi, arg);
# endif
#endif
}
