AM_CPPFLAGS = -I$(top_srcdir)/include

noinst_PROGRAMS_common =

if REMOTE_ONLY
 TESTS_cdep =
 noinst_PROGRAMS_cdep = $(noinst_PROGRAMS_common)
else
if ARCH_IA64
 noinst_PROGRAMS_arch = ia64-test-sig
 TESTS_arch = ia64-test-stack ia64-test-rbs
endif
 TESTS_cdep = test-dyn1 test-exc test-resume-sig test-setjmp test-ptrace
 noinst_PROGRAMS_cdep = bt test-init test-varargs
endif

TESTS = $(TESTS_arch) $(TESTS_cdep)

check_PROGRAMS = $(TESTS)

noinst_PROGRAMS = $(noinst_PROGRAMS_common) $(noinst_PROGRAMS_cdep) \
	$(noinst_PROGRAMS_arch)

ia64_test_stack_SOURCES = ia64-test-stack.c ia64-test-stack-asm.S
ia64_test_rbs_SOURCES = ia64-test-rbs.c ia64-test-rbs-asm.S

LDADD = ../src/libunwind-$(arch).la

test_setjmp_LDADD = ../src/libunwind-setjmp.la
