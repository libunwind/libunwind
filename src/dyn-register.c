#include <libunwind.h>

#ifndef HAVE_CMP8XCHG16

#include <pthread.h>

/* Make it easy to write thread-safe code which may or may not be
   linked against libpthread.  The macros below can be used
   unconditionally and if -lpthread is around, they'll call the
   corresponding routines otherwise, they do nothing.  */

#pragma weak pthread_rwlock_rdlock
#pragma weak pthread_rwlock_wrlock
#pragma weak pthread_rwlock_unlock

#define rw_rdlock(l)    (pthread_rwlock_rdlock ? pthread_rwlock_rdlock (l) : 0)
#define rw_wrlock(l)    (pthread_rwlock_wrlock ? pthread_rwlock_wrlock (l) : 0)
#define rw_unlock(l)    (pthread_rwlock_unlock ? pthread_rwlock_unlock (l) : 0)

static pthread_rwlock_t registration_lock = PTHREAD_RWLOCK_INITIALIZER;

#endif

extern unw_dyn_info_list_t _U_dyn_info_list;

int
_U_dyn_register (unw_dyn_info_t *di)
{
#ifdef HAVE_CMP8XCHG16
  unw_dyn_info_t *old_list_head, *new_list_head;
  unsigned long old_gen, new_gen;

  do
    {
      old_gen = _U_dyn_info_list.generation;
      old_list_head = _U_dyn_info_list.first;

      new_gen = old_gen + 1;
      new_list_head = di;
      di->next = old_list_head;
    }
  while (cmp8xchg16 (&_U_dyn_info_list, new_gen, new_list_head) != old_gen);
#else
  rw_wrlock (&registration_lock);
  {
    ++_U_dyn_info_list.generation;
    di->next = _U_dyn_info_list.first;
    _U_dyn_info_list.first = di;
  }
  rw_unlock (&registration_lock);
#endif
}
