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

include_HEADERS_common = $(include_HEADERS_tdep)		\
	include/libunwind-dynamic.h include/libunwind-ptrace.h

if REMOTE_ONLY
  include_HEADERS = $(include_HEADERS_common)
else
  include_HEADERS = $(include_HEADERS_common) include/libunwind.h \
		    include/unwind.h
endif

nodist_include_HEADERS = include/libunwind-common.h

SUBDIRS = src tests doc

EXTRA_DIST = include/dwarf.h include/dwarf_i.h include/dwarf-eh.h	\
	include/libunwind_i.h include/mempool.h				\
	include/remote.h						\
	include/libunwind-common.h.in					\
	include/libunwind-ia64.h include/tdep-ia64/libunwind_i.h	\
	include/tdep-ia64/jmpbuf.h include/tdep-ia64/rse.h 		\
	include/tdep-ia64/script.h					\
	include/libunwind-hppa.h include/tdep-hppa/libunwind_i.h	\
	include/tdep-hppa/jmpbuf.h include/tdep-hppa/dwarf-config.h	\
	include/libunwind-x86.h include/tdep-x86/libunwind_i.h		\
	include/tdep-x86/jmpbuf.h include/tdep-x86/dwarf-config.h	\
	include/libunwind-x86_64.h include/tdep-x86_64/libunwind_i.h	\
	include/tdep-x86_64/jmpbuf.h include/tdep-x86_64/dwarf-config.h

DISTCLEANFILES = include/libunwind.h include/tdep
