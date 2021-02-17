#!/bin/bash
#
# Copyright (C) 2018 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

# This script updates SQLite source files with a SQLite tarball.
#
# Usage: UPDATE-SOURCE.bash SQLITE-SOURCE.tgz
#
# This script must be executed in $ANDROID_BUILD_TOP/external/sqlite/
#

set -e

script_name="$(basename "$0")"

source_tgz="$1"
source_ext_dir="$1.extracted"

die() {
    echo "$script_name: $*"
    exit 1
}

echo_and_exec() {
    echo "  Running: $@"
    "$@"
}

# Make sure the source tgz file exists.
pwd="$(pwd)"
if [[ ! "$pwd" =~ .*/external/sqlite/? ]] ; then
    die 'Execute this script in $ANDROID_BUILD_TOP/external/sqlite/'
fi

# Make sure the source tgz file exists.

if [[ ! -f "$source_tgz" ]] ; then
    die " Missing or invalid argument.

  Usage: $script_name SQLITE-SOURCE_TGZ
"
fi


echo
echo "# Extracting the source tgz..."
echo_and_exec rm -fr "$source_ext_dir"
echo_and_exec mkdir -p "$source_ext_dir"
echo_and_exec tar xvf "$source_tgz" -C "$source_ext_dir" --strip-components=1

echo
echo "# Making file sqlite3.c in $source_ext_dir ..."
(
    cd "$source_ext_dir"
    echo_and_exec ./configure
    echo_and_exec make -j 4 sqlite3.c
)

echo
echo "# Copying the source files ..."
for to in dist/orig/ dist/ ; do
    echo_and_exec cp "$source_ext_dir/"{shell.c,sqlite3.c,sqlite3.h,sqlite3ext.h} "$to"
done

echo
echo "# Applying Android.patch ..."
(
    cd dist
    echo_and_exec patch -i Android.patch
)

echo
echo "# Regenerating Android.patch ..."
(
    cd dist
    echo_and_exec bash -c '(for x in orig/*; do diff -u -d $x ${x#orig/}; done) > Android.patch'
)

cat <<EOF

=======================================================

  Finished successfully!

  Make sure to update README.version

=======================================================

EOF

