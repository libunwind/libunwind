SOVERSION=0:0:0		# See comments at end of file.

libunwind_la_SOURCES_common =					\
	backtrace.c get_proc_name.c				\
	dyn-cancel.c dyn-extract.c dyn-local.c dyn-register.c	\
	dyn-remote.c					 	\
	mempool.c mempool.h					\
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

libunwind_la_SOURCES_ia64 = $(libunwind_la_SOURCES_common)		  \
	ia64/init.h ia64/offsets.h ia64/regs.h ia64/rse.h		  \
	ia64/ucontext_i.h ia64/unwind_decoder.h ia64/unwind_i.h		  \
									  \
	ia64/__ia64_install_context.S ia64/tables.c			  \
	ia64/unw_regname.c						  \
									  \
	ia64/Ginit.c ia64/Gparser.c ia64/Gregs.c ia64/Gscript.c		  \
	ia64/Gcreate_addr_space.c ia64/Gdestroy_addr_space.c		  \
	ia64/Gset_caching_policy.c ia64/Gflush_cache.c			  \
	ia64/Gget_accessors.c ia64/Gget_proc_info.c ia64/Gget_proc_name.c \
	ia64/Gget_reg.c ia64/Gget_fpreg.c ia64/Gget_save_loc.c		  \
	ia64/Gis_signal_frame.c						  \
	ia64/Gresume.c ia64/Gset_reg.c ia64/Gset_fpreg.c		  \
	ia64/Ginit_local.c ia64/Ginit_remote.c ia64/Gstep.c		  \
									  \
	ia64/Linit.c ia64/Lparser.c ia64/Lregs.c ia64/Lscript.c		  \
	ia64/Lcreate_addr_space.c ia64/Ldestroy_addr_space.c		  \
	ia64/Lset_caching_policy.c ia64/Lflush_cache.c			  \
	ia64/Lget_accessors.c ia64/Lget_proc_info.c ia64/Lget_proc_name.c \
	ia64/Lget_reg.c ia64/Lget_fpreg.c ia64/Lget_save_loc.c		  \
	ia64/Lis_signal_frame.c						  \
	ia64/Lresume.c ia64/Lset_reg.c ia64/Lset_fpreg.c		  \
	ia64/Linit_local.c ia64/Linit_remote.c ia64/Lstep.c

if ARCH_IA64
 lib_LTLIBRARIES = libunwind-ia64.la
 libunwind_ia64_la_SOURCES = $(libunwind_la_SOURCES_ia64)
 libunwind_ia64_la_LDFLAGS = -version-info $(SOVERSION)
else
if ARCH_IA32
 lib_LTLIBRARIES = libunwind-ia32.la
 libunwind_ia32_la_SOURCES = $(libunwind_la_SOURCES_ia32)
 libunwind_ia64_la_LDFLAGS = -version-info $(SOVERSION)
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
