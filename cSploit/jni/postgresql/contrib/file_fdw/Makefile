# contrib/file_fdw/Makefile

MODULES = file_fdw

EXTENSION = file_fdw
DATA = file_fdw--1.0.sql

REGRESS = file_fdw

EXTRA_CLEAN = sql/file_fdw.sql expected/file_fdw.out

ifdef USE_PGXS
PG_CONFIG = pg_config
PGXS := $(shell $(PG_CONFIG) --pgxs)
include $(PGXS)
else
subdir = contrib/file_fdw
top_builddir = ../..
include $(top_builddir)/src/Makefile.global
include $(top_srcdir)/contrib/contrib-global.mk
endif
