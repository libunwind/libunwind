AM_CPPFLAGS = -I$(top_srcdir)/include

EXTRA_DIST = run-ia64-test-dyn1

noinst_PROGRAMS_common =
check_PROGRAMS_common =
check_SCRIPTS_common =

if REMOTE_ONLY
 check_SCRIPTS_cdep =
 check_PROGRAMS_cdep =
 noinst_PROGRAMS_cdep = $(noinst_PROGRAMS_common)
else
if ARCH_IA64
 noinst_PROGRAMS_arch = ia64-test-sig ia64-test-dyn1
 check_SCRIPTS_arch =	run-ia64-test-dyn1
 check_PROGRAMS_arch =	Gia64-test-stack Lia64-test-stack	\
			Gia64-test-rbs Lia64-test-rbs
endif
 check_SCRIPTS_cdep =
 check_PROGRAMS_cdep =	Gtest-exc Ltest-exc Gtest-resume-sig Ltest-resume-sig \
			Gtest-dyn1 Ltest-dyn1 test-setjmp test-ptrace
 noinst_PROGRAMS_cdep = bt test-init test-varargs
endif

check_PROGRAMS = $(check_PROGRAMS_common) $(check_PROGRAMS_cdep) \
		 $(check_PROGRAMS_arch)
check_SCRIPTS =	$(check_SCRIPTS_common) $(check_SCRIPTS_cdep) \
		$(check_SCRIPTS_arch)


TESTS = $(check_PROGRAMS) $(check_SCRIPTS)

noinst_PROGRAMS = $(noinst_PROGRAMS_common) $(noinst_PROGRAMS_cdep) \
	$(noinst_PROGRAMS_arch)

Lia64_test_stack_SOURCES = Lia64-test-stack.c ia64-test-stack-asm.S \
				ia64-test-stack.h
Gia64_test_stack_SOURCES = Gia64-test-stack.c ia64-test-stack-asm.S \
				ia64-test-stack.h
Lia64_test_rbs_SOURCES = Lia64-test-rbs.c ia64-test-rbs-asm.S ia64-test-rbs.h
Gia64_test_rbs_SOURCES = Gia64-test-rbs.c ia64-test-rbs-asm.S ia64-test-rbs.h
ia64_test_dyn1_SOURCES = ia64-test-dyn1.c ia64-dyn-asm.S

LDADD = ../src/libunwind-$(arch).la

test_setjmp_LDADD = ../src/libunwind-setjmp.la
