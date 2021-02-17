#!/bin/sh

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

# usage: run this script from the root of your working copy
#        and / or adjust the path settings below as needed

# set SVNPATH to the 'subversion' folder of your SVN source code w/c

SVNPATH="$('pwd')/subversion"

# if using the installed svn, you may need to adapt the following.
# Uncomment the VALGRIND line to use that tool instead of "time".
# Comment the SVNSERVE line to use file:// instead of svn://.

SVN=${SVNPATH}/svn/svn
SVNADMIN=${SVNPATH}/svnadmin/svnadmin   
SVNSERVE=${SVNPATH}/svnserve/svnserve
# VALGRIND="valgrind --tool=callgrind"

# set your data paths here

WC=/dev/shm/wc
REPOROOT=/dev/shm

# number of items per folder on the first run. It will be doubled
# after every iteration. The test will stop if MAXCOUNT has been
# reached or exceeded (and will not be executed for MAXCOUNT).

FILECOUNT=1
MAXCOUNT=20000

# only 1.7 supports server-side caching and uncompressed data transfer 

SERVEROPTS="-c 0 -M 400"

# from here on, we should be good

TIMEFORMAT='%3R  %3U  %3S'
REPONAME=dirs
PORT=54321
if [ "${SVNSERVE}" != "" ] ; then
  URL=svn://localhost:$PORT/$REPONAME
else
  URL=file://${REPOROOT}/$REPONAME
fi

# create repository

rm -rf $WC $REPOROOT/$REPONAME
mkdir $REPOROOT/$REPONAME
${SVNADMIN} create $REPOROOT/$REPONAME
echo "[general]
anon-access = write" > $REPOROOT/$REPONAME/conf/svnserve.conf

# fire up svnserve

if [ "${SVNSERVE}" != "" ] ; then
  VERSION=$( ${SVNSERVE} --version | grep " version" | sed 's/.*\ 1\.\([0-9]\).*/\1/' )
  if [ "$VERSION" -lt "7" ]; then
    SERVEROPTS=""
  fi

  ${SVNSERVE} -Tdr ${REPOROOT} ${SERVEROPTS} --listen-port ${PORT} --foreground &
  PID=$!
  sleep 1
fi

# construct valgrind parameters

if [ "${VALGRIND}" != "" ] ; then
  VG_TOOL=$( echo ${VALGRIND} | sed 's/.*\ --tool=\([a-z]*\).*/\1/' )
  VG_OUTFILE="--${VG_TOOL}-out-file"
fi

# print header

printf "using "
${SVN} --version | grep " version"
echo

# init working copy

rm -rf $WC
${SVN} co $URL $WC > /dev/null

# helpers

get_sequence() {
  # three equivalents...
  (jot - "$1" "$2" "1" 2>/dev/null || seq -s ' ' "$1" "$2" 2>/dev/null || python -c "for i in range($1,$2+1): print(i)")
}

# functions that execute an SVN command

run_svn() {
  if [ "${VALGRIND}" = "" ] ; then
    time ${SVN} $1 $WC/$2 $3 > /dev/null
  else
    ${VALGRIND} ${VG_OUTFILE}="${VG_TOOL}.out.$1.$2" ${SVN} $1 $WC/$2 $3 > /dev/null
  fi
}

run_svn_del() {
  if [ "${VALGRIND}" = "" ] ; then
    time ${SVN} del $WC/${1}_c/$2 -q > /dev/null
  else
    ${VALGRIND} ${VG_OUTFILE}="${VG_TOOL}.out.del.$1" ${SVN} del $WC/${1}_c/$2 -q > /dev/null
  fi
}

run_svn_ci() {
  if [ "${VALGRIND}" = "" ] ; then
    time ${SVN} ci $WC/$1 -m "" -q > /dev/null
  else
    ${VALGRIND} ${VG_OUTFILE}="${VG_TOOL}.out.ci_$2.$1" ${SVN} ci $WC/$1 -m "" -q > /dev/null
  fi
}

run_svn_cp() {
  if [ "${VALGRIND}" = "" ] ; then
    time ${SVN} cp $WC/$1 $WC/$2 > /dev/null
  else
    ${VALGRIND} ${VG_OUTFILE}="${VG_TOOL}.out.cp.$1" ${SVN} cp $WC/$1 $WC/$2 > /dev/null
  fi
}

run_svn_get() {
  if [ "${VALGRIND}" = "" ] ; then
    time ${SVN} $1 $URL $WC -q > /dev/null
  else
    ${VALGRIND} ${VG_OUTFILE}="${VG_TOOL}.out.$1.$2" ${SVN} $1 $URL $WC -q > /dev/null
  fi
}

# main loop 

while [ $FILECOUNT -lt $MAXCOUNT ]; do
  echo "Processing $FILECOUNT files in the same folder"

  sequence=`get_sequence 2 $FILECOUNT`
  printf "\tCreating files ... \t real   user    sys\n"
  mkdir $WC/$FILECOUNT
  for i in 1 $sequence; do
    echo "File number $i" > $WC/$FILECOUNT/$i
  done    

  printf "\tAdding files ...   \t"
  run_svn add $FILECOUNT -q

  printf "\tRunning status ... \t"
  run_svn st $FILECOUNT -q

  printf "\tCommit files ...   \t"
  run_svn_ci $FILECOUNT add
  
  printf "\tListing files ...  \t"
  run_svn ls $FILECOUNT

  printf "\tUpdating files ... \t"
  run_svn up $FILECOUNT -q

  printf "\tLocal copy ...     \t"
  run_svn_cp $FILECOUNT ${FILECOUNT}_c

  printf "\tCommit copy ...    \t"
  run_svn_ci ${FILECOUNT}_c copy

  printf "\tDelete 1 file ...  \t"
  run_svn_del ${FILECOUNT} 1

  printf "\tDeleting files ... \t"
  time sh -c "
  for i in $sequence; do
    ${SVN} del $WC/${FILECOUNT}_c/\$i -q
  done "

  printf "\tCommit deletions ...\t"
  run_svn_ci ${FILECOUNT}_c del

  rm -rf $WC

  printf "\tExport all ...  \t"
  run_svn_get export $FILECOUNT

  rm -rf $WC
  mkdir $WC

  printf "\tCheck out all ...  \t"
  run_svn_get co $FILECOUNT

  FILECOUNT=`echo 2 \* $FILECOUNT | bc`
  echo ""
done

# tear down

if [ "${SVNSERVE}" != "" ] ; then
  echo "killing svnserve ... "
  kill $PID
fi

