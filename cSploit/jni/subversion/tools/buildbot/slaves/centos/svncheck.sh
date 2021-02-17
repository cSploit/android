#!/bin/bash
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


set -x

if test -z "$1" ; then
  echo "Missing FS_TYPE specifier (arg #1)."
  exit 1
fi

echo "========= mount RAM disc"
# ignore the result: if it fails, the test will just take longer...
mkdir -p subversion/tests/cmdline/svn-test-work
test -e ../mount-ramdrive && ../mount-ramdrive

echo "========= make"
case "$2" in
  ""|ra_dav|ra_neon)
    make davautocheck FS_TYPE=$1 HTTP_LIBRARY=neon CLEANUP=1 || exit $?
    ;;
  ra_serf)
    make davautocheck FS_TYPE=$1 HTTP_LIBRARY=serf CLEANUP=1 || exit $?
    ;;
  ra_svn)
    make svnserveautocheck FS_TYPE="$1" CLEANUP=1 || exit $?
    ;;
  ra_local)
    make check FS_TYPE="$1" CLEANUP=1 || exit $?
    ;;
  *)
    echo "Bad RA specifier (arg #2): '$2'."
    exit 1
  ;;
esac

# the bindings are checked with svncheck-bindings.sh
exit 0
