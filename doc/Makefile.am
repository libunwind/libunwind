# man pages that go into section 3:
man3_MANS = $(srcdir)/libunwind.man $(srcdir)/libunwind-ia64.man

EXTRA_DIST = NOTES libunwind.trans		\
	libunwind.tex libunwind-ia64.tex	\
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
