SOVERSION=7:0:0		# See comments at end of file.
SETJMP_SO_VERSION=0:0:0
#
# Don't link with start-files since we don't use any constructors/destructors:
#
COMMON_SO_LDFLAGS =	-XCClinker -nostartfiles

if REMOTE_ONLY
LIBRARIES_cdep			=
lib_LTLIBRARIES_cdep		=
lib_LTLIBRARIES_cdep_setjmp	=
else
LIBRARIES_cdep			= libunwind-ptrace.a
lib_LTLIBRARIES_cdep		= libunwind.la
lib_LTLIBRARIES_cdep_setjmp	= libunwind-setjmp.la
endif

### libunwind-ptrace:
libunwind_ptrace_a_SOURCES =						  \
	ptrace/_UPT_elf.c						  \
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
libunwind_setjmp_la_SOURCES_common	= setjmp/setjmp_i.h 	\
					  setjmp/longjmp.c	\
					  setjmp/siglongjmp.c
libunwind_setjmp_la_SOURCES_ia64	= ia64/setjmp.S  ia64/sigsetjmp.S  \
					  ia64/longjmp.S ia64/siglongjmp.S
libunwind_setjmp_la_SOURCES_hppa	= hppa/siglongjmp.S
libunwind_setjmp_la_SOURCES_x86		= x86/longjmp.S x86/siglongjmp.S
libunwind_setjmp_la_SOURCES_x86_64	= x86_64/longjmp.S x86_64/siglongjmp.S

### libunwind:

# List of arch-independent files needed by both local-only and generic
# libraries:
libunwind_la_SOURCES_common =					\
	$(libunwind_la_SOURCES_os)				\
	mi/init.c mi/flush_cache.c mi/mempool.c mi/strerror.c

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
	$(libunwind_la_SOURCES_os_local)				\
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

#  _ReadULEB()/_ReadSLEB() are needed for Intel C++ 8.0 compatibility
libunwind_la_SOURCES_os_linux_local = mi/_ReadULEB.c mi/_ReadSLEB.c
libunwind_la_SOURCES_os_hpux = os-hpux.c

dwarf_SOURCES_common =				\
	dwarf/global.c

dwarf_SOURCES_local =							     \
	dwarf/Lexpr.c dwarf/Lfde.c dwarf/Lparser.c dwarf/Lpe.c dwarf/Lstep.c

dwarf_SOURCES_generic =							     \
	dwarf/Gexpr.c dwarf/Gfde.c dwarf/Gparser.c dwarf/Gpe.c dwarf/Gstep.c

# The list of files that go both into libunwind and libunwind-ia64:
libunwind_la_SOURCES_ia64_common = $(libunwind_la_SOURCES_common)	    \
	elf64.c elf64.h							    \
	ia64/init.h ia64/offsets.h ia64/regs.h				    \
	ia64/ucontext_i.h ia64/unwind_decoder.h ia64/unwind_i.h		    \
	ia64/regname.c

# The list of files that go into libunwind:
libunwind_la_SOURCES_ia64 = $(libunwind_la_SOURCES_ia64_common)		     \
	$(libunwind_la_SOURCES_local)					     \
									     \
	ia64/dyn_info_list.S ia64/getcontext.S				     \
									     \
	ia64/Lcreate_addr_space.c ia64/Lget_proc_info.c ia64/Lget_save_loc.c \
	ia64/Lglobal.c ia64/Linit.c ia64/Linit_local.c ia64/Linit_remote.c   \
	ia64/Linstall_cursor.S ia64/Lis_signal_frame.c ia64/Lparser.c	     \
	ia64/Lrbs.c ia64/Lregs.c ia64/Lresume.c ia64/Lscript.c ia64/Lstep.c  \
	ia64/Ltables.c

# The list of files that go into libunwind-ia64:
libunwind_ia64_la_SOURCES_ia64 = $(libunwind_la_SOURCES_ia64_common)	     \
	$(libunwind_la_SOURCES_generic)					     \
	ia64/Gcreate_addr_space.c ia64/Gget_proc_info.c ia64/Gget_save_loc.c \
	ia64/Gglobal.c ia64/Ginit.c ia64/Ginit_local.c ia64/Ginit_remote.c   \
	ia64/Ginstall_cursor.S ia64/Gis_signal_frame.c ia64/Gparser.c	     \
	ia64/Grbs.c ia64/Gregs.c ia64/Gresume.c ia64/Gscript.c ia64/Gstep.c  \
	ia64/Gtables.c

