SOVERSION=2:0:1		# See comments at end of file.
SETJMP_SO_VERSION=0:0:0

libunwind_setjmp_la_SOURCES_local = longjmp.c siglongjmp.c
libunwind_setjmp_la_SOURCES_local_ia64 = ia64/setjmp-ia64.S	\
	ia64/sigsetjmp-ia64.S ia64/siglongjmp-ia64.S
libunwind_setjmp_la_SOURCES_local_hppa = setjmp.c sigsetjmp.c	\
	hppa/siglongjmp-hppa.S
libunwind_setjmp_la_SOURCES_local_x86 = setjmp.c sigsetjmp.c	\
	x86/siglongjmp-x86.S

libunwind_la_SOURCES_common = $(libunwind_la_SOURCES_cdep)		      \
	$(libunwind_la_SOURCES_os)					      \
	backtrace.c							      \
	dyn-extract.c dyn-remote.c					      \
	mi-init.c							      \
	mempool.c							      \
	Gfind_dynamic_proc_info.c Gget_proc_name.c Gput_dynamic_unwind_info.c \
	Lfind_dynamic_proc_info.c Lget_proc_name.c Lput_dynamic_unwind_info.c \
	Gget_proc_info_by_ip.c						      \
	Lget_proc_info_by_ip.c

libunwind_la_SOURCES_local =						     \
	_UPT_internal.h							     \
	dyn-cancel.c dyn-register.c					     \
	_UPT_accessors.c _UPT_access_fpreg.c _UPT_access_mem.c		     \
	_UPT_access_reg.c _UPT_create.c _UPT_destroy.c _UPT_find_proc_info.c \
	_UPT_get_dyn_info_list_addr.c _UPT_put_unwind_info.c		     \
	_UPT_get_proc_name.c _UPT_reg_offset.c _UPT_resume.c		     \
									     \
	_Unwind_Backtrace.c _Unwind_DeleteException.c			     \
	_Unwind_FindEnclosingFunction.c _Unwind_ForcedUnwind.c		     \
	_Unwind_GetBSP.c _Unwind_GetCFA.c _Unwind_GetDataRelBase.c	     \
	_Unwind_GetGR.c _Unwind_GetIP.c _Unwind_GetLanguageSpecificData.c    \
	_Unwind_GetRegionStart.c _Unwind_GetTextRelBase.c		     \
	_Unwind_RaiseException.c _Unwind_Resume.c			     \
	_Unwind_Resume_or_Rethrow.c _Unwind_SetGR.c _Unwind_SetIP.c

libunwind_la_SOURCES_os_linux = os-linux.h os-linux.c
libunwind_la_SOURCES_os_hpux = os-hpux.c

#
# Note: As of automake-1.6, there is a stupid limitation which requires
#	that each filename be unique.  This prevents us from having, e.g.,
#	ia64/tables.c and x86/tables.c.  The fix for now is to uglify the
#	filenames by appending the arch-name (e.g., tables-ia64.c).  Oh, well.
#

libunwind_la_SOURCES_ia64 = $(libunwind_la_SOURCES_common)		      \
	ia64/init.h ia64/offsets.h ia64/regs.h ia64/rse.h		      \
	ia64/ucontext_i.h ia64/unwind_decoder.h ia64/unwind_i.h		      \
									      \
	elf64.c elf64.h							      \
	ia64/global-ia64.c ia64/global-asm-ia64.S			      \
	ia64/tables-ia64.c ia64/get_accessors-ia64.c			      \
	ia64/flush_cache-ia64.c ia64/set_caching_policy-ia64.c		      \
	ia64/regname-ia64.c						      \
									      \
	ia64/Ginit-ia64.c ia64/Ginstall_cursor.S			      \
	ia64/Gparser-ia64.c ia64/Gregs-ia64.c				      \
	ia64/Grbs-ia64.c ia64/Gscript-ia64.c ia64/Gcreate_addr_space-ia64.c   \
	ia64/Gdestroy_addr_space-ia64.c					      \
	ia64/Gget_proc_info-ia64.c ia64/Gget_proc_name-ia64.c		      \
	ia64/Gget_reg-ia64.c ia64/Gget_fpreg-ia64.c ia64/Gget_save_loc-ia64.c \
	ia64/Gis_signal_frame-ia64.c					      \
	ia64/Gresume-ia64.c ia64/Gset_reg-ia64.c ia64/Gset_fpreg-ia64.c	      \
	ia64/Ginit_local-ia64.c ia64/Ginit_remote-ia64.c ia64/Gstep-ia64.c    \
									      \
	ia64/Linit-ia64.c ia64/Linstall_cursor.S			      \
	ia64/Lparser-ia64.c ia64/Lregs-ia64.c				      \
	ia64/Lrbs-ia64.c ia64/Lscript-ia64.c ia64/Lcreate_addr_space-ia64.c   \
	ia64/Ldestroy_addr_space-ia64.c					      \
	ia64/Lget_proc_info-ia64.c ia64/Lget_proc_name-ia64.c		      \
	ia64/Lget_reg-ia64.c ia64/Lget_fpreg-ia64.c ia64/Lget_save_loc-ia64.c \
	ia64/Lis_signal_frame-ia64.c					      \
	ia64/Lresume-ia64.c ia64/Lset_reg-ia64.c ia64/Lset_fpreg-ia64.c	      \
	ia64/Linit_local-ia64.c ia64/Linit_remote-ia64.c ia64/Lstep-ia64.c

