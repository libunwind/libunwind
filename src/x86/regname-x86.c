#include "unwind_i.h"

static const char *regname[] =
  {
    "eax", "ebx", "ecx", "edx", "esi", "edi", "ebp", "eip", "esp"
  };

const char *
unw_regname (unw_regnum_t reg)
{
  if (reg < sizeof (regname) / sizeof (regname[0]))
    return regname[reg];
  else
    return "???";
}
