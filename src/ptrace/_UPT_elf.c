/* We need to get a separate copy of the ELF-code into
   libunwind-ptrace since it cannot (and must not) have any ELF
   dependencies on libunwind.  */
#include "tdep.h"	/* get ELFCLASS defined */
#include "../elf64.c"
