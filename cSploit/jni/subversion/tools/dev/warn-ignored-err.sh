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
HELP="\
Usage: $0 [--remove] [FILE...]

Insert or remove the GCC attribute \"warn_unused_result\" on each function
that returns a Subversion error, in the specified files or, by default,
*.h and *.c in the ./subversion and ./tools trees.
"

LC_ALL=C

# Parse options
REMOVE=
case "$1" in
--remove) REMOVE=1; shift;;
--help)   echo "$HELP"; exit 0;;
--*)      echo "$0: unknown option \"$1\"; try \"--help\""; exit 1;;
esac

# Set the positional parameters to the default files if none specified
if [ $# = 0 ]; then
  set -- `find subversion/ tools/ -name '*.[ch]'`
fi

# A line that declares a function return type of "svn_error_t *" looks like:
# - Possibly leading whitespace, though not often.
# - Possibly "static" or "typedef".
# - The return type "svn_error_t *".
# - Possibly a function or pointer-to-function declarator:
#     - "identifier"
#     - "(identifier)"  (used in some typedefs)
#     - "(*identifier)"
#   with either nothing more, or a "(" next (especially not "," or ";" or "="
#   which all indicate a variable rather than a function).

# Regular expressions for "sed"
# Note: take care in matching back-reference numbers to parentheses
PREFIX="^\( *\| *static  *\| *typedef  *\)"
RET_TYPE="\(svn_error_t *\* *\)"
IDENT="[a-zA-Z_][a-zA-Z0-9_]*"
DECLR="\($IDENT\|( *\(\*\|\) *$IDENT *)\)"
SUFFIX="\($DECLR *\((.*\|\)\|\)$"

# The attribute string to be inserted or removed
ATTRIB_RE="__attribute__((warn_unused_result))"  # regex version of it
ATTRIB_STR="__attribute__((warn_unused_result))"  # plain text version of it

if [ $REMOVE ]; then
  SUBST="s/$PREFIX$ATTRIB_RE $RET_TYPE$SUFFIX/\1\2\3/"
else
  SUBST="s/$PREFIX$RET_TYPE$SUFFIX/\1$ATTRIB_STR \2\3/"
fi

for F do
  # Edit the file, leaving a backup suffixed with a tilde
  { sed -e "$SUBST" "$F" > "$F~1" &&
    { ! cmp -s "$F" "$F~1"; } &&
    mv "$F" "$F~" &&  # F is briefly absent now; a copy could avoid this
    mv "$F~1" "$F"
  } ||
  # If anything went wrong or no change was made, remove the temporary file
  rm "$F~1"
done