# The list of files that go both into libunwind and libunwind-hppa:
libunwind_la_SOURCES_hppa_common = $(libunwind_la_SOURCES_common)	\
	$(dwarf_SOURCES_common)						\
	elf32.c elf32.h							\
	hppa/init.h hppa/offsets.h hppa/unwind_i.h			\
	hppa/regname.c

# The list of files that go into libunwind:
libunwind_la_SOURCES_hppa = $(libunwind_la_SOURCES_hppa_common)		\
	$(libunwind_la_SOURCES_local)					\
	hppa/getcontext.S hppa/setcontext.S				\
	$(dwarf_SOURCES_local)						\
	dwarf/Lfind_proc_info-lsb.c					\
	hppa/Lcreate_addr_space.c hppa/Lget_save_loc.c hppa/Lglobal.c	\
	hppa/Linit.c hppa/Linit_local.c hppa/Linit_remote.c		\
	hppa/Lis_signal_frame.c hppa/Lget_proc_info.c hppa/Lregs.c	\
	hppa/Lresume.c hppa/Lstep.c

# The list of files that go into libunwind-hppa:
libunwind_hppa_la_SOURCES_hppa = $(libunwind_la_SOURCES_hppa_common)	\
	$(libunwind_la_SOURCES_generic)					\
	$(dwarf_SOURCES_generic)					\
	dwarf/Gfind_proc_info-lsb.c					\
	hppa/Gcreate_addr_space.c hppa/Gget_save_loc.c hppa/Gglobal.c	\
	hppa/Ginit.c hppa/Ginit_local.c hppa/Ginit_remote.c		\
	hppa/Gis_signal_frame.c hppa/Gget_proc_info.c hppa/Gregs.c	\
	hppa/Gresume.c hppa/Gstep.c

# The list of files that go both into libunwind and libunwind-x86:
libunwind_la_SOURCES_x86_common = $(libunwind_la_SOURCES_common)	\
	$(dwarf_SOURCES_common)						\
	elf32.c elf32.h							\
	x86/init.h x86/offsets.h x86/unwind_i.h				\
	x86/is_fpreg.c x86/regname.c

# The list of files that go into libunwind:
libunwind_la_SOURCES_x86 = $(libunwind_la_SOURCES_x86_common)		\
	$(libunwind_la_SOURCES_local)					\
	$(dwarf_SOURCES_local)						\
	dwarf/Lfind_proc_info-lsb.c					\
	x86/Lcreate_addr_space.c x86/Lget_save_loc.c x86/Lglobal.c	\
	x86/Linit.c x86/Linit_local.c x86/Linit_remote.c		\
	x86/Lis_signal_frame.c x86/Lget_proc_info.c x86/Lregs.c		\
	x86/Lresume.c x86/Lstep.c

# The list of files that go into libunwind-x86:
libunwind_x86_la_SOURCES_x86 = $(libunwind_la_SOURCES_x86_common)	\
	$(libunwind_la_SOURCES_generic)					\
	$(dwarf_SOURCES_generic)					\
	dwarf/Gfind_proc_info-lsb.c					\
	x86/Gcreate_addr_space.c x86/Gget_save_loc.c x86/Gglobal.c	\
	x86/Ginit.c x86/Ginit_local.c x86/Ginit_remote.c		\
	x86/Gis_signal_frame.c x86/Gget_proc_info.c x86/Gregs.c		\
	x86/Gresume.c x86/Gstep.c

# The list of files that go both into libunwind and libunwind-x86_64:
libunwind_la_SOURCES_x86_64_common = $(libunwind_la_SOURCES_common)	\
	$(dwarf_SOURCES_common)						\
	elf64.c elf64.h							\
	x86_64/setcontext.S						\
	x86_64/init.h x86_64/unwind_i.h x86_64/ucontext_i.h		\
	x86_64/is_fpreg.c x86_64/regname.c

# The list of files that go into libunwind:
libunwind_la_SOURCES_x86_64 = $(libunwind_la_SOURCES_x86_64_common)	    \
	$(libunwind_la_SOURCES_local)					    \
	$(dwarf_SOURCES_local)						    \
	dwarf/Lfind_proc_info-lsb.c					    \
	x86_64/Lcreate_addr_space.c x86_64/Lget_save_loc.c x86_64/Lglobal.c \
	x86_64/Linit.c x86_64/Linit_local.c x86_64/Linit_remote.c	    \
	x86_64/Lis_signal_frame.c x86_64/Lget_proc_info.c x86_64/Lregs.c    \
	x86_64/Lresume.c x86_64/Lstep.c

