# man pages that go into section 3:
man3_MANS = libunwind.man

EXTRA_DIST = NOTES libunwind.trans libunwind.tex $(man3_MANS)

L2M	= latex2man
L2M_CMD	= $(L2M) -t $(srcdir)/libunwind.trans
L2H_CMD	= $(L2M) -H -t $(srcdir)/libunwind.trans

.tex.man:
	$(L2M_CMD) $< $@

html:
	for n in $(man3_MANS); do				\
		page=`basename $$n .man`;			\
		$(L2H_CMD) $(srcdir)/$$page.tex $$page.php;	\
	done
