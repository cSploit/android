## Makefile for OpenLDAP Administrator's Guide
# $OpenLDAP$
## This work is part of OpenLDAP Software <http://www.openldap.org/>.
##
## Copyright 2005-2014 The OpenLDAP Foundation.
## All rights reserved.
##
## Redistribution and use in source and binary forms, with or without
## modification, are permitted only as authorized by the OpenLDAP
## Public License.
##
## A copy of this license is available in the file LICENSE in the
## top-level directory of the distribution or, alternatively, at
## <http://www.OpenLDAP.org/license.html>.
all: guide.html index.html

# for website building (for webmaster use, don't change)
www: guide.html index.html OpenLDAP-Admin-Guide.pdf

sdf-src: \
	../plain.sdf \
	../preamble.sdf \
	abstract.sdf \
	access-control.sdf \
	appendix-changes.sdf \
	appendix-common-errors.sdf \
	appendix-configs.sdf \
	appendix-contrib.sdf \
	appendix-deployments.sdf \
	appendix-ldap-result-codes.sdf \
	appendix-recommended-versions.sdf \
	appendix-upgrading.sdf \
	backends.sdf \
	config.sdf \
	dbtools.sdf \
	glossary.sdf \
	guide.sdf \
	install.sdf \
	intro.sdf \
	maintenance.sdf \
	master.sdf \
	monitoringslapd.sdf \
	overlays.sdf \
	preface.sdf \
	quickstart.sdf \
	referrals.sdf \
	replication.sdf \
	runningslapd.sdf \
	sasl.sdf \
	schema.sdf \
	security.sdf \
	slapdconfig.sdf \
	title.sdf \
	tls.sdf \
	troubleshooting.sdf \
	tuning.sdf

sdf-img: \
	../images/LDAPlogo.gif \
	allmail-en.png \
	allusersgroup-en.png \
	config_dit.png \
	config_local.png \
	config_ref.png \
	config_repl.png \
	delta-syncrepl.png \
	dual_dc.png \
	intro_dctree.png \
	intro_tree.png \
	ldap-sync-refreshandpersist.png \
	ldap-sync-refreshonly.png \
	n-way-multi-master.png \
	push-based-complete.png \
	push-based-standalone.png \
	refint.png \
	set-following-references.png \
	set-memberUid.png \
	set-recursivegroup.png 

guide.html: guide.sdf sdf-src sdf-img
	sdf -2html guide.sdf

index.html: index.sdf sdf-src sdf-img
	sdf -2topics index.sdf

admin.html: admin.sdf sdf-src sdf-img
	sdf -DPDF -2html admin.sdf

guide.pdf: admin.html guide.book
	htmldoc --batch guide.book -f guide.pdf

OpenLDAP-Admin-Guide.pdf: admin.html guide.book
	htmldoc --batch guide.book -f OpenLDAP-Admin-Guide.pdf

clean: 
	rm -f *.pdf *.html *~ *.bak