libunwind_la_SOURCES_hppa = $(libunwind_la_SOURCES_common)		\
	hppa/flush_cache-hppa.c hppa/get_accessors-hppa.c		\
	hppa/global-hppa.c hppa/tables-hppa.c				\
	hppa/init.h hppa/unwind_i.h					\
									\
	hppa/Gget_reg-hppa.c hppa/Gget_proc_name-hppa.c			\
	hppa/Ginit-hppa.c hppa/Ginit_local-hppa.c			\
	hppa/Gget_proc_info-hppa.c hppa/Gregs-hppa.c hppa/Gstep-hppa.c	\
									\
	hppa/Lget_reg-hppa.c hppa/Lget_proc_name-hppa.c			\
	hppa/Linit-hppa.c hppa/Linit_local-hppa.c			\
	hppa/Lget_proc_info-hppa.c hppa/Lregs-hppa.c hppa/Lstep-hppa.c

libunwind_la_SOURCES_x86 = $(libunwind_la_SOURCES_common)		\
	x86/offsets.h							\
									\
	elf32.c elf32.h							\
									\
	x86/flush_cache-x86.c x86/get_accessors-x86.c x86/global-x86.c	\
	x86/tables-x86.c x86/set_caching_policy-x86.c			\
	x86/init.h x86/unwind_i.h x86/regname-x86.c			\
									\
	x86/Gcreate_addr_space-x86.c					\
	x86/Gget_reg-x86.c x86/Gget_proc_name-x86.c x86/Ginit-x86.c	\
	x86/Ginit_local-x86.c x86/Ginit_remote-x86.c			\
	x86/Gis_signal_frame-x86.c					\
	x86/Gget_proc_info-x86.c x86/Gregs-x86.c			\
	x86/Gresume-x86.c x86/Gset_reg-x86.c x86/Gstep-x86.c		\
									\
	x86/Lcreate_addr_space-x86.c					\
	x86/Lget_reg-x86.c x86/Lget_proc_name-x86.c x86/Linit-x86.c	\
	x86/Linit_local-x86.c x86/Linit_remote-x86.c			\
	x86/Lis_signal_frame-x86.c					\
	x86/Lget_proc_info-x86.c x86/Lregs-x86.c			\
	x86/Lresume-x86.c x86/Lset_reg-x86.c x86/Lstep-x86.c

if REMOTE_ONLY
libunwind_la_SOURCES_cdep = $(libunwind_la_SOURCES_remote)
install-exec-hook:
#	Nothing to do here....
else
libunwind_la_SOURCES_cdep = $(libunwind_la_SOURCES_local)

lib_LTLIBRARIES_cdep		= libunwind-setjmp.la
libunwind_setjmp_la_SOURCES_common = $(libunwind_setjmp_la_SOURCES_local)
libunwind_setjmp_la_LDFLAGS	= -version-info $(SETJMP_SO_VERSION)
libunwind_setjmp_la_LIBADD	= libunwind-$(arch).la

libunwind_setjmp_la_SOURCES_ia64 = $(libunwind_setjmp_la_SOURCES_local_ia64)
libunwind_setjmp_la_SOURCES_hppa = $(libunwind_setjmp_la_SOURCES_local_hppa)
libunwind_setjmp_la_SOURCES_x86	 = $(libunwind_setjmp_la_SOURCES_local_x86)
#
# This is not ideal, but I know of no other way to install an
# alias for a library.
#
install-exec-hook:
	$(LN_S) -f libunwind-$(arch).a  $(DESTDIR)$(libdir)/libunwind.a
	$(LN_S) -f libunwind-$(arch).so $(DESTDIR)$(libdir)/libunwind.so

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
 libunwind_ia64_la_SOURCES = $(libunwind_la_SOURCES_ia64)
 libunwind_ia64_la_LDFLAGS = -version-info $(SOVERSION)
 libunwind_setjmp_la_SOURCES	= $(libunwind_setjmp_la_SOURCES_common) \
				  $(libunwind_setjmp_la_SOURCES_ia64)
else
if ARCH_HPPA
 lib_LTLIBRARIES_arch = libunwind-hppa.la
 libunwind_hppa_la_SOURCES = $(libunwind_la_SOURCES_hppa)
 libunwind_hppa_la_LDFLAGS = -version-info $(SOVERSION)
 libunwind_setjmp_la_SOURCES	= $(libunwind_setjmp_la_SOURCES_common) \
				  $(libunwind_setjmp_la_SOURCES_hppa)
else
if ARCH_X86
 lib_LTLIBRARIES_arch = libunwind-x86.la
 libunwind_x86_la_SOURCES = $(libunwind_la_SOURCES_x86)
 libunwind_x86_la_LDFLAGS = -version-info $(SOVERSION)
 libunwind_setjmp_la_SOURCES	= $(libunwind_setjmp_la_SOURCES_common) \
				  $(libunwind_setjmp_la_SOURCES_x86)
endif # ARCH_X86
endif # ARCH_HPPA
endif # ARCH_IA64

lib_LTLIBRARIES = $(lib_LTLIBRARIES_cdep) $(lib_LTLIBRARIES_arch)

AM_CPPFLAGS = -I$(top_srcdir)/include -I.
AM_CCASFLAGS = $(AM_CPPFLAGS)

EXTRA_DIST =	elfxx.h elfxx.c unwind-internal.h		\
		$(libunwind_la_SOURCES_hppa)			\
		$(libunwind_la_SOURCES_ia64)			\
		$(libunwind_la_SOURCES_x86)			\
		$(libunwind_la_SOURCES_os_linux)		\
		$(libunwind_la_SOURCES_os_hpux)			\
		$(libunwind_la_SOURCES_local)			\
		$(libunwind_la_SOURCES_remote)			\
		$(libunwind_setjmp_la_SOURCES_local)		\
		$(libunwind_setjmp_la_SOURCES_local_hppa)	\
		$(libunwind_setjmp_la_SOURCES_local_ia64)	\
		$(libunwind_setjmp_la_SOURCES_local_x86)

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