# The list of files that go into libunwind:
libunwind_x86_64_la_SOURCES_x86_64 = $(libunwind_la_SOURCES_x86_64_common)  \
	$(libunwind_la_SOURCES_generic)					    \
	$(dwarf_SOURCES_generic)					    \
	dwarf/Gfind_proc_info-lsb.c					    \
	x86_64/Gcreate_addr_space.c x86_64/Gget_save_loc.c x86_64/Gglobal.c \
	x86_64/Ginit.c x86_64/Ginit_local.c x86_64/Ginit_remote.c	    \
	x86_64/Gis_signal_frame.c x86_64/Gget_proc_info.c x86_64/Gregs.c    \
	x86_64/Gresume.c x86_64/Gstep.c

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
 libunwind_la_SOURCES_os	= $(libunwind_la_SOURCES_os_linux)
 libunwind_la_SOURCES_os_local	= $(libunwind_la_SOURCES_os_linux_local)
endif

if OS_HPUX
 libunwind_la_SOURCES_os	= $(libunwind_la_SOURCES_os_hpux)
 libunwind_la_SOURCES_os_local	= $(libunwind_la_SOURCES_os_hpux_local)
endif

if ARCH_IA64
 ia64_mk_Gcursor_i_SOURCES = ia64/mk_Gcursor_i.c
 ia64_mk_Lcursor_i_SOURCES = ia64/mk_Lcursor_i.c
 noinst_PROGRAMS = ia64/mk_Gcursor_i ia64/mk_Lcursor_i
 BUILT_SOURCES = Gcursor_i.h Lcursor_i.h
Gcursor_i.h: ia64/mk_Gcursor_i
	ia64/mk_Gcursor_i > $@
Lcursor_i.h: ia64/mk_Lcursor_i
	ia64/mk_Lcursor_i > $@
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
else
if ARCH_X86_64
 lib_LTLIBRARIES_arch = libunwind-x86_64.la
 libunwind_la_SOURCES = $(libunwind_la_SOURCES_x86_64)
 libunwind_x86_64_la_SOURCES = $(libunwind_x86_64_la_SOURCES_x86_64)
 libunwind_x86_64_la_LDFLAGS = $(COMMON_SO_LDFLAGS) -version-info $(SOVERSION)
if !REMOTE_ONLY
 libunwind_x86_64_la_LIBADD = libunwind.la -lc
endif
 libunwind_setjmp_la_SOURCES	= $(libunwind_setjmp_la_SOURCES_common) \
				  $(libunwind_setjmp_la_SOURCES_x86_64)
endif # ARCH_X86_64
endif # ARCH_X86
endif # ARCH_HPPA
endif # ARCH_IA64

#
# Don't link with standard libraries, because those may mention
# libunwind already.
#
libunwind_la_LDFLAGS =	$(COMMON_SO_LDFLAGS) -XCClinker -nostdlib \
			$(LDFLAGS_STATIC_LIBCXA) -version-info $(SOVERSION)
libunwind_la_LIBADD  = -lc $(LIBCRTS)

lib_LIBRARIES = $(LIBRARIES_cdep)
lib_LTLIBRARIES = $(lib_LTLIBRARIES_cdep) $(lib_LTLIBRARIES_arch) \
		  $(lib_LTLIBRARIES_cdep_setjmp)

AM_CPPFLAGS = -I$(top_srcdir)/include -I$(top_srcdir)/include/tdep-$(arch) -I.
AM_CCASFLAGS = $(AM_CPPFLAGS)

EXTRA_DIST =	elfxx.h elfxx.c unwind/unwind-internal.h	\
		$(libunwind_la_SOURCES_hppa)			\
		$(libunwind_la_SOURCES_ia64)			\
		$(libunwind_la_SOURCES_x86)			\
		$(libunwind_la_SOURCES_os_linux)		\
		$(libunwind_la_SOURCES_os_hpux)			\
		$(libunwind_la_SOURCES_common)			\
		$(libunwind_la_SOURCES_local)			\
		$(libunwind_la_SOURCES_generic)			\
		$(libunwind_hppa_la_SOURCES_hppa)		\
		$(libunwind_ia64_la_SOURCES_ia64)		\
		$(libunwind_x86_la_SOURCES_x86)			\
		$(libunwind_x86_64_la_SOURCES_x86_64)		\
		$(libunwind_ptrace_a_SOURCES)			\
		$(libunwind_setjmp_la_SOURCES_common)		\
		$(libunwind_setjmp_la_SOURCES_hppa)		\
		$(libunwind_setjmp_la_SOURCES_ia64)		\
		$(libunwind_setjmp_la_SOURCES_x86)		\
		$(libunwind_setjmp_la_SOURCES_x86_64)

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
