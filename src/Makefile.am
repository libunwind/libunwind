SOVERSION=3:0:2		# See comments at end of file.
SETJMP_SO_VERSION=0:0:0
#
# Don't link with standard libraries, because those may mention
# libunwind already.
#
COMMON_SO_LDFLAGS =	-XCClinker -nostartfiles	\
			-XCClinker -nostdlib		\
			$(LDFLAGS_STATIC_LIBCXA)

if REMOTE_ONLY
LIBRARIES_cdep			=
lib_LTLIBRARIES_cdep		=
else
LIBRARIES_cdep			= libunwind-ptrace.a
lib_LTLIBRARIES_cdep		= libunwind.la libunwind-setjmp.la
endif

### libunwind-ptrace:
libunwind_ptrace_a_SOURCES =						  \
	ptrace/_UPT_internal.h						  \
	ptrace/_UPT_accessors.c ptrace/_UPT_access_fpreg.c		  \
	ptrace/_UPT_access_mem.c ptrace/_UPT_access_reg.c		  \
	ptrace/_UPT_create.c ptrace/_UPT_destroy.c			  \
	ptrace/_UPT_find_proc_info.c ptrace/_UPT_get_dyn_info_list_addr.c \
	ptrace/_UPT_put_unwind_info.c ptrace/_UPT_get_proc_name.c	  \
	ptrace/_UPT_reg_offset.c ptrace/_UPT_resume.c

### libunwind-setjmp:
libunwind_setjmp_la_LDFLAGS		= $(COMMON_SO_LDFLAGS)		     \
					  -version-info $(SETJMP_SO_VERSION)
libunwind_setjmp_la_LIBADD		= libunwind-$(arch).la -lc
libunwind_setjmp_la_SOURCES_common	= longjmp.c siglongjmp.c
libunwind_setjmp_la_SOURCES_ia64	= ia64/setjmp-ia64.S		\
					  ia64/sigsetjmp-ia64.S		\
					  ia64/siglongjmp-ia64.S
libunwind_setjmp_la_SOURCES_hppa	= setjmp.c sigsetjmp.c		\
					  hppa/siglongjmp-hppa.S
libunwind_setjmp_la_SOURCES_x86		= setjmp.c sigsetjmp.c	\
					  x86/siglongjmp-x86.S

### libunwind:

# List of arch-independent files needed by both local-only and generic
# libraries:
libunwind_la_SOURCES_common =			\
	$(libunwind_la_SOURCES_os)		\
	mi/init.c mi/flush_cache.c mi/mempool.c

# List of arch-independent files needed by generic library (libunwind-$ARCH):
libunwind_la_SOURCES_generic =						\
	mi/Gdyn-extract.c mi/Gdyn-remote.c mi/Gfind_dynamic_proc_info.c	\
	mi/Gget_accessors.c						\
	mi/Gget_proc_info_by_ip.c mi/Gget_proc_name.c			\
	mi/Gput_dynamic_unwind_info.c mi/Gdestroy_addr_space.c		\
	mi/Gget_reg.c mi/Gset_reg.c					\
	mi/Gget_fpreg.c mi/Gset_fpreg.c					\
	mi/Gset_caching_policy.c

# List of arch-independent files needed by local-only library (libunwind):
libunwind_la_SOURCES_local =						\
	mi/backtrace.c							\
	mi/dyn-cancel.c mi/dyn-info-list.c mi/dyn-register.c		\
	mi/Ldyn-extract.c mi/Lfind_dynamic_proc_info.c			\
	mi/Lget_accessors.c						\
	mi/Lget_proc_info_by_ip.c mi/Lget_proc_name.c			\
	mi/Lput_dynamic_unwind_info.c mi/Ldestroy_addr_space.c		\
	mi/Lget_reg.c   mi/Lset_reg.c					\
	mi/Lget_fpreg.c mi/Lset_fpreg.c					\
	mi/Lset_caching_policy.c					\
	unwind/Backtrace.c unwind/DeleteException.c			\
	unwind/FindEnclosingFunction.c unwind/ForcedUnwind.c		\
	unwind/GetBSP.c unwind/GetCFA.c unwind/GetDataRelBase.c		\
	unwind/GetGR.c unwind/GetIP.c unwind/GetLanguageSpecificData.c	\
	unwind/GetRegionStart.c unwind/GetTextRelBase.c			\
	unwind/RaiseException.c unwind/Resume.c				\
	unwind/Resume_or_Rethrow.c unwind/SetGR.c unwind/SetIP.c

