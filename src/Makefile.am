AM_CPPFLAGS = -I$(top_srcdir)/include

lib_LTLIBRARIES = libunwind.la

libunwind_la_SOURCES_common = backtrace.c

if ARCH_IA64
libunwind_la_SOURCES = $(libunwind_la_SOURCES_common)			   \
	ia64/init.h ia64/offsets.h ia64/rse.h ia64/script.h		   \
	ia64/ucontext_i.h ia64/unwind_i.h ia64/unwind_decoder.h		   \
									   \
	ia64/__ia64_install_context.S ia64/shared.c ia64/tables-glibc.c	   \
									   \
	ia64/init.c ia64/parser.c ia64/regs.c ia64/script.c		   \
	ia64/unw_get_reg.c ia64/unw_get_fpreg.c ia64/unw_get_save_loc.c	   \
	ia64/unw_resume.c ia64/unw_set_reg.c ia64/unw_set_fpreg.c	   \
	ia64/unw_init_local.c ia64/unw_init_remote.c ia64/unw_step.c	   \
									   \
	ia64/Linit.c ia64/Lparser.c ia64/Lregs.c ia64/Lscript.c		   \
	ia64/Lunw_get_reg.c ia64/Lunw_get_fpreg.c ia64/Lunw_get_save_loc.c \
	ia64/Lunw_resume.c ia64/Lunw_set_reg.c ia64/Lunw_set_fpreg.c	   \
	ia64/Lunw_init_local.c ia64/Lunw_init_remote.c ia64/Lunw_step.c
else
if ARCH_x86
libunwind_la_SOURCES = $(libunwind_la_SOURCES_common)
endif
endif

libunwind_la_LDFLAGS = -version-info 0:0:0

# The -version-info flag accepts an argument of the form
# `current[:revision[:age]]'. So, passing `-version-info 3:12:1' sets
# current to 3, revision to 12, and age to 1.

# If either revision or age are omitted, they default to 0. Also note
# that age must be less than or equal to the current interface number.

# Here are a set of rules to help you update your library version
# information:

#    1. Start with version information of `0:0:0' for each libtool
#       library.

#    2. Update the version information only immediately before a public
#       release of your software. More frequent updates are unnecessary,
#       and only guarantee that the current interface number gets larger
#       faster.

#    3. If the library source code has changed at all since the last
#       update, then increment revision (`c:r:a' becomes `c:r+1:a').

#    4. If any interfaces have been added, removed, or changed since the
#       last update, increment current, and set revision to 0.

#    5. If any interfaces have been added since the last public release,
#       then increment age.

#    6. If any interfaces have been removed since the last public
#       release, then set age to 0.
