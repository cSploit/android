#!/bin/bash
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
# Script to automate testing of an svnsync master/slave
# configuration.  Commits to the slave should write through
# to the master, and the master's post-commit hook svnsync's
# to the slave.  The test should be able to throw all kinds
# of svn operations at one or the other, and master/slave
# verified as identical in the end.
# 
# Master / slave setup is achieved in a single httpd process
# using virtual hosts bound to different addresses on the
# loopback network (127.0.0.1, 127.0.0.2) for slave and
# master, respectively.
#
# The set of changes sent through the system is currently
# just the test case for issue 2939, using svnmucc
# http://subversion.tigris.org/issues/show_bug.cgi?id=2939
# But of course, any svn traffic liable to break over
# mirroring would be a good addition.
# 
# Most of the httpd setup was lifted from davautocheck.sh.
# The common boilerplate snippets to setup/start/stop httpd
# between the two could be factored out and shared.
#

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
  say "FAIL: " $*
  stop_httpd_and_die
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

# splat out httpd config 
function setup_config() {

  say "setting up config: " $1
cat > "$1" <<__EOF__
$LOAD_MOD_LOG_CONFIG
$LOAD_MOD_MIME
$LOAD_MOD_UNIXD
$LOAD_MOD_DAV
LoadModule          dav_svn_module "$MOD_DAV_SVN"
$LOAD_MOD_AUTH
$LOAD_MOD_AUTHN_CORE
$LOAD_MOD_AUTHN_FILE
$LOAD_MOD_PROXY
$LOAD_MOD_PROXY_HTTP
$LOAD_MOD_AUTHZ_CORE
$LOAD_MOD_AUTHZ_USER
$LOAD_MOD_AUTHZ_HOST

LockFile            lock
User                $(id -un)
Group               $(id -gn)
Listen              ${TEST_PORT}
ServerName          localhost
PidFile             "${HTTPD_ROOT}/pid"
LogFormat           "%h %l %u %t \"%r\" %>s %b" common
CustomLog           "${HTTPD_ROOT}/access_log" common
ErrorLog            "${HTTPD_ROOT}/error_log"
LogLevel            Debug
ServerRoot          "${HTTPD_ROOT}"
DocumentRoot        "${HTTPD_ROOT}"
CoreDumpDirectory   "${HTTPD_ROOT}"
TypesConfig         "${HTTPD_ROOT}/mime.types"
StartServers        4
MaxRequestsPerChild 0
<IfModule worker.c>
  ThreadsPerChild   8
</IfModule>
MaxClients          16
HostNameLookups     Off
LogFormat           "%h %l %u %t \"%r\" %>s %b \"%{Referer}i\" \"%{User-Agent}i\"" format
CustomLog           "${HTTPD_ROOT}/req" format
CustomLog           "${HTTPD_ROOT}/ops" "%t %u %{SVN-REPOS-NAME}e %{SVN-ACTION}e" env=SVN-ACTION

<Directory />
  AllowOverride     none
</Directory>
<Directory "${HTTPD_ROOT}">
  AllowOverride     none
  #Require           all granted
</Directory>

# slave
<VirtualHost ${SLAVE_HOST}>
  ServerName ${SLAVE_HOST}
  CustomLog           "${HTTPD_ROOT}/slave_access_log" common
  ErrorLog            "${HTTPD_ROOT}/slave_error_log"
# slave 'normal' location  
  <Location "/${SLAVE_LOCATION}">
    DAV               svn
    SVNPath           "${SLAVE_REPOS}"
    SVNMasterURI      "${MASTER_URL}"
    AuthType          Basic
    AuthName          "Subversion Repository"
    AuthUserFile      ${HTTPD_ROOT}/users
    Require           valid-user
  </Location>
# slave 'sync' location
  <Location "/${SYNC_LOCATION}">
   DAV svn
   SVNPath "${SLAVE_REPOS}"
   AuthType Basic
   AuthName "Slave Sync Repository"
   AuthUserFile ${HTTPD_ROOT}/users
   Require valid-user
</Location>
</VirtualHost>

# master
<VirtualHost ${MASTER_HOST}>
  ServerName ${MASTER_HOST}>
  CustomLog           "${HTTPD_ROOT}/master_access_log" common
  ErrorLog            "${HTTPD_ROOT}/master_error_log"
  <Location "/${MASTER_LOCATION}">
    DAV               svn
    SVNPath           "${MASTER_REPOS}"
    AuthType          Basic
    AuthName          "Subversion Repository"
    AuthUserFile      ${HTTPD_ROOT}/users
    Require           valid-user
  </Location>
</VirtualHost>
__EOF__
}

