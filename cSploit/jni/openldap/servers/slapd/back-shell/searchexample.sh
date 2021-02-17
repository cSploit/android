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
#
## Portions Copyright (c) 1995 Regents of the University of Michigan.
## All rights reserved.
##
## Redistribution and use in source and binary forms are permitted
## provided that this notice is preserved and that due credit is given
## to the University of Michigan at Ann Arbor. The name of the University
## may not be used to endorse or promote products derived from this
## software without specific prior written permission. This software
## is provided ``as is'' without express or implied warranty.

while [ 1 ]; do
	read TAG VALUE
	if [ $? -ne 0 ]; then
		break
	fi
	case "$TAG" in
		base:)
		BASE=$VALUE
		;;
		filter:)
		FILTER=$VALUE
		;;
		# include other parameters here
	esac
done

LOGIN=`echo $FILTER | sed -e 's/.*=\(.*\))/\1/'`

PWLINE=`grep -i "^$LOGIN" /etc/passwd`

#sleep 60
# if we found an entry that matches
if [ $? = 0 ]; then
	echo $PWLINE | awk -F: '{
		printf("dn: cn=%s,%s\n", $1, base);
		printf("objectclass: top\n");
		printf("objectclass: person\n");
		printf("cn: %s\n", $1);
		printf("cn: %s\n", $5);
		printf("sn: %s\n", $1);
		printf("uid: %s\n", $1);
	}' base="$BASE"
	echo ""
fi

# result
echo "RESULT"
echo "code: 0"

exit 0
