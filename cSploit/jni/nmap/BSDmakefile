# $Id: BSDmakefile 13976 2009-06-29 23:48:19Z fyodor $
# Redirect BSD make to GNU gmake for convenience

USE_GNU:
	@gmake $(.TARGETS)

$(.TARGETS): USE_GNU

.PHONY: USE_GNU
