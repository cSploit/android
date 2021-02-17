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
# -*- mode: shell-script; -*-
# $Id$

# This script simplifies preparation of environment for Subversion client
# communicating with a server via DAV protocol. The prerequisites of such
# testing are:
#   - Subversion built using --enable-shared --enable-dso --with-apxs options,
#   - Working Apache 2 HTTPD Server with the apxs program reachable through
#     PATH or specified via the APXS environment variable,
#   - Modules dav_module and log_config_module compiled as DSO or built into
#     Apache HTTPD Server executable.
# The basic intension of this script is to be able to perform "make check"
# operation over DAV without any configuration efforts whatsoever, provided
# that conditions above are met.
#
# The script will find Apache and all necessary modules including mod_dav_svn,
# create a temporary directory in subversion/tests/cmdline, create
# Apache 2 configuration file in this directory, start Apache 2 on a random
# port number higher than 1024, and execute Subversion command-line client
# test suites against this instance of HTTPD. Every vital configuration
# parameter is checked before the tests start. The script will ask questions
# about browsing Apache error log (default is "no") and about deleting
# temporary directory (default "yes") and pause for 32 seconds before
# proceeding with the default. HTTPD access log is also created in the
# temporary directory.
#
# Run this script without parameters to execute the full battery of tests:
#   subversion/tests/cmdline/davautocheck.sh
# Run this script with the name of a test suite to run this suite:
#   subversion/tests/cmdline/davautocheck.sh basic
# Run this script with the test suite name and test number to execute just this
# test:
#   subversion/tests/cmdline/davautocheck.sh basic 4
#
# If the temporary directory is not deleted, it can be reused for further
# manual DAV protocol interoperation testing. HTTPD must be started by
# specifying configuration file on the command line:
#   httpd -f subversion/tests/cmdline/<httpd-...>/cfg
#
# If you want to run this against an *installed* HTTPD (for example, to test
# one version's client against another version's server) specify both APXS
# *and* MODULE_PATH for the other server:
#
#   APXS=/opt/svn/1.4.x/bin/apxs MODULE_PATH=/opt/svn/1.4.x/modules \ 
#     subversion/tests/cmdline/davautocheck.sh
#
# To prevent the server from advertising httpv2, pass USE_HTTPV1 in
# the environment.
# 
# To use value for "SVNPathAuthz" directive set SVN_PATH_AUTHZ with
# appropriate value in the environment.
#
# To load an MPM module for Apache 2.4 use APACHE_MPM=event in the
# environment.
#
# Passing --no-tests as argv[1] will have the script start a server
# but not run any tests.

SCRIPTDIR=$(dirname $0)
SCRIPT=$(basename $0)

trap stop_httpd_and_die SIGHUP SIGTERM SIGINT

# Ensure the server uses a known locale.
LC_ALL=C
export LC_ALL

function stop_httpd_and_die() {
  [ -e "$HTTPD_PID" ] && kill $(cat "$HTTPD_PID")
  exit 1
}

function say() {
  echo "$SCRIPT: $*"
}

function fail() {
  say $*
  stop_httpd_and_die
}

function query() {
  echo -n "$SCRIPT: $1 (y/n)? [$2] "
  read -n 1 -t 32
  echo
  [ "${REPLY:-$2}" = 'y' ]
}

function get_loadmodule_config() {
  local SO="$($APXS -q LIBEXECDIR)/$1.so"

  # shared object module?
  if [ -r "$SO" ]; then
    local NM=$(echo "$1" | sed 's|mod_\(.*\)|\1_module|')
    echo "LoadModule $NM \"$SO\"" &&
    return
  fi

  # maybe it's built-in?
  "$HTTPD" -l | grep -q "$1\\.c" && return

  return 1
}

# Check apxs's SBINDIR and BINDIR for given program names
function get_prog_name() {
  for prog in $*
  do
    for dir in $($APXS -q SBINDIR) $($APXS -q BINDIR)
    do
      if [ -e "$dir/$prog" ]; then
        echo "$dir/$prog" && return
      fi
    done
  done

  return 1
}

# Don't assume sbin is in the PATH.
PATH="$PATH:/usr/sbin:/usr/local/sbin"

# Remove any proxy environmental variables that affect wget or curl.
# We don't need a proxy to connect to localhost and having the proxy
# environmental variables set breaks the Apache configuration file
# test below, since wget or curl will ask the proxy to connect to
# localhost.
unset PROXY
unset http_proxy
unset HTTPS_PROXY

