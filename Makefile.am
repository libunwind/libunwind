if ARCH_ARM
include_HEADERS_tdep = include/libunwind-arm.h
else
if ARCH_IA64
include_HEADERS_tdep = include/libunwind-ia64.h
else
if ARCH_HPPA
include_HEADERS_tdep = include/libunwind-hppa.h
else
if ARCH_MIPS
include_HEADERS_tdep = include/libunwind-mips.h
else
if ARCH_X86
include_HEADERS_tdep = include/libunwind-x86.h
else
if ARCH_X86_64
include_HEADERS_tdep = include/libunwind-x86_64.h
else
if ARCH_PPC32
include_HEADERS_tdep = include/libunwind-ppc32.h
else
if ARCH_PPC64
include_HEADERS_tdep = include/libunwind-ppc64.h
endif # ARCH_PPC64
endif # ARCH_PPC32
endif # ARCH_X86_64
endif # ARCH_X86
endif # ARCH_MIPS
endif # ARCH_HPPA
endif # ARCH_IA64
endif # ARCH_ARM

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
	include/tdep-arm/dwarf-config.h					\
	include/tdep-arm/jmpbuf.h include/tdep-arm/libunwind_i.h	\
	include/tdep-ia64/jmpbuf.h include/tdep-ia64/rse.h 		\
	include/libunwind-ia64.h include/tdep-ia64/libunwind_i.h	\
	include/tdep-ia64/jmpbuf.h include/tdep-ia64/rse.h 		\
	include/tdep-ia64/script.h					\
	include/libunwind-hppa.h include/tdep-hppa/libunwind_i.h	\
	include/tdep-hppa/jmpbuf.h include/tdep-hppa/dwarf-config.h	\
	include/libunwind-x86.h include/tdep-x86/libunwind_i.h		\
	include/tdep-x86/jmpbuf.h include/tdep-x86/dwarf-config.h	\
	include/libunwind-x86_64.h include/tdep-x86_64/libunwind_i.h	\
	include/tdep-x86_64/jmpbuf.h include/tdep-x86_64/dwarf-config.h \
	include/libunwind-ppc64.h include/tdep-ppc64/dwarf-config.h     \
	include/tdep-ppc32/jmpbuf.h include/tdep-ppc32/libunwind_i.h	\
	include/tdep-ppc64/jmpbuf.h include/tdep-ppc64/libunwind_i.h	\
	include/libunwind.h include/tdep/libunwind_i.h			\
	include/tdep/jmpbuf.h include/tdep/dwarf-config.h
