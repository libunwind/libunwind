# man pages that go into section 3:
man3_MANS = $(srcdir)/libunwind.man $(srcdir)/libunwind-ia64.man	\
	$(srcdir)/libunwind-ptrace.man $(srcdir)/libunwind-setjmp.man	\
	$(srcdir)/unw_init_local.man $(srcdir)/unw_init_remote.man	\
	$(srcdir)/unw_create_addr_space.man				\
	$(srcdir)/unw_destroy_addr_space.man				\
	$(srcdir)/unw_resume.man

EXTRA_DIST = NOTES libunwind.trans				\
	libunwind.tex libunwind-ia64.tex libunwind-ptrace.tex	\
	libunwind-setjmp.tex					\
	unw_init_local.tex unw_init_remote.tex unw_resume.tex	\
	unw_create_addr_space.tex unw_destroy_addr_space.tex	\
	$(man3_MANS)

L2M	= latex2man
L2P	= pdflatex
L2M_CMD	= $(L2M) -t $(srcdir)/libunwind.trans
L2H_CMD	= $(L2M) -H -t $(srcdir)/libunwind.trans

.tex.man:
	-bk get -e $@
	$(L2M_CMD) $< $@

html:
	for n in $(man3_MANS); do					\
		page=`basename $$n .man`;				\
		$(L2H_CMD) $(srcdir)/$$page.tex "$$page(3).php";	\
	done

pdf:
	for n in $(man3_MANS); do					\
		page=`basename $$n .man`;				\
		$(L2P) $(srcdir)/$$page.tex "$$page(3).pdf";		\
	done