function usage() {
  echo "usage: $SCRIPT <test-work-directory>" 1>&2
  echo "  e.g. \"$SCRIPT /tmp/test-work\"" 1>&2
  echo
  echo " " '<test-work-directory>' must not exist, \
    I will not clobber it for you 1>&2
  exit 1  
}
### Start execution here ###

SCRIPT=$(basename $0)

if [ $# -ne 1 ] ; then
  usage
fi


# httpd ServerRoot, all test and runtime artifacts below here
# verify that this doesn't already exist - don't clobber
HTTPD_ROOT=$1

if [ -e "$HTTPD_ROOT" ] ; then
  say "ERROR: test work directory $HTTPD_ROOT already exists, please remove" 1>&2
  usage
fi

#set -e

# Don't assume sbin is in the PATH.
PATH="$PATH:/usr/sbin:/usr/local/sbin"

# Pick up value from environment or PATH (also try apxs2 - for Debian)
[ ${APXS:+set} ] \
 || APXS=$(which apxs) \
 || APXS=$(which apxs2) \
 || fail "neither apxs or apxs2 found - required to run $SCRIPT"

[ -x $APXS ] || fail "Can't execute apxs executable $APXS"

say APXS: $APXS

if [ -x subversion/svn/svn ]; then
  ABS_BUILDDIR=$(pwd)
elif [ -x $SCRIPTDIR/../../svn/svn ]; then
  pushd $SCRIPTDIR/../../../ >/dev/null
  ABS_BUILDDIR=$(pwd)
  popd >/dev/null
else
  fail "Run this script from the root of Subversion's build tree!"
fi

# find all our needed executables, in WC or via apxs
httpd="$($APXS -q PROGNAME)"
HTTPD=$(get_prog_name $httpd) || fail "HTTPD not found"
HTPASSWD=$(get_prog_name htpasswd htpasswd2) \
  || fail "Could not find htpasswd or htpasswd2"
SVN=$ABS_BUILDDIR/subversion/svn/svn
SVNADMIN=$ABS_BUILDDIR/subversion/svnadmin/svnadmin
SVNSYNC=$ABS_BUILDDIR/subversion/svnsync/svnsync
SVNMUCC=${SVNMUCC:-$ABS_BUILDDIR/tools/client-side/svnmucc/svnmucc}
SVNLOOK=$ABS_BUILDDIR/subversion/svnlook/svnlook

[ -x $HTTPD ] || fail "HTTPD '$HTTPD' not executable"
[ -x $HTPASSWD ] \
  || fail "HTPASSWD '$HTPASSWD' not executable"
[ -x $SVN ] || fail "SVN $SVN not built"
[ -x $SVNADMIN ] || fail "SVNADMIN $SVNADMIN not built"
[ -x $SVNSYNC ] || fail "SVNSYNC $SVNSYNC not built"
[ -x $SVNLOOK ] || fail "SVNLOOK $SVNLOOK not built"
[ -x $SVNMUCC ] \
 || fail SVNMUCC $SVNMUCC executable not built, needed for test. \
    \'cd $ABS_BUILDDIR\; make svnmucc\' to fix.

say HTTPD: $HTTPD
say SVN: $SVN
say SVNADMIN: $SVNADMIN
say SVNSYNC: $SVNSYNC
say SVNLOOK: $SVNLOOK
say SVNMUCC: $SVNMUCC

LOAD_MOD_DAV=$(get_loadmodule_config mod_dav) \
  || fail "DAV module not found"

LOAD_MOD_LOG_CONFIG=$(get_loadmodule_config mod_log_config) \
  || fail "log_config module not found"

# proxy needed for svnsync mirroring
LOAD_MOD_PROXY=$(get_loadmodule_config mod_proxy) \
  || fail "proxy module not found"
LOAD_MOD_PROXY_HTTP=$(get_loadmodule_config mod_proxy_http) \
  || fail "proxy_http module not found"

# needed for TypesConfig
LOAD_MOD_MIME=$(get_loadmodule_config mod_mime) \
  || fail "MIME module not found"

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

MASTER_REPOS="${MASTER_REPOS:-"$HTTPD_ROOT/master_repos"}"
SLAVE_REPOS="${SLAVE_REPOS:-"$HTTPD_ROOT/slave_repos"}"

MASTER_HOST=127.0.0.2
SLAVE_HOST=127.0.0.1
#TEST_PORT=11111
TEST_PORT=$(($RANDOM+1024))

# location directive elements for master,slave,sync
# tests currently work if master==slave,fail if different
# ** Should different locations for each work?
#MASTER_LOCATION="master"
#SLAVE_LOCATION="slave"
MASTER_LOCATION="repo"
SLAVE_LOCATION="repo"
SYNC_LOCATION="sync"

MASTER_URL="http://${MASTER_HOST}:${TEST_PORT}/${MASTER_LOCATION}"
SLAVE_URL="http://${SLAVE_HOST}:${TEST_PORT}/${SLAVE_LOCATION}"
SYNC_URL="http://${SLAVE_HOST}:${TEST_PORT}/${SYNC_LOCATION}"

BASE_URL="$SLAVE_URL"

# setup server and repositories
say "setting up in ${HTTPD_ROOT}:"
mkdir -p $HTTPD_ROOT || fail "cannot mkdir $HTTPD_ROOT"
HTTPD_CONFIG=$HTTPD_ROOT/cfg
setup_config $HTTPD_CONFIG
touch $HTTPD_ROOT/mime.types
HTTPD_USERS="$HTTPD_ROOT/users"
$HTPASSWD -bc $HTTPD_USERS jrandom   rayjandom
$HTPASSWD -b  $HTTPD_USERS jconstant rayjandom
$HTPASSWD -b  $HTTPD_USERS scm scm
$HTPASSWD -b  $HTTPD_USERS svnsync svnsync
$SVNADMIN create "$MASTER_REPOS" || fail "create master repos failed"
$SVNADMIN create "$SLAVE_REPOS" || fail "create slave repos failed"
# dup them
$SVNADMIN dump "$MASTER_REPOS" | $SVNADMIN load "$SLAVE_REPOS" \
  || fail "duplicate repositories failed"
# make sure uuid's match
[ `cat "$SLAVE_REPOS/db/uuid"` = `cat "$MASTER_REPOS/db/uuid"` ] \
  || fail "master/slave uuid mismatch"
# setup hooks:
#  slave allows revprop changes
#  master syncs changes to slave
echo "#!/bin/sh" > "$SLAVE_REPOS/hooks/pre-revprop-change"
echo "#!/bin/sh" > "$MASTER_REPOS/hooks/post-revprop-change"
echo "#!/bin/sh" > "$MASTER_REPOS/hooks/post-commit"
echo "$SVNSYNC --non-interactive sync '$SYNC_URL' --username=svnsync --password=svnsync" \
    >> "$MASTER_REPOS/hooks/post-revprop-change"
echo "$SVNSYNC --non-interactive sync '$SYNC_URL' --username=svnsync --password=svnsync" \
    >> "$MASTER_REPOS/hooks/post-commit"

chmod 0755 "$SLAVE_REPOS/hooks/pre-revprop-change"
chmod 0755 "$MASTER_REPOS/hooks/post-revprop-change"
chmod 0755 "$MASTER_REPOS/hooks/post-commit"

say "created master and slave repositories"

# test config
$HTTPD -f $HTTPD_CONFIG -t || fail "httpd config failure in $HTTPD_CONFIG"

# start httpd
echo -n "${SCRIPT}: starting httpd: "
$HTTPD -f $HTTPD_CONFIG -k start || fail "httpd start failed"
echo "."
say initializing svnsync to $SYNC_URL
HTTPD_PID=$HTTPD_ROOT/pid
$SVNSYNC initialize --non-interactive "$SYNC_URL" "$MASTER_URL" \
    --username=svnsync --password=svnsync \
    || fail "svnsync initialize failed"

# OK, let's start testing! Commit changes to slave, expect
# them to proxy through to the master, and then
# svnsync back to the slave
#
# reproducible test case from:
# http://subversion.tigris.org/issues/show_bug.cgi?id=2939
# 
BASE_URL="$SLAVE_URL"
say running svnmucc test to $BASE_URL
svnmucc="$SVNMUCC --non-interactive --username jrandom --password rayjandom -mm"

$svnmucc mkdir "$BASE_URL/trunk" mkdir "$BASE_URL/trunk/dir1" mkdir "$BASE_URL/trunk/dir1/dir2"
$svnmucc rm "$BASE_URL/trunk/dir1/dir2"
$svnmucc cp 2 "$BASE_URL/trunk" "$BASE_URL/branch" put /dev/null "$BASE_URL/branch/dir1/dir2"
$svnmucc rm "$BASE_URL/branch" cp 2 "$BASE_URL/trunk" "$BASE_URL/branch" put /dev/null "$BASE_URL/branch/dir1/dir2"

say "svn log on $BASE_URL : "
$SVN --username jrandom --password rayjandom log -vq "$BASE_URL"


# verify result: should be at rev 4 in both repos
# FIXME: do more rigorous verification here
MASTER_HEAD=`$SVNLOOK youngest "$MASTER_REPOS"`
SLAVE_HEAD=`$SVNLOOK youngest "$SLAVE_REPOS"`

say checking consistency of master, slave repositories:

if [ "$MASTER_HEAD" != "4" ] || [ "$SLAVE_HEAD" != "4" ] ;
then
  say FAIL: master, slave are at rev $MASTER_HEAD, $SLAVE_HEAD, not 4
  say server may be started/stopped manually with:
  say "  $HTTPD -f $HTTPD_CONFIG -k start|stop"
  fail charred remains in $HTTPD_ROOT for your perusal
fi

say "PASS: master, slave are both at r4, as expected"

# The following test case is for the regression issue triggered by r917523.
# The revision r917523 do some url encodings to the paths and uris which are
# not url-encoded. But there is one additional url-encoding of an uri which is
# already encoded. With this extra encoding, committing a path to slave which
# has space in it fails. Please see this thread
# http://svn.haxx.se/dev/archive-2011-03/0641.shtml for more info.

say "Test case for regression issue triggered by r917523"

$svnmucc cp 2 "$BASE_URL/trunk" "$BASE_URL/branch new"
$svnmucc put /dev/null "$BASE_URL/branch new/file" \
--config-option servers:global:http-library=neon
RETVAL=$?

if [ $RETVAL -eq 0 ] ; then
  say "PASS: committing a path which has space in it passes"
else
  say "FAIL: committing a path which has space in it fails as there are extra
  url-encodings happening in server side"
fi

# Test case for commit to out-dated(though target path is up to date) slave.
# See issue #3860 for details.
say "Test case for out-dated slave commit"

svn="$SVN --non-interactive --username=jrandom --password=rayjandom"
# Make a working copy of the slave.
$svn checkout $SLAVE_URL $HTTPD_ROOT/wc
cd $HTTPD_ROOT/wc
# Add a new file named newfile and commit it.
touch branch/newfile
$svn add branch/newfile
$svn commit -mm

say "De-activating post-commit hook on $MASTER_REPOS to make $SLAVE_REPOS go out of sync"
mv "$MASTER_REPOS/hooks/post-commit" "$MASTER_REPOS/hooks/post-commit_"

echo "Change made to file in branch" > $HTTPD_ROOT/wc/branch/newfile
$svn ci -m "Commit from slave"

MASTER_HEAD=`$SVNLOOK youngest "$MASTER_REPOS"`
SLAVE_HEAD=`$SVNLOOK youngest "$SLAVE_REPOS"`
say "Now the slave is at r$SLAVE_HEAD and master is at r$MASTER_HEAD."

# Now any other commit operation will fail with an out-of-date error

$svn cp -m "Creating a branch" ^/trunk ^/branch/newbranch --config-option "servers:global:http-library=neon"
RETVAL=$?

if [ $RETVAL -eq 0 ]; then
  say "PASS: Commits succeed even with an out-of-date slave"
else
  say "FAIL: Commits fail with an out-of-date slave"
fi
say "Some house-keeping..."
say "Re-activating the post-commit hook on the master repo: $MASTER_REPOS."
mv "$MASTER_REPOS/hooks/post-commit_" "$MASTER_REPOS/hooks/post-commit"
say "Syncing slave with master."
$SVNSYNC --non-interactive sync "$SYNC_URL" --username=svnsync --password=svnsync 
# shut it down
echo -n "${SCRIPT}: stopping httpd: "
$HTTPD -f $HTTPD_CONFIG -k stop
echo "."
exit 0
