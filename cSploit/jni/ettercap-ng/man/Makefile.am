
# $Id: Makefile.am,v 1.8 2004/09/22 15:53:10 alor Exp $

include $(top_srcdir)/Makefile.am.common

man_MANS = ettercap.8 ettercap_curses.8 etterlog.8 etterfilter.8 etter.conf.5

if PLUGINS
   man_MANS += ettercap_plugins.8
endif

EXTRA_DIST = $(man_MANS)

CLEANFILES = *.8 *.5 *.ps *.pdf

pdf: $(man_MANS:.8=.ps) $(man_MANS:.5=.ps)

%.ps: %.8
	groff -mandoc -Tps $< > $@;
	ps2pdf13 $@ $(*F).pdf;
	rm -f *.ps

%.ps: %.5
	groff -mandoc -Tps $< > $@;
	ps2pdf13 $@ $(*F).pdf;
	rm -f *.ps

