if ARCH_IA64
include_HEADERS_tdep = include/libunwind-ia64.h
else
if ARCH_IA32
include_HEADERS_tdep = include/libunwind-ia32.h
endif
endif

include_HEADERS_common = $(include_HEADERS_tdep) include/libunwind-common.h

if REMOTE_ONLY
  include_HEADERS = $(include_HEADERS_common)
else
  include_HEADERS = $(include_HEADERS_common) include/libunwind.h
endif

SUBDIRS = src tests doc

EXTRA_DIST = include/libunwind-ia64.h
