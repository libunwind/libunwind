#include "unwind_i.h"

HIDDEN int hppa_needs_initialization = 1;

#if UNW_DEBUG
HIDDEN int tdep_debug_level;
#endif

HIDDEN void
hppa_init (void)
{
  extern void _ULhppa_local_addr_space_init (void);

  mi_init();

  _Uhppa_local_addr_space_init ();
  _ULhppa_local_addr_space_init ();
}
