## Process this file with automake to produce Makefile.in
# Copyright (C) 2004-2013 Simon Josefsson
#
# This file is part of GNU Libidn.
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

SUBDIRS = src

EXTRA_DIST = LICENSE-2.0.txt

INSTALL_TARGETS =

if JAVA
libidn_jardir = $(datadir)/java
dist_libidn_jar_DATA = libidn-$(VERSION).jar

libidn-$(VERSION).jar:
	$(JAR) cf $@ \
		-C $(builddir)/src/main/java \
			gnu/inet/encoding/CombiningClass.class		\
		-C $(builddir)/src/main/java \
			gnu/inet/encoding/Composition.class		\
		-C $(builddir)/src/main/java \
			gnu/inet/encoding/DecompositionKeys.class	\
		-C $(builddir)/src/main/java \
			gnu/inet/encoding/DecompositionMappings.class	\
		-C $(builddir)/src/main/java \
			gnu/inet/encoding/IDNA.class			\
		-C $(builddir)/src/main/java \
			gnu/inet/encoding/IDNAException.class		\
		-C $(builddir)/src/main/java \
			gnu/inet/encoding/NFKC.class			\
		-C $(builddir)/src/main/java \
			gnu/inet/encoding/Punycode.class		\
		-C $(builddir)/src/main/java \
			gnu/inet/encoding/PunycodeException.class	\
		-C $(builddir)/src/main/java \
			gnu/inet/encoding/RangeSet\$$1.class		\
		-C $(builddir)/src/main/java \
			gnu/inet/encoding/RangeSet\$$Builder.class	\
		-C $(builddir)/src/main/java \
			gnu/inet/encoding/RangeSet.class		\
		-C $(builddir)/src/main/java \
			gnu/inet/encoding/RangeSet\$$Range.class	\
		-C $(builddir)/src/main/java \
			gnu/inet/encoding/RangeSet\$$RangeContainsComparator.class \
		-C $(builddir)/src/main/java \
			gnu/inet/encoding/RFC3454.class			\
		-C $(builddir)/src/main/java \
			gnu/inet/encoding/Stringprep.class		\
		-C $(builddir)/src/main/java \
			gnu/inet/encoding/StringprepException.class

INSTALL_TARGETS += install-jar
install-jar:
	install -d -m 755 $(DESTDIR)$(libidn_jardir)
	ln -sf libidn-$(VERSION).jar $(DESTDIR)$(libidn_jardir)/libidn.jar

clean-local:
	rm -f $(libidn_jar_DATA)

endif

install-data-local: $(INSTALL_TARGETS)