# Pick up value from environment or PATH (also try apxs2 - for Debian)
[ ${APXS:+set} ] \
 || APXS=$(which apxs) \
 || APXS=$(which apxs2) \
 || fail "neither apxs or apxs2 found - required to run davautocheck"

[ -x $APXS ] || fail "Can't execute apxs executable $APXS"

say "Using '$APXS'..."

# Pick up $USE_HTTPV1
ADVERTISE_V2_PROTOCOL=on
if [ ${USE_HTTPV1:+set} ]; then
 ADVERTISE_V2_PROTOCOL=off
fi

# Pick up $SVN_PATH_AUTHZ
SVN_PATH_AUTHZ_LINE=""
if [ ${SVN_PATH_AUTHZ:+set} ]; then
 SVN_PATH_AUTHZ_LINE="SVNPathAuthz      ${SVN_PATH_AUTHZ}"
fi

# Find the source and build directories. The build dir can be found if it is
# the current working dir or the source dir.
pushd ${SCRIPTDIR}/../../../ > /dev/null
ABS_SRCDIR=$(pwd)
popd > /dev/null
if [ -x subversion/svn/svn ]; then
  ABS_BUILDDIR=$(pwd)
elif [ -x $ABS_SRCDIR/subversion/svn/svn ]; then
  ABS_BUILDDIR=$ABS_SRCDIR
else
  fail "Run this script from the root of Subversion's build tree!"
fi

if [ ${MODULE_PATH:+set} ]; then
    MOD_DAV_SVN="$MODULE_PATH/mod_dav_svn.so"
    MOD_AUTHZ_SVN="$MODULE_PATH/mod_authz_svn.so"
else
    MOD_DAV_SVN="$ABS_BUILDDIR/subversion/mod_dav_svn/.libs/mod_dav_svn.so"
    MOD_AUTHZ_SVN="$ABS_BUILDDIR/subversion/mod_authz_svn/.libs/mod_authz_svn.so"
fi

[ -r "$MOD_DAV_SVN" ] \
  || fail "dav_svn_module not found, please use '--enable-shared --enable-dso --with-apxs' with your 'configure' script"
[ -r "$MOD_AUTHZ_SVN" ] \
  || fail "authz_svn_module not found, please use '--enable-shared --enable-dso --with-apxs' with your 'configure' script"

export LD_LIBRARY_PATH="$ABS_BUILDDIR/subversion/libsvn_ra_neon/.libs:$ABS_BUILDDIR/subversion/libsvn_ra_local/.libs:$ABS_BUILDDIR/subversion/libsvn_ra_svn/.libs"

case "`uname`" in
  Darwin*) LDD='otool -L'
    ;;
  *) LDD='ldd'
    ;;
esac

httpd="$($APXS -q PROGNAME)"
HTTPD=$(get_prog_name $httpd) || fail "HTTPD '$HTTPD' not found"
[ -x $HTTPD ] || fail "HTTPD '$HTTPD' not executable"

"$HTTPD" -v 1>/dev/null 2>&1 \
  || fail "HTTPD '$HTTPD' doesn't start properly"

say "Using '$HTTPD'..."

HTPASSWD=$(get_prog_name htpasswd htpasswd2) \
  || fail "Could not find htpasswd or htpasswd2"
[ -x $HTPASSWD ] \
  || fail "HTPASSWD '$HTPASSWD' not executable"
say "Using '$HTPASSWD'..."

LOAD_MOD_DAV=$(get_loadmodule_config mod_dav) \
  || fail "DAV module not found"

LOAD_MOD_LOG_CONFIG=$(get_loadmodule_config mod_log_config) \
  || fail "log_config module not found"

# needed for TypesConfig
LOAD_MOD_MIME=$(get_loadmodule_config mod_mime) \
  || fail "MIME module not found"

LOAD_MOD_ALIAS=$(get_loadmodule_config mod_alias) \
  || fail "ALIAS module not found"

# needed for Auth*, Require, etc. directives
LOAD_MOD_AUTH=$(get_loadmodule_config mod_auth) \
  || {
say "Monolithic Auth module not found. Assuming we run against Apache 2.1+"
LOAD_MOD_AUTH="$(get_loadmodule_config mod_auth_basic)" \
    || fail "Auth_Basic module not found."
LOAD_MOD_ACCESS_COMPAT="$(get_loadmodule_config mod_access_compat)" \
    && {
say "Found modules for Apache 2.3.0+"
LOAD_MOD_AUTHN_CORE="$(get_loadmodule_config mod_authn_core)" \
    || fail "Authn_Core module not found."
LOAD_MOD_AUTHZ_CORE="$(get_loadmodule_config mod_authz_core)" \
    || fail "Authz_Core module not found."
LOAD_MOD_AUTHZ_HOST="$(get_loadmodule_config mod_authz_host)" \
    || fail "Authz_Host module not found."
LOAD_MOD_UNIXD=$(get_loadmodule_config mod_unixd) \
    || fail "UnixD module not found"
}
LOAD_MOD_AUTHN_FILE="$(get_loadmodule_config mod_authn_file)" \
    || fail "Authn_File module not found."
LOAD_MOD_AUTHZ_USER="$(get_loadmodule_config mod_authz_user)" \
    || fail "Authz_User module not found."
}
if [ ${APACHE_MPM:+set} ]; then
    LOAD_MOD_MPM=$(get_loadmodule_config mod_mpm_$APACHE_MPM) \
      || fail "MPM module not found"
