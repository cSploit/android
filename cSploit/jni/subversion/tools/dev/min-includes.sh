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
# Attempt to figure out the minimum set of includes for our header files.
#
# ### this is incomplete. it merely lists the header files in order from
# ### "no dependencies on other svn headers" to the larger header files
# ### which have dependencies. manually working through the headers in
# ### this order will minimize includes.
#
# Each header file is test-compiled to ensure that it has enough headers.
# Of course, this could be false-positive because another header that
# has been included has further included something to enable compilation
# of the header in question. More sophisticated testing (e.g. filtering
# includes out of the included header) would be necessary for detection.
#

files="*.h private/*.h"
deps="deps.$$"

INCLUDES="-I. -I.. -I/usr/include/apr-1 -I/usr/include/apache2"

rm -f "$deps"
for f in $files ; do
  sed -n "s%#include \"\(svn_[a-z0-9_]*\.h\)\".*%$f \1%p" $f | fgrep -v svn_private_config.h >> "$deps"
done


function process_file ()
{
  echo "Processing $header"

  echo "#include \"$header\"" > "$deps".c
  gcc -o /dev/null -S $INCLUDES "$deps".c

  ### monkey the includes and recompile to find the minimal set
}

while test -s "$deps" ; do
#wc -l $deps

  for header in $files ; do

    if grep -q "^$header" "$deps" ; then
      continue
    fi

    process_file

    fgrep -v "$header" "$deps" > "$deps".new
    mv "$deps".new "$deps"

    files="`echo $files | sed s%$header%%`"
    break
  done

done

for header in $files ; do
  process_file
done
