if ARCH_IA64
include_HEADERS_tdep = include/libunwind-ia64.h
else
if ARCH_X86
include_HEADERS_tdep = include/libunwind-x86.h
endif
endif

include_HEADERS_common = $(include_HEADERS_tdep) include/libunwind-common.h \
	include/libunwind-dynamic.h

if REMOTE_ONLY
  include_HEADERS = $(include_HEADERS_common)
else
  include_HEADERS = $(include_HEADERS_common) include/libunwind.h
endif

SUBDIRS = src tests doc

EXTRA_DIST = include/internal.h include/mempool.h \
	include/libunwind-ia64.h include/tdep-ia64.h include/ia64/script.h
