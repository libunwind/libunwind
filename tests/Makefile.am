AM_CPPFLAGS = -I$(top_srcdir)/include

noinst_PROGRAMS_common =

if REMOTE_ONLY
 noinst_PROGRAMS = $(noinst_PROGRAMS_common)
else
if ARCH_IA64
 noinst_PROGRAMS_arch = ia64-test-sig ia64-test-stack
endif
 noinst_PROGRAMS = $(noinst_PROGRAMS_common) $(noinst_PROGRAMS_arch) bt exc \
	test-init test-varargs test-dyn1 test-resume-sig
endif

ia64_test_stack_SOURCES = ia64-test-stack.c ia64-test-stack-asm.S

LDADD = ../src/libunwind-$(arch).la
