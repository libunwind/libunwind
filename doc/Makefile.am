# man pages that go into section 3:
man3_MANS = libunwind.3 libunwind-dynamic.3 libunwind-ia64.3	\
	libunwind-coredump.3 \
	libunwind-ptrace.3 \
	libunwind-setjmp.3			\
	libunwind-nto.3 \
	unw_apply_reg_state.3						\
	unw_backtrace.3						\
	unw_flush_cache.3						\
	unw_get_accessors.3						\
	unw_get_proc_info.3						\
	unw_get_proc_info_by_ip.3					\
	unw_get_proc_info_in_range.3					\
	unw_get_proc_name.3						\
	unw_get_proc_name_by_ip.3					\
	unw_get_fpreg.3						\
	unw_get_reg.3							\
	unw_getcontext.3						\
	unw_init_local.3 unw_init_remote.3				\
	unw_init_local2.3						\
	unw_is_fpreg.3						\
	unw_is_signal_frame.3						\
	unw_create_addr_space.3					\
	unw_destroy_addr_space.3					\
	unw_regname.3 unw_resume.3					\
	unw_reg_states_iterate.3					\
	unw_set_caching_policy.3					\
	unw_set_iterate_phdr_function.3				\
	unw_set_cache_size.3						\
	unw_set_fpreg.3						\
	unw_set_reg.3							\
	unw_step.3							\
	unw_strerror.3						\
	_U_dyn_register.3						\
	_U_dyn_cancel.3         \
	unw_get_elf_filename.3			\
	unw_get_elf_filename_by_ip.3

EXTRA_DIST = NOTES libunwind.trans					\
	libunwind.tex libunwind-dynamic.tex libunwind-ia64.tex		\
	libunwind-coredump.tex \
	libunwind-ptrace.tex \
	libunwind-setjmp.tex			\
	libunwind-nto.tex \
	unw_apply_reg_state.tex						\
	unw_backtrace.tex						\
	unw_flush_cache.tex						\
	unw_get_accessors.tex						\
	unw_get_proc_info.tex						\
	unw_get_proc_info_by_ip.tex					\
	unw_get_proc_info_in_range.tex					\
	unw_get_proc_name.tex						\
	unw_get_proc_name_by_ip.tex					\
	unw_get_fpreg.tex						\
	unw_get_reg.tex							\
	unw_getcontext.tex						\
	unw_init_local.tex unw_init_remote.tex				\
	unw_is_fpreg.tex						\
	unw_is_signal_frame.tex						\
	unw_create_addr_space.tex unw_destroy_addr_space.tex		\
	unw_regname.tex unw_resume.tex unw_set_caching_policy.tex	\
	unw_set_iterate_phdr_function.tex				\
	unw_reg_states_iterate.tex					\
	unw_set_cache_size.tex						\
	unw_set_fpreg.tex						\
	unw_set_reg.tex							\
	unw_step.tex							\
	unw_strerror.tex						\
	_U_dyn_register.tex						\
	_U_dyn_cancel.tex						\
	unw_get_elf_filename.tex		\
	unw_get_elf_filename_by_ip.tex	\
	$(man3_MANS)

L2M	= latex2man
L2P	= pdflatex
L2M_CMD	= $(L2M) -t $(srcdir)/libunwind.trans
L2H_CMD	= $(L2M) -H -t $(srcdir)/libunwind.trans

.tex.3:
	$(AM_V_GEN)$(L2M_CMD) $< $@
	-cp $@ $(srcdir)/$@

html-local:
	for n in $(man3_MANS); do					\
		page=`basename $$n .3`;				\
		$(L2H_CMD) $(srcdir)/$$page.tex "$$page(3).raw";	\
	done

pdf-local:
	for n in $(man3_MANS); do					\
		page=`basename $$n .3`;				\
		$(L2P) $(srcdir)/$$page.tex "$$page(3).pdf";		\
	done

MAINTAINERCLEANFILES = Makefile.in