libunwind_la_SOURCES_os_linux = os-linux.h os-linux.c
libunwind_la_SOURCES_os_hpux = os-hpux.c

dwarf_SOURCES_common =				\
	dwarf/dwarf-eh.h			\
	dwarf/global-dwarf.c

dwarf_SOURCES_local =							\
	dwarf/Lexpr-dwarf.c dwarf/Lfde-dwarf.c dwarf/Lparser-dwarf.c	\
	dwarf/Lpe-dwarf.c dwarf/Lstep-dwarf.c

dwarf_SOURCES_generic =							\
	dwarf/Gexpr-dwarf.c dwarf/Gfde-dwarf.c dwarf/Gparser-dwarf.c	\
	dwarf/Gpe-dwarf.c dwarf/Gstep-dwarf.c

#
# Note: As of automake-1.6, there is a stupid limitation which requires
#	that each filename be unique.  This prevents us from having, e.g.,
#	ia64/tables.c and x86/tables.c.  The fix for now is to uglify the
#	filenames by appending the arch-name (e.g., tables-ia64.c).  Oh, well.
#

# The list of files that go both into libunwind and libunwind-ia64:
libunwind_la_SOURCES_ia64_common = $(libunwind_la_SOURCES_common)	    \
	ia64/init.h ia64/offsets.h ia64/regs.h ia64/rse.h		    \
	ia64/ucontext_i.h ia64/unwind_decoder.h ia64/unwind_i.h		    \
	ia64/regname-ia64.c

# The list of files that go into libunwind:
#  _ReadULEB()/_ReadSLEB() are needed for Intel C++ 8.0 compatibility
libunwind_la_SOURCES_ia64 = $(libunwind_la_SOURCES_ia64_common)		   \
	$(libunwind_la_SOURCES_local)					   \
									   \
	elf64.c elf64.h							   \
									   \
	mi/_ReadULEB.c mi/_ReadSLEB.c					   \
									   \
	ia64/dyn_info_list-ia64.S ia64/getcontext-ia64.S		   \
									   \
	ia64/Lcreate_addr_space-ia64.c					   \
	ia64/Lget_proc_info-ia64.c					   \
	ia64/Lget_save_loc-ia64.c					   \
	ia64/Lglobal-ia64.c						   \
	ia64/Linit-ia64.c ia64/Linit_local-ia64.c ia64/Linit_remote-ia64.c \
	ia64/Linstall_cursor-ia64.S ia64/Lis_signal_frame-ia64.c	   \
	ia64/Lparser-ia64.c ia64/Lrbs-ia64.c ia64/Lregs-ia64.c		   \
	ia64/Lresume-ia64.c ia64/Lscript-ia64.c				   \
	ia64/Lstep-ia64.c ia64/Ltables-ia64.c

# The list of files that go into libunwind-ia64:
libunwind_ia64_la_SOURCES_ia64 = $(libunwind_la_SOURCES_ia64_common)	   \
	$(libunwind_la_SOURCES_generic)					   \
	ia64/Gcreate_addr_space-ia64.c					   \
	ia64/Gget_proc_info-ia64.c					   \
	ia64/Gget_save_loc-ia64.c					   \
	ia64/Gglobal-ia64.c						   \
	ia64/Ginit-ia64.c ia64/Ginit_local-ia64.c ia64/Ginit_remote-ia64.c \
	ia64/Ginstall_cursor-ia64.S ia64/Gis_signal_frame-ia64.c	   \
	ia64/Gparser-ia64.c ia64/Grbs-ia64.c ia64/Gregs-ia64.c		   \
	ia64/Gresume-ia64.c ia64/Gscript-ia64.c				   \
	ia64/Gstep-ia64.c ia64/Gtables-ia64.c

