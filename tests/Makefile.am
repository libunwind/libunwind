AM_CPPFLAGS = -I$(top_srcdir)/include

noinst_PROGRAMS_common =

if REMOTE_ONLY
 noinst_PROGRAMS = $(noinst_PROGRAMS_common)
else
if ARCH_IA64
 noinst_PROGRAMS_arch = sig
endif
 noinst_PROGRAMS = $(noinst_PROGRAMS_common) $(noinst_PROGRAMS_arch) bt exc \
	test-init test-varargs test-dyn1 test-resume-sig
endif

LDADD = ../src/libunwind-$(arch).la
