#include "unwind_i.h"

static const char *regname[] =
  {
    "eax", "ebx", "ecx", "edx", "esi", "edi", "ebp", "eip", "esp"
  };

const char *
unw_regname (unw_regnum_t reg)
{
  if (reg < NELEMS (regname))
    return regname[reg];
  else
    return "???";
}
