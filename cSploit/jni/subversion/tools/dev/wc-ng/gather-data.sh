#/usr/bin/env sh
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
# Trap Ctrl-C
trap 'exit 1' 2

# Some useful variables
REPOS=file:///home/hwright/dev/test/svn-mirror
WC=blech
REV_LIST=revs_list
SCRIPT=count-progress.py
DATA=data.csv

# Sync up the local repo
svnsync sync $REPOS

# Grab the list of revisions of interest on trunk
svn log -q -r0:HEAD $REPOS/trunk \
  | grep -v '^----' \
  | cut -f1 -d '|' \
  | cut -b2- > $REV_LIST

# Export the counting script
if [ -e $SCRIPT ]; then
  rm $SCRIPT
fi
svn export $REPOS/trunk/tools/dev/wc-ng/$SCRIPT $SCRIPT

# Checkout a working copy
if [ ! -d "$WC" ]; then
  svn co $REPOS/trunk $WC -r1
fi

# Get all the symbols of interest from the counting script and write
# them out at the headers in our csv file
LINE=""
for l in `./$SCRIPT $WC | tail -n +3 | grep -v '^----' | cut -f 1 -d '|'`; do
  LINE="$LINE,$l"
done
echo "Revision$LINE" > $DATA

# Iterate over all the revisions of interest
export SVN_I_LOVE_CORRUPTED_WORKING_COPIES_SO_DISABLE_SLEEP_FOR_TIMESTAMPS='yes'
for r in `cat $REV_LIST`; do
  svn up -r$r $WC -q

  # Do the count for that rev, and put the data in our data file
  LINE=""
  for l in `./$SCRIPT $WC | tail -n +3 | grep -v '^----' | cut -f 4 -d '|'`; do
    LINE="$LINE,$l"
  done
  echo "$r$LINE" >> $DATA

  echo "Done with revision $r"
done
unset SVN_I_LOVE_CORRUPTED_WORKING_COPIES_SO_DISABLE_SLEEP_FOR_TIMESTAMPS

# Cleanup
rm -rf $WC
rm $REV_LIST
