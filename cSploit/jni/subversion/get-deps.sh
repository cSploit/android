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
# get-deps.sh -- download the dependencies useful for building Subversion
#

APR=apr-1.4.5
APR_UTIL=apr-util-1.3.12
NEON=neon-0.29.6
SERF=serf-0.7.2
ZLIB=zlib-1.2.8
SQLITE_VERSION=3.7.6.3
SQLITE_VERSION_LIST=`echo $SQLITE_VERSION | sed -e 's/\./ /g'`
SQLITE=sqlite-amalgamation-`printf %d%02d%02d%02d $SQLITE_VERSION_LIST`


HTTPD=httpd-2.2.25
APR_ICONV=apr-iconv-1.2.1

BASEDIR=`pwd`
TEMPDIR=$BASEDIR/temp

HTTP_FETCH=
[ -z "$HTTP_FETCH" ] && type wget  >/dev/null 2>&1 && HTTP_FETCH="wget -nc"
[ -z "$HTTP_FETCH" ] && type curl  >/dev/null 2>&1 && HTTP_FETCH="curl -O"
[ -z "$HTTP_FETCH" ] && type fetch >/dev/null 2>&1 && HTTP_FETCH="fetch"

# Need this uncommented if any of the specific versions of the ASF tarballs to
# be downloaded are no longer available on the general mirrors.
APACHE_MIRROR=http://archive.apache.org/dist

# helpers
usage() {
    echo "Usage: $0"
    echo "Usage: $0 [ apr | neon | serf | zlib | sqlite ] ..."
    exit $1
}

# getters
get_apr() {
    cd $TEMPDIR
    $HTTP_FETCH $APACHE_MIRROR/apr/$APR.tar.bz2
    $HTTP_FETCH $APACHE_MIRROR/apr/$APR_UTIL.tar.bz2
    cd $BASEDIR

    bzip2 -dc $TEMPDIR/$APR.tar.bz2 | tar -xf -
    bzip2 -dc $TEMPDIR/$APR_UTIL.tar.bz2 | tar -xf -

    mv $APR apr
    mv $APR_UTIL apr-util
}

get_neon() {
    cd $TEMPDIR
    $HTTP_FETCH http://webdav.org/neon/$NEON.tar.gz
    cd $BASEDIR

    gzip  -dc $TEMPDIR/$NEON.tar.gz | tar -xf -

    mv $NEON neon
}

get_serf() {
    cd $TEMPDIR
    $HTTP_FETCH http://serf.googlecode.com/files/$SERF.tar.bz2
    cd $BASEDIR

    bzip2 -dc $TEMPDIR/$SERF.tar.bz2 | tar -xf -

    mv $SERF serf
}

get_zlib() {
    cd $TEMPDIR
    $HTTP_FETCH http://www.zlib.net/$ZLIB.tar.gz
    cd $BASEDIR

    gzip -dc $TEMPDIR/$ZLIB.tar.gz | tar -xf -

    mv $ZLIB zlib
}

get_sqlite() {
    cd $TEMPDIR
    $HTTP_FETCH http://www.sqlite.org/$SQLITE.zip
    cd $BASEDIR

    unzip -q $TEMPDIR/$SQLITE.zip

    mv $SQLITE sqlite-amalgamation

}

# main()
get_deps() {
    mkdir -p $TEMPDIR

    for i in neon zlib serf sqlite-amalgamation apr apr-util; do
      if [ -d $i ]; then
        echo "Local directory '$i' already exists; the downloaded copy won't be used" >&2
      fi
    done

    if [ $# -gt 0 ]; then
      for target in "$@"; do
        if [ "$target" != "deps" ]; then
          get_$target || usage
        else
          usage
        fi
      done
    else
      get_apr
      get_neon
      get_serf
      get_zlib
      get_sqlite

      echo
      echo "If you require mod_dav_svn, the recommended version of httpd is:"
      echo "   $APACHE_MIRROR/httpd/$HTTPD.tar.bz2"

      echo
      echo "If you require apr-iconv, its recommended version is:"
      echo "   $APACHE_MIRROR/apr/$APR_ICONV.tar.bz2"
    fi

    rm -rf $TEMPDIR
}

get_deps "$@"
