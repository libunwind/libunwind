#include "internal.h"

pthread_mutex_t _U_dyn_info_list_lock = PTHREAD_MUTEX_INITIALIZER;

void
_U_dyn_register (unw_dyn_info_t *di)
{
  mutex_lock (&_U_dyn_info_list_lock);
  {
    ++_U_dyn_info_list.generation;

    di->next = _U_dyn_info_list.first;
    di->prev = NULL;
    if (di->next)
	    di->next->prev = di;
    _U_dyn_info_list.first = di;
  }
  mutex_unlock (&_U_dyn_info_list_lock);
}
