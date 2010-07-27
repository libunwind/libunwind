
# libunwind configuration


include(CheckIncludeFile)
include(CheckFunctionExists)
include(CheckSymbolExists)
include(CheckStructHasMember)
include(CheckTypeSize)


check_include_file(asm/ptrace_offsets.h HAVE_ASM_PTRACE_OFFSETS_H)
check_include_file(atomic_ops.h HAVE_ATOMIC_OPS_H)
check_include_file(dlfcn.h HAVE_DLFCN_H)
check_symbol_exists(dlmodinfo "dlfcn.h" HAVE_DLMODINFO)
check_function_exists(dl_iterate_phdr HAVE_DL_ITERATE_PHDR)
check_function_exists(dl_phdr_removals_counter HAVE_DL_PHDR_REMOVALS_COUNTER)
check_include_file(endian.h HAVE_ENDIAN_H)
check_include_file(execinfo.h HAVE_EXECINFO_H)
check_function_exists(getunwind HAVE_GETUNWIND)
check_include_file(ia64intrin.h HAVE_IA64INTRIN_H)
check_include_file(inttypes.h HAVE_INTTYPES_H)
check_include_file(memory.h HAVE_MEMORY_H)
check_include_file(stdint.h HAVE_STDINT_H)
check_include_file(stdlib.h HAVE_STDLIB_H)
check_include_file(strings.h HAVE_STRINGS_H)
check_include_file(string.h HAVE_STRING_H)
check_struct_has_member("struct dl_phdr_info" dlpi_subs link.h HAVE_STRUCT_DL_PHDR_INFO_DLPI_SUBS)
check_include_file(sys/stat.h HAVE_SYS_STAT_H)
check_include_file(sys/types.h HAVE_SYS_TYPES_H)
check_include_file(sys/uc_access.h HAVE_SYS_UC_ACCESS_H)
check_function_exists(ttrace HAVE_TTRACE)
check_include_file(unistd.h HAVE_UNISTD_H)


configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config.h.cmake.in
               ${CMAKE_CURRENT_BINARY_DIR}/include/config.h)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/libunwind-common.h.cmake.in
               ${CMAKE_CURRENT_BINARY_DIR}/include/libunwind-common.h)
