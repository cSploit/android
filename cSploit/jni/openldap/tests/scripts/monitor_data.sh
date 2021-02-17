#! /bin/sh
# $OpenLDAP$
## This work is part of OpenLDAP Software <http://www.openldap.org/>.
##
## Copyright 1998-2014 The OpenLDAP Foundation.
## All rights reserved.
##
## Redistribution and use in source and binary forms, with or without
## modification, are permitted only as authorized by the OpenLDAP
## Public License.
##
## A copy of this license is available in the file LICENSE in the
## top-level directory of the distribution or, alternatively, at
## <http://www.OpenLDAP.org/license.html>.

MONITORDB="$1"
SRCDIR="$2"
DSTDIR="$3"

echo "MONITORDB $MONITORDB"
echo "SRCDIR $SRCDIR"
echo "DSTDIR $DSTDIR"
echo "pwd `pwd`"

# copy test data
cp "$SRCDIR"/do_* "$DSTDIR"
if test $MONITORDB != no ; then

	# add back-monitor testing data
	cat >> "$DSTDIR/do_search.0" << EOF
cn=Monitor
(objectClass=*)
cn=Monitor
(objectClass=*)
cn=Monitor
(objectClass=*)
cn=Monitor
(objectClass=*)
EOF

	cat >> "$DSTDIR/do_read.0" << EOF
cn=Backend 1,cn=Backends,cn=Monitor
cn=Entries,cn=Statistics,cn=Monitor
cn=Database 1,cn=Databases,cn=Monitor
EOF

fi

