#include "unwind_i.h"

HIDDEN int x86_needs_initialization = 1;

#if UNW_DEBUG
HIDDEN int tdep_debug_level;
#endif

HIDDEN void
x86_init (void)
{
  extern void _ULx86_local_addr_space_init (void);

  mi_init();

  _Ux86_local_addr_space_init ();
  _ULx86_local_addr_space_init ();
}
