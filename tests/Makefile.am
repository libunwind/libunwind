AM_CPPFLAGS = -I$(top_srcdir)/include

noinst_PROGRAMS_common =

if REMOTE_ONLY
 TESTS_cdep =
 noinst_PROGRAMS_cdep = $(noinst_PROGRAMS_common)
else
if ARCH_IA64
 noinst_PROGRAMS_arch = ia64-test-sig
 TESTS_arch =	Gia64-test-stack Lia64-test-stack \
		Gia64-test-rbs Lia64-test-rbs
endif
 TESTS_cdep =	Gtest-exc Ltest-exc Gtest-resume-sig Ltest-resume-sig \
		Gtest-dyn1 Ltest-dyn1 test-setjmp test-ptrace
 noinst_PROGRAMS_cdep = bt test-init test-varargs
endif

TESTS = $(TESTS_arch) $(TESTS_cdep)

check_PROGRAMS = $(TESTS)

noinst_PROGRAMS = $(noinst_PROGRAMS_common) $(noinst_PROGRAMS_cdep) \
	$(noinst_PROGRAMS_arch)

Lia64_test_stack_SOURCES = Lia64-test-stack.c ia64-test-stack-asm.S
Gia64_test_stack_SOURCES = Gia64-test-stack.c ia64-test-stack-asm.S
Lia64_test_rbs_SOURCES = Lia64-test-rbs.c ia64-test-rbs-asm.S
Gia64_test_rbs_SOURCES = Gia64-test-rbs.c ia64-test-rbs-asm.S

LDADD = ../src/libunwind-$(arch).la

test_setjmp_LDADD = ../src/libunwind-setjmp.la
