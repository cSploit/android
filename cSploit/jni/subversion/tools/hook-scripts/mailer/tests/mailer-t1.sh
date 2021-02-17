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
#
# mailer-t1.sh: test #1 for the mailer.py script
#
# This test generates "email" for each revision in the repository,
# concatenating them into one big blob, which is then compared against
# a known output.
#
# Note: mailer-tweak.py must have been run to make the test outputs
#       consistent and reproducible
#
# USAGE: ./mailer-t1.sh REPOS MAILER-SCRIPT
#

if test "$#" != 2; then
    echo "USAGE: ./mailer-t1.sh REPOS MAILER-SCRIPT"
    exit 1
fi

scripts="`dirname $0`"
scripts="`cd $scripts && pwd`"

glom=$scripts/mailer-t1.current
orig=$scripts/mailer-t1.output
conf=$scripts/mailer.conf
rm -f $glom

export TZ=GST

youngest="`svnlook youngest $1`"
for rev in `python -c "print(\" \".join(map(str, range(1,$youngest+1))))"`; do
  $2 commit $1 $rev $conf >> $glom
done

echo "current mailer.py output in: $glom"

dos2unix $glom

echo diff -q $orig $glom
diff -q $orig $glom && echo "SUCCESS: no differences detected"
