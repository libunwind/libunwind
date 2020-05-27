// This is an incomplete & imprecice implementation of the Posix
// standard file by the same name


// Since this is only intended for VC++ compilers
// use #pragma once instead of guard macros
#pragma once

#ifdef _MSC_VER // Only for cross compilation to windows

// Posix is a superset of the ISO C signal.h
// include ISO C version first
#include <../ucrt/signal.h>
#include <sys/types.h>
#include <sys/ucontext.h>

typedef struct siginfo
{
    uint8_t content[SIZEOF_SIGINFO];
} siginfo_t;

typedef long sigset_t;

int          sigfillset(sigset_t *set);

#endif // _MSC_VER
