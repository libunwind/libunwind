#include <stdio.h>
#include <stdlib.h>

/*
 * Run the backtrace test using the ARM DWARF unwinder based on the
 * .debug_frame section.
 */
__attribute__((constructor))
void set_dwarf(void) {
	setenv("UNW_ARM_UNWIND_METHOD", "1", 1);
}

#include "Gtest-bt.c"