fi

HTTPD_PORT=$(($RANDOM+1024))
HTTPD_ROOT="$ABS_BUILDDIR/subversion/tests/cmdline/httpd-$(date '+%Y%m%d-%H%M%S')"
HTTPD_CFG="$HTTPD_ROOT/cfg"
HTTPD_PID="$HTTPD_ROOT/pid"
HTTPD_ACCESS_LOG="$HTTPD_ROOT/access_log"
HTTPD_ERROR_LOG="$HTTPD_ROOT/error_log"
HTTPD_MIME_TYPES="$HTTPD_ROOT/mime.types"
BASE_URL="http://localhost:$HTTPD_PORT"
HTTPD_USERS="$HTTPD_ROOT/users"

mkdir "$HTTPD_ROOT" \
  || fail "couldn't create temporary directory '$HTTPD_ROOT'"

say "Using directory '$HTTPD_ROOT'..."

say "Adding users for lock authentication"
$HTPASSWD -bc $HTTPD_USERS jrandom   rayjandom
$HTPASSWD -b  $HTTPD_USERS jconstant rayjandom

touch $HTTPD_MIME_TYPES

cat > "$HTTPD_CFG" <<__EOF__
$LOAD_MOD_MPM
$LOAD_MOD_LOG_CONFIG
$LOAD_MOD_MIME
$LOAD_MOD_ALIAS
$LOAD_MOD_UNIXD
$LOAD_MOD_DAV
LoadModule          dav_svn_module "$MOD_DAV_SVN"
$LOAD_MOD_AUTH
$LOAD_MOD_AUTHN_CORE
$LOAD_MOD_AUTHN_FILE
$LOAD_MOD_AUTHZ_CORE
$LOAD_MOD_AUTHZ_USER
$LOAD_MOD_AUTHZ_HOST
LoadModule          authz_svn_module "$MOD_AUTHZ_SVN"

__EOF__

if "$HTTPD" -v | grep '/2\.[012]' >/dev/null; then
  cat >> "$HTTPD_CFG" <<__EOF__
LockFile            lock
User                $(id -un)
Group               $(id -gn)
__EOF__
else
  cat >> "$HTTPD_CFG" <<__EOF__
# TODO: maybe uncomment this for prefork,worker MPMs only?
# Mutex file:lock mpm-accept
__EOF__
fi

cat >> "$HTTPD_CFG" <<__EOF__
Listen              $HTTPD_PORT
ServerName          localhost
PidFile             "$HTTPD_PID"
LogFormat           "%h %l %u %t \"%r\" %>s %b" common
CustomLog           "$HTTPD_ACCESS_LOG" common
ErrorLog            "$HTTPD_ERROR_LOG"
LogLevel            Debug
ServerRoot          "$HTTPD_ROOT"
DocumentRoot        "$HTTPD_ROOT"
ScoreBoardFile      "$HTTPD_ROOT/run"
CoreDumpDirectory   "$HTTPD_ROOT"
TypesConfig         "$HTTPD_MIME_TYPES"
StartServers        4
MaxRequestsPerChild 0
<IfModule worker.c>
  ThreadsPerChild   8
</IfModule>
MaxClients          16
HostNameLookups     Off
LogFormat           "%h %l %u %t \"%r\" %>s %b \"%{Referer}i\" \"%{User-Agent}i\"" format
CustomLog           "$HTTPD_ROOT/req" format
CustomLog           "$HTTPD_ROOT/ops" "%t %u %{SVN-REPOS-NAME}e %{SVN-ACTION}e" env=SVN-ACTION

<Directory />
  AllowOverride     none
</Directory>
<Directory "$HTTPD_ROOT">
  AllowOverride     none
  #Require           all granted
</Directory>

