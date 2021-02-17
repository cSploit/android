# This makefile for History library documentation is in -*- text -*- mode.
# Emacs likes it that way.

DOC_SUPPORT = ../../doc-support/
TEXINDEX    = $(DOC_SUPPORT)/texindex

TEX    = tex

DVIOBJ = readline.dvi history.dvi
INFOBJ = readline.info history.info
PSOBJ = readline.ps history.ps

all: info dvi

readline.dvi: rlman.texinfo rluser.texinfo rltech.texinfo
	$(TEX) rlman.texinfo
	$(TEXINDEX) rlman.??
	$(TEX) rlman.texinfo
	mv rlman.dvi readline.dvi

readline.info: rlman.texinfo rluser.texinfo rltech.texinfo
	makeinfo rlman.texinfo

history.dvi: hist.texinfo hsuser.texinfo hstech.texinfo
	$(TEX) hist.texinfo
	$(TEXINDEX) hist.??
	$(TEX) hist.texinfo
	mv hist.dvi history.dvi

history.info: hist.texinfo hsuser.texinfo hstech.texinfo
	makeinfo hist.texinfo

readline.ps:	readline.dvi
	dvips -D 300 -o $@ readline.dvi

history.ps:	history.dvi
	dvips -D 300 -o $@ history.dvi

info:	$(INFOOBJ)
dvi:	$(DVIOBJ)
ps:	$(PSOBJ)


$(TEXINDEX):
	(cd $(DOC_SUPPORT); $(MAKE) $(MFLAGS) CFLAGS='$(CFLAGS)' texindex)

clean:
	rm -f *.aux *.cp *.fn *.ky *.log *.pg *.toc *.tp *.vr *.cps *.pgs \
	      *.fns *.kys *.tps *.vrs *.o core

squeaky-clean:
	rm -f *.aux *.cp *.fn *.ky *.log *.pg *.toc *.tp *.vr *.cps *.pgs \
	      *.dvi *.info *.info-* *.fns *.kys *.tps *.vrs *.o core

distclean: clean
realclean: clean
