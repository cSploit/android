#!/bin/sh
#
#
# Licensed to the Apache Software Foundation (ASF) under one
# or more contributor license agreements.  See the NOTICE file
# distributed with this work for additional information
# regarding copyright ownership.  The ASF licenses this file
# to you under the Apache License, Version 2.0 (the
# "License"); you may not use this file except in compliance
# with the License.  You may obtain a copy of the License at
#
#   http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing,
# software distributed under the License is distributed on an
# "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
# KIND, either express or implied.  See the License for the
# specific language governing permissions and limitations
# under the License.
#
#
# -*- mode: shell-script; -*-

# This script simplifies the preparation of the environment for a Subversion client
# communicating with an svnserve server.
#
# The script runs svnserve, runs "make check", and kills the svnserve afterwards.
# It makes sure to kill the svnserve even if the test run dies.
#
# This script should be run from the top level of the Subversion
# distribution; it's easiest to just run it as "make
# svnserveautocheck".  Like "make check", you can specify further options
# like "make svnserveautocheck FS_TYPE=bdb TESTS=subversion/tests/cmdline/basic.py".

PYTHON=${PYTHON:-python}

SCRIPTDIR=$(dirname $0)
SCRIPT=$(basename $0)

set +e

trap trap_cleanup HUP TERM INT

# Ensure the server uses a known locale.
LC_ALL=C
export LC_ALL

really_cleanup() {
    if [ -e  "$SVNSERVE_PID" ]; then
        kill $(cat "$SVNSERVE_PID")
        rm -f $SVNSERVE_PID
    fi
}

trap_cleanup() {
    really_cleanup
    exit 1
}

say() {
  echo "$SCRIPT: $*"
}

fail() {
  say $*
  exit 1
}

if [ -x subversion/svn/svn ]; then
  ABS_BUILDDIR=$(pwd)
elif [ -x $SCRIPTDIR/../../svn/svn ]; then
  cd $SCRIPTDIR/../../../
  ABS_BUILDDIR=$(pwd)
  cd - >/dev/null
else
  fail "Run this script from the root of Subversion's build tree!"
fi

# If you change this, also make sure to change the svn:ignore entry
# for it and "make check-clean".
SVNSERVE_PID=$ABS_BUILDDIR/subversion/tests/svnserveautocheck.pid

export LD_LIBRARY_PATH="$ABS_BUILDDIR/subversion/libsvn_ra_neon/.libs:$ABS_BUILDDIR/subversion/libsvn_ra_local/.libs:$ABS_BUILDDIR/subversion/libsvn_ra_svn/.libs:$LD_LIBRARY_PATH"

SERVER_CMD="$ABS_BUILDDIR/subversion/svnserve/svnserve"

rm -f $SVNSERVE_PID

random_port() {
  if [ -n "$BASH_VERSION" ]; then
    echo $(($RANDOM+1024))
  else
    $PYTHON -c 'import random; print random.randint(1024, 2**16-1)'
  fi
}

SVNSERVE_PORT=$(random_port)
while netstat -an | grep $SVNSERVE_PORT | grep 'LISTEN'; do
  SVNSERVE_PORT=$(random_port)
done

if [ "$THREADED" != "" ]; then
  SVNSERVE_ARGS="-T"
fi

"$SERVER_CMD" -d -r "$ABS_BUILDDIR/subversion/tests/cmdline" \
            --listen-host 127.0.0.1 \
            --listen-port $SVNSERVE_PORT \
            --pid-file $SVNSERVE_PID \
            $SVNSERVE_ARGS &

BASE_URL=svn://127.0.0.1:$SVNSERVE_PORT
if [ $# = 0 ]; then
  time make check "BASE_URL=$BASE_URL"
  r=$?
else
  cd "$ABS_BUILDDIR/subversion/tests/cmdline/"
  TEST="$1"
  shift
  time "./${TEST}_tests.py" "--url=$BASE_URL" $*
  r=$?
  cd - > /dev/null
fi

really_cleanup
exit $r
