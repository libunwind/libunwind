AM_CPPFLAGS = -I$(top_srcdir)/include

EXTRA_DIST =	run-ia64-test-dyn1 run-ptrace-mapper		\
		run-check-namespace check-namespace.sh.in

noinst_PROGRAMS_common =
check_PROGRAMS_common = test-proc-info test-static-link \
			test-strerror
check_SCRIPTS_common =	run-check-namespace

if REMOTE_ONLY
 check_SCRIPTS_cdep =
 check_PROGRAMS_cdep =
 noinst_PROGRAMS_cdep = $(noinst_PROGRAMS_common)

perf:

else
if ARCH_IA64
 noinst_PROGRAMS_arch = ia64-test-sig ia64-test-dyn1
 check_SCRIPTS_arch =	run-ia64-test-dyn1
 check_PROGRAMS_arch =	Gia64-test-stack Lia64-test-stack		\
			Gia64-test-nat Lia64-test-nat			\
			Gia64-test-rbs Lia64-test-rbs			\
			Gia64-test-readonly Lia64-test-readonly
endif
 check_SCRIPTS_cdep =	run-ptrace-mapper
 check_PROGRAMS_cdep =	Gtest-exc Ltest-exc Gtest-init Ltest-init	  \
			Gtest-concurrent Ltest-concurrent		  \
			Gtest-resume-sig Ltest-resume-sig		  \
			Gtest-dyn1 Ltest-dyn1				  \
			test-async-sig test-flush-cache test-init-remote  \
			test-mem test-setjmp test-ptrace
 noinst_PROGRAMS_cdep = bt forker mapper test-varargs Gperf-simple Lperf-simple

perf: perf-startup Gperf-simple Lperf-simple
	@echo "########## Basic performance of generic libunwind:"
	@./Gperf-simple
	@echo "########## Basic performance of local-only libunwind:"
	@./Lperf-simple
	@echo "########## Startup overhead:"
	@$(srcdir)/perf-startup @arch@

endif

check_PROGRAMS = $(check_PROGRAMS_common) $(check_PROGRAMS_cdep) \
		 $(check_PROGRAMS_arch)
check_SCRIPTS =	$(check_SCRIPTS_common) $(check_SCRIPTS_cdep) \
		$(check_SCRIPTS_arch)


TESTS = $(check_PROGRAMS) $(check_SCRIPTS)

noinst_PROGRAMS = $(noinst_PROGRAMS_common) $(noinst_PROGRAMS_cdep) \
	$(noinst_PROGRAMS_arch)

Lia64_test_readonly_SOURCES = Lia64-test-readonly.c ia64-test-readonly-asm.S
Gia64_test_readonly_SOURCES = Gia64-test-readonly.c ia64-test-readonly-asm.S
Lia64_test_stack_SOURCES = Lia64-test-stack.c ia64-test-stack-asm.S \
				ia64-test-stack.h
Gia64_test_stack_SOURCES = Gia64-test-stack.c ia64-test-stack-asm.S \
				ia64-test-stack.h
Lia64_test_rbs_SOURCES = Lia64-test-rbs.c ia64-test-rbs-asm.S ia64-test-rbs.h
Gia64_test_rbs_SOURCES = Gia64-test-rbs.c ia64-test-rbs-asm.S ia64-test-rbs.h
Lia64_test_nat_SOURCES = Lia64-test-nat.c ia64-test-nat-asm.S
Gia64_test_nat_SOURCES = Gia64-test-nat.c ia64-test-nat-asm.S
ia64_test_dyn1_SOURCES = ia64-test-dyn1.c ia64-dyn-asm.S flush-cache.S
Gtest_init_SOURCES = Gtest-init.cxx
Ltest_init_SOURCES = Ltest-init.cxx
Gtest_dyn1_SOURCES = Gtest-dyn1.c flush-cache.S
Ltest_dyn1_SOURCES = Ltest-dyn1.c flush-cache.S
test_static_link_SOURCES = test-static-link-loc.c test-static-link-gen.c
test_static_link_LDFLAGS = -static
forker_LDFLAGS = -static

LIBUNWIND = ../src/libunwind-$(arch).la
LDADD = $(LIBUNWIND)

test_setjmp_LDADD = ../src/libunwind-setjmp.la
test_ptrace_LDADD = ../src/libunwind-ptrace.a $(LIBUNWIND)
Ltest_concurrent_LDADD = $(LIBUNWIND) -lpthread
Gtest_concurrent_LDADD = $(LIBUNWIND) -lpthread
test_async_sig_LDADD = $(LIBUNWIND) -lpthread
