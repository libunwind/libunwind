AM_CPPFLAGS = -I$(top_srcdir)/include

noinst_PROGRAMS_common =

if REMOTE_ONLY
 TESTS_cdep =
 noinst_PROGRAMS_cdep = $(noinst_PROGRAMS_common)
else
if ARCH_IA64
 noinst_PROGRAMS_arch = ia64-test-sig
 TESTS_arch = ia64-test-stack
endif
 TESTS_cdep = test-dyn1 test-ptrace
 noinst_PROGRAMS_cdep = bt exc test-init test-varargs test-dyn1 test-resume-sig
endif

TESTS = $(TESTS_cdep) $(TESTS_arch)

check_PROGRAMS = $(TESTS)

noinst_PROGRAMS = $(noinst_PROGRAMS_common) $(noinst_PROGRAMS_cdep) \
	$(noinst_PROGRAMS_arch)

ia64_test_stack_SOURCES = ia64-test-stack.c ia64-test-stack-asm.S

LDADD = ../src/libunwind-$(arch).la
