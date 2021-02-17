#!/bin/sh
#
#  The contents of this file are subject to the Initial
#  Developer's Public License Version 1.0 (the "License");
#  you may not use this file except in compliance with the
#  License. You may obtain a copy of the License at
#  http://www.ibphoenix.com/main.nfs?a=ibphoenix&page=ibp_idpl.
#
#  Software distributed under the License is distributed AS IS,
#  WITHOUT WARRANTY OF ANY KIND, either express or implied.
#  See the License for the specific language governing rights
#  and limitations under the License.
#
#  The Original Code was created by Alexander Peshkov on 2-Apr-2007
#  for the Firebird Open Source RDBMS project.
#
#  Copyright (c) 2007 Alexander Peshkov
#  and all contributors signed below.
#
#  All Rights Reserved.
#  Contributor(s): ______________________________________.
#
#
#
#
# Script to prepare an "official source distribution"
# User is responsible to have clean tree with no foreign files.
# Using such tarball does not require presence of auto-tools.
#

# Determine root of local checkout
SRCROOT=`dirname $0`/../..
pushd $SRCROOT >/dev/null 2>&1
SRCROOT=`pwd`
popd >/dev/null 2>&1

# What and where to bundle
MODULE=$SRCROOT/temp/src
MEMBERS="builds doc examples extern lang_helpers src ChangeLog Makefile.in acx_pthread.m4 autogen.sh binreloc.m4 configure.ac"

# Cleanup
rm -rf $MODULE

echo "Copying to new tree"
mkdir $MODULE
pushd $SRCROOT >/dev/null 2>&1
MAKEFILES=`echo gen/[Mm]ake*`
popd >/dev/null 2>&1
tar -C $SRCROOT -cf - $MEMBERS $MAKEFILES | tar -C $MODULE -xf -

# Load version information from the tree
source $MODULE/src/misc/writeBuildNum.sh
PACKNAME="Firebird-$PRODUCT_VER_STRING-$FIREBIRD_PACKAGE_VERSION"
DIRNAME="$SRCROOT/temp/$PACKNAME"

echo "Cleaning up"
rm -rf $DIRNAME
mv $MODULE $DIRNAME
pushd $DIRNAME >/dev/null 2>&1

# Remove CVS/SVN information
rm -rf `find . -name CVS -print`
rm -rf `find . -name .svn -print`

# Clean gpre-generated files and extern
cd gen
make clean_all
cd ..
rm -rf gen

# Copy pre-generated script
cp $SRCROOT/configure .

echo "Creating tarball for $PACKNAME"
cd ..
tar cjf $SRCROOT/gen/$PACKNAME.tar.bz2 $PACKNAME
popd >/dev/null 2>&1
