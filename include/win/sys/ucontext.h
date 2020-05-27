// This is an incomplete & imprecice implementation of the *nix file
// by the same name


// Since this is only intended for VC++ compilers
// use #pragma once instead of guard macros
#pragma once

#ifdef _MSC_VER // Only for cross compilation to windows
#include <inttypes.h>

typedef struct ucontext
{
    uint8_t content[SIZEOF_UCONTENT];
} ucontext_t;

#ifdef __aarch64__
// These types are used in the definition of the aarch64 unw_tdep_context_t
// They are not used in UNW_REMOTE_ONLY, so typedef them as something
typedef long sigset_t;
typedef long stack_t;

// Windows SDK defines reserved. It conflicts with arm64 ucontext
// Undefine it
#undef __reserved
#endif

#endif // _MSC_VER
