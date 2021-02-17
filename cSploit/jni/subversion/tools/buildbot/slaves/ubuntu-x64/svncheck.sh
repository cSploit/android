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
test ! -e /dev/shm/svn-test-work && mkdir /dev/shm/svn-test-work
test -e subversion/tests/cmdline/svn-test-work && rm -rf subversion/tests/cmdline/svn-test-work
ln -s /dev/shm/svn-test-work subversion/tests/cmdline/

echo "========= make check"
make check FS_TYPE=$1 CLEANUP=1 || exit $?

# the bindings are checked with svncheck-bindings.sh
exit 0
