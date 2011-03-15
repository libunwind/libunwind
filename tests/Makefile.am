AM_CPPFLAGS = -I$(top_srcdir)/include

EXTRA_DIST =	run-ia64-test-dyn1 run-ptrace-mapper run-ptrace-misc	\
		run-check-namespace check-namespace.sh.in Gtest-nomalloc.c

MAINTAINERCLEANFILES = Makefile.in

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
 LIBUNWIND_local = $(top_builddir)/src/libunwind.la
if ARCH_IA64
 noinst_PROGRAMS_arch = ia64-test-dyn1
 check_SCRIPTS_arch =	run-ia64-test-dyn1
 check_PROGRAMS_arch =	Gia64-test-stack Lia64-test-stack		\
			Gia64-test-nat Lia64-test-nat			\
			Gia64-test-rbs Lia64-test-rbs			\
			Gia64-test-readonly Lia64-test-readonly		\
			ia64-test-setjmp ia64-test-sig
else
if ARCH_PPC64
if USE_ALTIVEC 
 noinst_PROGRAMS_arch_altivec = ppc64-test-altivec ppc64-test-wchar
endif #USE_ALTIVEC
 noinst_PROGRAMS_arch = $(noinst_PROGRAMS_arch_altivec) ppc64-test-wchar 
endif #ARCH_PPC64
endif #ARCH_IA64
 check_SCRIPTS_cdep =	run-ptrace-mapper run-ptrace-misc
 check_PROGRAMS_cdep =	Gtest-bt Ltest-bt Gtest-exc Ltest-exc		 \
			Gtest-init Ltest-init				 \
			Gtest-concurrent Ltest-concurrent		 \
			Gtest-resume-sig Ltest-resume-sig		 \
			Gtest-dyn1 Ltest-dyn1				 \
			test-async-sig test-flush-cache test-init-remote \
			test-mem test-setjmp test-ptrace		 \
			Ltest-nomalloc rs-race
 noinst_PROGRAMS_cdep = forker mapper test-ptrace-misc test-varargs	\
			Gperf-simple Lperf-simple
if ARCH_ARM
 check_PROGRAMS_arch =	arm-extbl-test
endif #ARCH_ARM

if SUPPORT_CXX_EXCEPTIONS
 check_PROGRAMS_cdep += Ltest-cxx-exceptions
endif

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

arm_extbl_test_CPPFLAGS = -I$(top_srcdir)/src \
				-I$(top_srcdir)/include/tdep-$(arch) \
				-funwind-tables
arm_extbl_test_LDFLAGS = -static

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
ppc64_test_altivec_SOURCES = ppc64-test-altivec.c  ppc64-test-altivec-utils.c
ppc64_test_wchar_SOURCES = ppc64-test-wchar.c
arm_extbl_test_SOURCES = arm-extbl-test.c
Gtest_init_SOURCES = Gtest-init.cxx
Ltest_init_SOURCES = Ltest-init.cxx
Ltest_cxx_exceptions_SOURCES = Ltest-cxx-exceptions.cxx

Gtest_dyn1_SOURCES = Gtest-dyn1.c flush-cache.S
Ltest_dyn1_SOURCES = Ltest-dyn1.c flush-cache.S
test_static_link_SOURCES = test-static-link-loc.c test-static-link-gen.c
test_static_link_LDFLAGS = -static
forker_LDFLAGS = -static
Gtest_bt_SOURCES = Gtest-bt.c ident.c
Ltest_bt_SOURCES = Ltest-bt.c ident.c
test_ptrace_misc_SOURCES = test-ptrace-misc.c ident.c
Ltest_nomalloc_SOURCES = Ltest-nomalloc.c

LIBUNWIND = $(top_builddir)/src/libunwind-$(arch).la $(LIBUNWIND_local) 
LIBUNWIND_ptrace = $(top_builddir)/src/libunwind-ptrace.a
LIBUNWIND_setjmp = $(top_builddir)/src/libunwind-setjmp.la	\
		   $(top_builddir)/src/libunwind-elf32.la $(LIBUNWIND)

test_async_sig_LDADD = $(LIBUNWIND) -lpthread
test_flush_cache_LDADD = $(LIBUNWIND)
test_init_remote_LDADD = $(LIBUNWIND)
test_mem_LDADD = $(LIBUNWIND)
test_ptrace_LDADD = $(LIBUNWIND_ptrace) $(LIBUNWIND)
test_proc_info_LDADD = $(LIBUNWIND)
test_static_link_LDADD = $(LIBUNWIND)
test_strerror_LDADD = $(LIBUNWIND)
rs_race_LDADD = $(LIBUNWIND) -lpthread

Gtest_bt_LDADD = $(LIBUNWIND)
Gtest_concurrent_LDADD = $(LIBUNWIND) -lpthread
Gtest_dyn1_LDADD = $(LIBUNWIND)
Gtest_exc_LDADD = $(LIBUNWIND)
Gtest_init_LDADD = $(LIBUNWIND)
Gtest_resume_sig_LDADD = $(LIBUNWIND)
Gperf_simple_LDADD = $(LIBUNWIND)

Ltest_bt_LDADD = $(LIBUNWIND_local)
Ltest_concurrent_LDADD = $(LIBUNWIND_local) -lpthread
Ltest_dyn1_LDADD = $(LIBUNWIND_local)
Ltest_exc_LDADD = $(LIBUNWIND_local)
Ltest_init_LDADD = $(LIBUNWIND_local)
Ltest_nomalloc_LDADD = $(LIBUNWIND_local) @DLLIB@
Ltest_resume_sig_LDADD = $(LIBUNWIND)
Lperf_simple_LDADD = $(LIBUNWIND_local)

test_setjmp_LDADD = $(LIBUNWIND_setjmp)
ia64_test_setjmp_LDADD = $(LIBUNWIND_setjmp)

arm_extbl_test_LDADD = $(top_builddir)/src/libunwind-elf32.la $(LIBUNWIND)
