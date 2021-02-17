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
# Usage:
# ./po-update.sh pot
#   - to generate just the pot file
# ./po-update.sh
#   - to update all locales
# ./po-update.sh LL
#   - to update one the LL locale

set -e

XGETTEXT=${XGETTEXT:-xgettext}
MSGMERGE=${MSGMERGE:-msgmerge}

svn_base=
for i in . .. ../..; do
  if [ -d "$i/subversion/po" ]; then
    svn_base="$i"
    break
  fi
done
if [ -z "$svn_base" ]; then
  echo "E: You must run po-update.sh from within a Subversion source tree." >&2
  exit 1
fi

pot_done=

make_pot()
{
  if [ -z "$pot_done" ]; then
    echo "Building subversion.pot..."
    (cd $svn_base/subversion/po && \
    find .. \
    -name .svn -prune -or \
    -name tests -prune -or \
    -name bindings -prune -or \
    -name "*.c" -print -or \
    -name "svn_error_codes.h" -print -or \
    -name "svn_fs_util.h" -print | \
    $XGETTEXT --sort-by-file -k_ -kN_ -kQ_:1,2 -kSVN_ERRDEF:3 \
    --flag=_:1:pass-c-format \
    --flag=N_:1:pass-c-format \
    --flag=Q_:1:pass-c-format \
    --flag=Q_:2:pass-c-format \
    --flag=svn_cmdline_printf:2:c-format \
    --flag=svn_cmdline_fprintf:3:c-format \
    --flag=svn_error_createf:3:c-format \
    --flag=svn_error_wrap_apr:2:c-format \
    --flag=svn_stream_printf:3:c-format \
    --flag=svn_stream_printf_from_utf8:4:c-format \
    --flag=svn_string_createf:2:c-format \
    --flag=svn_string_createv:2:c-format \
    --flag=svn_stringbuf_createf:2:c-format \
    --flag=svn_stringbuf_createv:2:c-format \
    --flag=svn_fs_bdb__dberrf:3:c-format \
    --flag=file_printf_from_utf8:3:c-format \
    --flag=do_io_file_wrapper_cleanup:3:c-format \
    --flag=do_io_file_wrapper_cleanup:4:c-format \
    --msgid-bugs-address=dev@subversion.apache.org \
    --add-comments --files-from=- -o subversion.pot )
    pot_done=1
  fi
}

update_po()
{
  (cd $svn_base/subversion/po &&
  for i in $1.po; do
    echo "Updating $i..."
    # In a display of truly bizarre behaviour, msgmerge (at least, the
    # GNU gettext-tools 0.14.6 implementation) inverts the order of obsolete
    # messages every time it is run. Therefore, run it twice, to invert and
    # then re-invert, to minimize spurious diffs.
    $MSGMERGE --sort-by-file --no-wrap --update $i subversion.pot 
    $MSGMERGE --sort-by-file --no-wrap --update $i subversion.pot 
  done )
}

if [ $# -eq 0 ]; then
  make_pot
  update_po \*
else
  langs=
  while [ $# -ge 1 ]; do
    case $1 in
      pot) ;;
      *)
      if [ -e $svn_base/subversion/po/$1.po ]; then
        langs="$langs $1"
      else
        echo "E: No such .po file '$1.po'" >&2
        exit 1
      fi
    esac
    shift
  done
  make_pot
  for lang in $langs; do
    update_po $lang
  done
fi
