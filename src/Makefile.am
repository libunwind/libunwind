SOVERSION=0:0:0		# See comments at end of file.

libunwind_la_SOURCES_common =					\
	backtrace.c get_proc_name.c				\
	dyn-cancel.c dyn-extract.c dyn-register.c		\
	dyn-remote.c					 	\
	mi-init.c						\
	mempool.c						\
	Gfind_dynamic_proc_info.c Gput_dynamic_unwind_info.c	\
	Lfind_dynamic_proc_info.c Lput_dynamic_unwind_info.c

if REMOTE_ONLY
# Nothing special.
else
#
# This is not ideal, but I know of no other way to install an
# alias for a library.
#
install-exec-hook:
	$(LN_S) -f libunwind-$(arch).a  $(DESTDIR)$(libdir)/libunwind.a
	$(LN_S) -f libunwind-$(arch).so $(DESTDIR)$(libdir)/libunwind.so

endif

#
# Note: As of automake-1.6, there is a stupid limitation which requires
#	that each filename is unique.  This prevents us from having, e.g.,
#	ia64/tables.c and x86/tables.c.  The fix for now is to uglify the
#	filenames by appending the arch-name (e.g., tables-ia64.c).  Oh, well.
#

libunwind_la_SOURCES_ia64 = $(libunwind_la_SOURCES_common)		      \
	ia64/ia64-init.h ia64/offsets.h ia64/regs.h ia64/rse.h		      \
	ia64/ucontext_i.h ia64/unwind_decoder.h ia64/unwind_i.h		      \
									      \
	ia64/__ia64_install_context.S ia64/global-ia64.c ia64/tables-ia64.c   \
	ia64/get_accessors-ia64.c					      \
	ia64/flush_cache-ia64.c ia64/set_caching_policy-ia64.c		      \
	ia64/regname-ia64.c						      \
									      \
	ia64/Ginit-ia64.c ia64/Gparser-ia64.c ia64/Gregs-ia64.c		      \
	ia64/Gscript-ia64.c ia64/Gcreate_addr_space-ia64.c		      \
	ia64/Gdestroy_addr_space-ia64.c					      \
	ia64/Gget_proc_info-ia64.c ia64/Gget_proc_name-ia64.c		      \
	ia64/Gget_reg-ia64.c ia64/Gget_fpreg-ia64.c ia64/Gget_save_loc-ia64.c \
	ia64/Gis_signal_frame-ia64.c					      \
	ia64/Gresume-ia64.c ia64/Gset_reg-ia64.c ia64/Gset_fpreg-ia64.c	      \
	ia64/Ginit_local-ia64.c ia64/Ginit_remote-ia64.c ia64/Gstep-ia64.c    \
									      \
	ia64/Linit-ia64.c ia64/Lparser-ia64.c ia64/Lregs-ia64.c		      \
	ia64/Lscript-ia64.c ia64/Lcreate_addr_space-ia64.c		      \
	ia64/Ldestroy_addr_space-ia64.c					      \
	ia64/Lget_proc_info-ia64.c ia64/Lget_proc_name-ia64.c		      \
	ia64/Lget_reg-ia64.c ia64/Lget_fpreg-ia64.c ia64/Lget_save_loc-ia64.c \
	ia64/Lis_signal_frame-ia64.c					      \
	ia64/Lresume-ia64.c ia64/Lset_reg-ia64.c ia64/Lset_fpreg-ia64.c	      \
	ia64/Linit_local-ia64.c ia64/Linit_remote-ia64.c ia64/Lstep-ia64.c

if ARCH_IA64
 lib_LTLIBRARIES = libunwind-ia64.la
 libunwind_ia64_la_SOURCES = $(libunwind_la_SOURCES_ia64)
 libunwind_ia64_la_LDFLAGS = -version-info $(SOVERSION)
else
if ARCH_X86

libunwind_la_SOURCES_x86 = $(libunwind_la_SOURCES_common)		   \
	x86/tables-x86.c x86/global-x86.c				   \
									   \
	x86/Gflush_cache-x86.c x86/Gget_accessors-x86.c x86/Gget_reg-x86.c \
	x86/Gget_proc_name.c x86/Ginit-x86.c x86/Ginit_local-x86.c	   \
	x86/Gget_proc_info.c x86/Gregs.c				   \
	x86/Gstep-x86.c							   \
									   \
	x86/Lflush_cache-x86.c x86/Lget_accessors-x86.c x86/Lget_reg-x86.c \
	x86/Lget_proc_name.c x86/Linit-x86.c x86/Linit_local-x86.c	   \
	x86/Lget_proc_info.c x86/Gregs.c				   \
	x86/Lstep-x86.c

 lib_LTLIBRARIES = libunwind-x86.la
 libunwind_x86_la_SOURCES = $(libunwind_la_SOURCES_x86)
 libunwind_x86_la_LDFLAGS = -version-info $(SOVERSION)
endif
endif

# XXX Need to create symlinks from libunwind-ia64.so to libunwind.so
# XXX end libunwind-ia64.a to libunwind.a

AM_CPPFLAGS = -I$(top_srcdir)/include

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