libunwind_la_SOURCES_hppa = $(libunwind_la_SOURCES_common)		\
	hppa/global-hppa.c hppa/tables-hppa.c				\
	hppa/init.h hppa/unwind_i.h					\
									\
	hppa/Gget_reg-hppa.c						\
	hppa/Ginit-hppa.c hppa/Ginit_local-hppa.c			\
	hppa/Gget_proc_info-hppa.c hppa/Gregs-hppa.c hppa/Gstep-hppa.c	\
									\
	hppa/Lget_reg-hppa.c hppa/Lget_proc_name-hppa.c			\
	hppa/Linit-hppa.c hppa/Linit_local-hppa.c			\
	hppa/Lget_proc_info-hppa.c hppa/Lregs-hppa.c hppa/Lstep-hppa.c

# The list of files that go both into libunwind and libunwind-x86:
libunwind_la_SOURCES_x86_common = $(libunwind_la_SOURCES_common)	\
	$(dwarf_SOURCES_common)						\
	x86/init.h x86/offsets.h x86/unwind_i.h				\
	x86/is_fpreg-x86.c x86/regname-x86.c

# The list of files that go into libunwind:
libunwind_la_SOURCES_x86 = $(libunwind_la_SOURCES_x86_common)		\
	$(libunwind_la_SOURCES_local)					\
	elf32.c elf32.h							\
	$(dwarf_SOURCES_local)						\
	dwarf/Lfind_proc_info-lsb.c					\
	x86/Lcreate_addr_space-x86.c					\
	x86/Lget_save_loc-x86.c x86/Lglobal-x86.c x86/Linit-x86.c	\
	x86/Linit_local-x86.c x86/Linit_remote-x86.c			\
	x86/Lis_signal_frame-x86.c					\
	x86/Lget_proc_info-x86.c x86/Lregs-x86.c			\
	x86/Lresume-x86.c x86/Lstep-x86.c

# The list of files that go into libunwind-x86:
libunwind_x86_la_SOURCES_x86 = $(libunwind_la_SOURCES_x86_common)	\
	$(libunwind_la_SOURCES_generic)					\
	$(dwarf_SOURCES_generic)					\
	dwarf/Gfind_proc_info-lsb.c					\
	x86/Gcreate_addr_space-x86.c					\
	x86/Gget_save_loc-x86.c x86/Gglobal-x86.c x86/Ginit-x86.c	\
	x86/Ginit_local-x86.c x86/Ginit_remote-x86.c			\
	x86/Gis_signal_frame-x86.c					\
	x86/Gget_proc_info-x86.c x86/Gregs-x86.c			\
	x86/Gresume-x86.c x86/Gstep-x86.c

if REMOTE_ONLY
install-exec-hook:
#	Nothing to do here....
else
#
# This is not ideal, but I know of no other way to install an
# alias for a library.
#
install-exec-hook:
	$(LN_S) -f libunwind-$(arch).a  $(DESTDIR)$(libdir)/libunwind-generic.a
	$(LN_S) -f libunwind-$(arch).so \
		$(DESTDIR)$(libdir)/libunwind-generic.so
endif

if OS_LINUX
 libunwind_la_SOURCES_os = $(libunwind_la_SOURCES_os_linux)
endif

if OS_HPUX
 libunwind_la_SOURCES_os = $(libunwind_la_SOURCES_os_hpux)
endif

if ARCH_IA64
 mk_Gcursor_i_ia64_SOURCES = ia64/mk_Gcursor_i-ia64.c
 mk_Lcursor_i_ia64_SOURCES = ia64/mk_Lcursor_i-ia64.c
 noinst_PROGRAMS = mk_Gcursor_i-ia64 mk_Lcursor_i-ia64
 BUILT_SOURCES = Gcursor_i.h Lcursor_i.h
