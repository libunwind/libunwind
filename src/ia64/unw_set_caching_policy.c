#include "unwind_i.h"

int
unw_set_caching_policy (unw_caching_policy_t policy)
{
#ifndef HAVE___THREAD
  if (policy == UNW_CACHE_PER_THREAD)
    return -UNW_EINVAL;
#endif
  unw.caching_policy = policy;

  if (policy == UNW_CACHE_NONE)
    unw_flush_cache ();
}
