#!/bin/sh
#
#  create_wc_for_upgrade.sh : create a working copy for upgrade testing
#
#  Subversion is a tool for revision control.
#  See http://subversion.apache.org/ for more information.
#
# ====================================================================
#    Licensed to the Apache Software Foundation (ASF) under one
#    or more contributor license agreements.  See the NOTICE file
#    distributed with this work for additional information
#    regarding copyright ownership.  The ASF licenses this file
#    to you under the Apache License, Version 2.0 (the
#    "License"); you may not use this file except in compliance
#    with the License.  You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
#    Unless required by applicable law or agreed to in writing,
#    software distributed under the License is distributed on an
#    "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
#    KIND, either express or implied.  See the License for the
#    specific language governing permissions and limitations
#    under the License.
######################################################################

#
# USAGE:
#   $ ./create_wc_for_upgrade.sh /path/to/svn-1.x /tmp/wc-1.x
#
# At this point, the working copy will be constructed for *property*
# testing. Future changes may set up other testing scenarios.
#
# Note that 'svnadmin' must be in PATH
#

if test $# != 2; then
  echo "ERROR: not enough parameters"
  exit 1
fi

SVN="$1"
WC="$2"

if test -e "${WC}"; then
  echo "ERROR: ${WC} exists."
  exit 1
fi

# some higgery-jiggery to get to the parent directory in order to create
# the repo as a sibling.
mkdir "${WC}"
cd "${WC}"
cd ..
rmdir "${WC}"

# create a repo that is usable by any SVN. bail out if anything goes wrong.
svnadmin create --pre-1.4-compatible repo
"${SVN}" co file://`pwd`/repo "${WC}"
cd "${WC}" || exit 1

# we need some files with properties for copy sources and for deletions
echo alpha > alpha
echo beta > beta
echo gamma > gamma
echo delta > delta
echo epsilon > epsilon
"${SVN}" add alpha beta gamma delta epsilon
"${SVN}" propset a-prop a-value1 alpha
"${SVN}" propset b-prop b-value1 beta
"${SVN}" propset g-prop g-value1 gamma
"${SVN}" propset d-prop d-value1 delta
"${SVN}" propset e-prop e-value1 epsilon
"${SVN}" commit -m "commit files"

### the code below needs to be rejiggered for svn <= 1.3. revert base is not
### available, so some of the operations are not allowed.

# a file with .base and .revert
"${SVN}" delete alpha
"${SVN}" copy epsilon alpha
### whoops. there is an alpha.working (when using 1.7-dev, tho no ACTUAL_NODE
### row is created).

# a file with just .working
### what comes after epsilon??
echo lambda > lambda
"${SVN}" add lambda
"${SVN}" propset l-prop l-value lambda

# a file with .base and .working
"${SVN}" propset b-more b-value2 beta

# a file with .base, .revert, and .working
"${SVN}" delete gamma
"${SVN}" copy epsilon gamma
"${SVN}" propset g-more g-value2 gamma

# a file with .revert and .working
"${SVN}" delete delta
echo delta revisited > delta
"${SVN}" add delta
"${SVN}" propset d-more d-value2 delta

# a file with just .revert
"${SVN}" delete epsilon
echo epsilon revisited > epsilon
"${SVN}" add epsilon