Gcursor_i.h: mk_Gcursor_i-ia64
	./mk_Gcursor_i-ia64 > $@
Lcursor_i.h: mk_Lcursor_i-ia64
	./mk_Lcursor_i-ia64 > $@
 lib_LTLIBRARIES_arch = libunwind-ia64.la
 libunwind_la_SOURCES = $(libunwind_la_SOURCES_ia64)
 libunwind_ia64_la_SOURCES = $(libunwind_ia64_la_SOURCES_ia64)
 libunwind_ia64_la_LDFLAGS = $(COMMON_SO_LDFLAGS) -version-info $(SOVERSION)
if !REMOTE_ONLY
 libunwind_ia64_la_LIBADD = libunwind.la -lc
endif
 libunwind_setjmp_la_SOURCES	= $(libunwind_setjmp_la_SOURCES_common) \
				  $(libunwind_setjmp_la_SOURCES_ia64)
else
if ARCH_HPPA
 lib_LTLIBRARIES_arch = libunwind-hppa.la
 libunwind_la_SOURCES = $(libunwind_la_SOURCES_hppa)
 libunwind_hppa_la_SOURCES = $(libunwind_hppa_la_SOURCES_hppa)
 libunwind_hppa_la_LDFLAGS = $(COMMON_SO_LDFLAGS) -version-info $(SOVERSION)
if !REMOTE_ONLY
 libunwind_hppa_la_LIBADD = libunwind.la -lc
endif
 libunwind_setjmp_la_SOURCES	= $(libunwind_setjmp_la_SOURCES_common) \
				  $(libunwind_setjmp_la_SOURCES_hppa)
else
if ARCH_X86
 lib_LTLIBRARIES_arch = libunwind-x86.la
 libunwind_la_SOURCES = $(libunwind_la_SOURCES_x86)
 libunwind_x86_la_SOURCES = $(libunwind_x86_la_SOURCES_x86)
 libunwind_x86_la_LDFLAGS = $(COMMON_SO_LDFLAGS) -version-info $(SOVERSION)
if !REMOTE_ONLY
 libunwind_x86_la_LIBADD = libunwind.la -lc
endif
 libunwind_setjmp_la_SOURCES	= $(libunwind_setjmp_la_SOURCES_common) \
				  $(libunwind_setjmp_la_SOURCES_x86)
endif # ARCH_X86
endif # ARCH_HPPA
endif # ARCH_IA64

libunwind_la_LDFLAGS =	$(COMMON_SO_LDFLAGS) -version-info $(SOVERSION)
libunwind_la_LIBADD  = -lc

lib_LIBRARIES = $(LIBRARIES_cdep)
lib_LTLIBRARIES = $(lib_LTLIBRARIES_cdep) $(lib_LTLIBRARIES_arch)

AM_CPPFLAGS = -I$(top_srcdir)/include/$(arch) -I$(top_srcdir)/include -I.
AM_CCASFLAGS = $(AM_CPPFLAGS)

EXTRA_DIST =	elfxx.h elfxx.c unwind-internal.h	\
		$(libunwind_la_SOURCES_hppa)		\
		$(libunwind_la_SOURCES_ia64)		\
		$(libunwind_la_SOURCES_x86)		\
		$(libunwind_la_SOURCES_os_linux)	\
		$(libunwind_la_SOURCES_os_hpux)		\
		$(libunwind_la_SOURCES_common)		\
		$(libunwind_la_SOURCES_local)		\
		$(libunwind_la_SOURCES_generic)		\
		$(libunwind_ptrace_a_SOURCES)		\
		$(libunwind_setjmp_la_SOURCES_common)	\
		$(libunwind_setjmp_la_SOURCES_hppa)	\
		$(libunwind_setjmp_la_SOURCES_ia64)	\
		$(libunwind_setjmp_la_SOURCES_x86)

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