<Location /svn-test-work/repositories>
  DAV               svn
  SVNParentPath     "$ABS_BUILDDIR/subversion/tests/cmdline/svn-test-work/repositories"
  AuthzSVNAccessFile "$ABS_BUILDDIR/subversion/tests/cmdline/svn-test-work/authz"
  AuthType          Basic
  AuthName          "Subversion Repository"
  AuthUserFile      $HTTPD_USERS
  Require           valid-user
  SVNAdvertiseV2Protocol ${ADVERTISE_V2_PROTOCOL}
  ${SVN_PATH_AUTHZ_LINE}
</Location>
<Location /svn-test-work/local_tmp/repos>
  DAV               svn
  SVNPath           "$ABS_BUILDDIR/subversion/tests/cmdline/svn-test-work/local_tmp/repos"
  AuthzSVNAccessFile "$ABS_BUILDDIR/subversion/tests/cmdline/svn-test-work/authz"
  AuthType          Basic
  AuthName          "Subversion Repository"
  AuthUserFile      $HTTPD_USERS
  Require           valid-user
  SVNAdvertiseV2Protocol ${ADVERTISE_V2_PROTOCOL}
  ${SVN_PATH_AUTHZ_LINE}
</Location>
RedirectMatch permanent ^/svn-test-work/repositories/REDIRECT-PERM-(.*)\$ /svn-test-work/repositories/\$1
RedirectMatch           ^/svn-test-work/repositories/REDIRECT-TEMP-(.*)\$ /svn-test-work/repositories/\$1
__EOF__

START="$HTTPD -f $HTTPD_CFG"

$START -t \
  || fail "Configuration file didn't pass the check, most likely modules couldn't be loaded"

# need to pause for some time to let HTTPD start
$START &
sleep 2

say "HTTPD started and listening on '$BASE_URL'..."

# Perform a trivial validation of our httpd configuration by
# downloading a file and comparing it to the original copy.
### The file at the path "/cfg" can't be retrieved from Apache 2.3+.
### We get a 500 ISE, with the following error in the log from httpd's
### server/request.c:ap_process_request_internal():
###   [Wed Feb 22 13:06:55 2006] [crit] [client 127.0.0.1] configuration error:  couldn't check user: /cfg
HTTP_FETCH=wget
HTTP_FETCH_OUTPUT="-q -O"
type wget > /dev/null 2>&1
if [ $? -ne 0 ]; then
  type curl > /dev/null 2>&1
  if [ $? -ne 0 ]; then
    fail "Neither curl or wget found."
  fi
  HTTP_FETCH=curl
  HTTP_FETCH_OUTPUT='-s -o'
fi
$HTTP_FETCH $HTTP_FETCH_OUTPUT "$HTTPD_CFG-copy" "$BASE_URL/cfg"
diff -q "$HTTPD_CFG" "$HTTPD_CFG-copy" > /dev/null \
  || fail "HTTPD doesn't operate according to the generated configuration"
rm "$HTTPD_CFG-copy"

say "HTTPD is good"

if [ $# -eq 1 ] && [ "x$1" = 'x--no-tests' ]; then
  exit
fi

say "starting the tests..."

CLIENT_CMD="$ABS_BUILDDIR/subversion/svn/svn"
$LDD "$CLIENT_CMD" | grep -q 'not found' \
  && fail "Subversion client couldn't be fully linked at run-time"

if [ "$HTTP_LIBRARY" = "" ]; then
  say "Using default dav library"
  "$CLIENT_CMD" --version | egrep -q '^[*] ra_(neon|serf)' \
    || fail "Subversion client couldn't find and/or load ra_dav library"
else
  say "Requesting dav library '$HTTP_LIBRARY'"
  "$CLIENT_CMD" --version | egrep -q "^[*] ra_$HTTP_LIBRARY" \
    || fail "Subversion client couldn't find and/or load ra_dav library '$HTTP_LIBRARY'"
fi

if [ $# = 0 ]; then
  time make check "BASE_URL=$BASE_URL"
  r=$?
else
  pushd "$ABS_BUILDDIR/subversion/tests/cmdline/" >/dev/null
  TEST="$1"
  shift
  time "$ABS_SRCDIR/subversion/tests/cmdline/${TEST}_tests.py" "--url=$BASE_URL" "$@"
  r=$?
  popd >/dev/null
fi

say "Finished testing..."

kill $(cat "$HTTPD_PID")

query 'Browse server access log' n \
  && less "$HTTPD_ACCESS_LOG"

query 'Browse server error log' n \
  && less "$HTTPD_ERROR_LOG"

query 'Delete HTTPD root directory' y \
  && rm -fr "$HTTPD_ROOT/"

say 'Done'

exit $r
