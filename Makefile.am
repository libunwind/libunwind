if ARCH_IA64
include_HEADERS_tdep = include/libunwind-ia64.h
else
if ARCH_HPPA
include_HEADERS_tdep = include/libunwind-hppa.h
else
if ARCH_X86
include_HEADERS_tdep = include/libunwind-x86.h
else
if ARCH_X86_64
include_HEADERS_tdep = include/libunwind-x86_64.h
endif # ARCH_X86_64
endif # ARCH_X86
endif # ARCH_HPPA
endif # ARCH_IA64

include_HEADERS_common = $(include_HEADERS_tdep) include/libunwind-common.h \
	include/libunwind-dynamic.h include/libunwind-ptrace.h

if REMOTE_ONLY
  include_HEADERS = $(include_HEADERS_common)
else
  include_HEADERS = $(include_HEADERS_common) include/libunwind.h \
		    include/unwind.h
endif

SUBDIRS = src tests doc

EXTRA_DIST = include/dwarf.h include/dwarf_i.h				\
	include/internal.h include/mempool.h				\
	include/remote.h						\
	include/libunwind-ia64.h include/tdep-ia64.h include/ia64/rse.h	\
	include/ia64/script.h						\
	include/libunwind-hppa.h include/tdep-hppa.h			\
	include/libunwind-x86.h include/tdep-x86.h			\
	include/x86/dwarf-config.h					\
	include/libunwind-x86_64.h include/tdep-x86_64.h		\
	include/x86/dwarf-config.h					\
	include/x86_64/dwarf-config.h

DISTCLEANFILES = include/libunwind.h include/tdep.h
