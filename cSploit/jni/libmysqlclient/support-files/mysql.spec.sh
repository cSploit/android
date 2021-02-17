# Copyright (c) 2000, 2013, Oracle and/or its affiliates. All rights reserved.
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; version 2 of the License.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; see the file COPYING. If not, write to the
# Free Software Foundation, Inc., 51 Franklin St, Fifth Floor, Boston
# MA  02110-1301  USA.

##############################################################################
# Some common macro definitions
##############################################################################

%define mysql_vendor    Oracle and/or its affiliates

%define mysql_version   @VERSION@

%define mysqldatadir    /var/lib/mysql

%define release         1

#
# Macros we use which are not available in all supported versions of RPM
#
# - defined/undefined are missing on RHEL4
#
%if %{expand:%{?defined:0}%{!?defined:1}}
%define defined()       %{expand:%%{?%{1}:1}%%{!?%{1}:0}}
%endif
%if %{expand:%{?undefined:0}%{!?undefined:1}}
%define undefined()     %{expand:%%{?%{1}:0}%%{!?%{1}:1}}
%endif

# ----------------------------------------------------------------------------
# RPM build tools now automatically detect Perl module dependencies.  This
# detection causes problems as it is broken in some versions, and it also
# provides unwanted dependencies from mandatory scripts in our package.
# It might not be possible to disable this in all versions of RPM, but here we
# try anyway.  We keep the "AutoReqProv: no" for the "test" sub package, as
# disabling here might fail, and that package has the most problems.
# See:
#  http://fedoraproject.org/wiki/Packaging/Perl#Filtering_Requires:_and_Provides
#  http://www.wideopen.com/archives/rpm-list/2002-October/msg00343.html
# ----------------------------------------------------------------------------
%undefine __perl_provides
%undefine __perl_requires

##############################################################################
# Command line handling
##############################################################################
#
# To set options:
#
#   $ rpmbuild --define="option <x>" ...
#

# ----------------------------------------------------------------------------
# Commercial builds
# ----------------------------------------------------------------------------
%if %{undefined commercial}
%define commercial 0
%endif

# ----------------------------------------------------------------------------
# Source name
# ----------------------------------------------------------------------------
%if %{commercial}
%define product_suffix          -commercial
%define license_type            Commercial
%else
%define product_suffix          %{nil}
%define license_type            GPL
%endif

%define src_dir mysql-connector-c%{product_suffix}-%{mysql_version}-src

# ----------------------------------------------------------------------------
# Feature set (storage engines, options).  Default to community (everything)
# ----------------------------------------------------------------------------
%if %{undefined feature_set}
%define feature_set community
%endif

# ----------------------------------------------------------------------------
# Server comment strings
# ----------------------------------------------------------------------------
%if %{undefined compilation_comment_debug}
%define compilation_comment_debug       MySQL Connector/C - Debug (%{license_type})
%endif
%if %{undefined compilation_comment_release}
%define compilation_comment_release     MySQL Connector/C (%{license_type})
%endif

# ----------------------------------------------------------------------------
# Product and server suffixes
# ----------------------------------------------------------------------------
%if %{undefined product_suffix}
  %if %{defined short_product_tag}
    %define product_suffix      -%{short_product_tag}
  %else
    %define product_suffix      %{nil}
  %endif
%endif

%if %{undefined server_suffix}
%define server_suffix   %{nil}
%endif

# ----------------------------------------------------------------------------
# Distribution support
# ----------------------------------------------------------------------------
%if %{undefined distro_specific}
%define distro_specific 0
%endif
%if %{distro_specific}
  %if %(test -f /etc/enterprise-release && echo 1 || echo 0)
    %define oelver %(rpm -qf --qf '%%{version}\\n' /etc/enterprise-release | sed -e 's/^\\([0-9]*\\).*/\\1/g')
    %if "%oelver" == "4"
      %define distro_description        Oracle Enterprise Linux 4
      %define distro_releasetag         oel4
      %define distro_buildreq           gcc-c++ gperf ncurses-devel perl time zlib-devel
      %define distro_requires           chkconfig coreutils grep procps shadow-utils net-tools
    %else
      %if "%oelver" == "5"
        %define distro_description      Oracle Enterprise Linux 5
        %define distro_releasetag       oel5
        %define distro_buildreq         gcc-c++ gperf ncurses-devel perl time zlib-devel
        %define distro_requires         chkconfig coreutils grep procps shadow-utils net-tools
      %else
        %{error:Oracle Enterprise Linux %{oelver} is unsupported}
      %endif
    %endif
  %else
    %if %(test -f /etc/oracle-release && echo 1 || echo 0)
      %define elver %(rpm -qf --qf '%%{version}\\n' /etc/oracle-release | sed -e 's/^\\([0-9]*\\).*/\\1/g')
      %if "%elver" == "6"
        %define distro_description      Oracle Linux 6
        %define distro_releasetag       el6
        %define distro_buildreq         gcc-c++ ncurses-devel perl time zlib-devel
        %define distro_requires         chkconfig coreutils grep procps shadow-utils net-tools
      %else
        %{error:Oracle Linux %{elver} is unsupported}
      %endif
    %else
      %if %(test -f /etc/redhat-release && echo 1 || echo 0)
        %define rhelver %(rpm -qf --qf '%%{version}\\n' /etc/redhat-release | sed -e 's/^\\([0-9]*\\).*/\\1/g')
        %if "%rhelver" == "4"
          %define distro_description      Red Hat Enterprise Linux 4
          %define distro_releasetag       rhel4
          %define distro_buildreq         gcc-c++ gperf ncurses-devel perl time zlib-devel
          %define distro_requires         chkconfig coreutils grep procps shadow-utils net-tools
        %else
          %if "%rhelver" == "5"
            %define distro_description    Red Hat Enterprise Linux 5
            %define distro_releasetag     rhel5
            %define distro_buildreq       gcc-c++ gperf ncurses-devel perl time zlib-devel
            %define distro_requires       chkconfig coreutils grep procps shadow-utils net-tools
          %else
            %if "%rhelver" == "6"
              %define distro_description    Red Hat Enterprise Linux 6
              %define distro_releasetag     rhel6
              %define distro_buildreq       gcc-c++ ncurses-devel perl time zlib-devel
              %define distro_requires       chkconfig coreutils grep procps shadow-utils net-tools
            %else
              %{error:Red Hat Enterprise Linux %{rhelver} is unsupported}
            %endif
          %endif
        %endif
      %else
        %if %(test -f /etc/SuSE-release && echo 1 || echo 0)
          %define susever %(rpm -qf --qf '%%{version}\\n' /etc/SuSE-release | cut -d. -f1)
          %if "%susever" == "10"
            %define distro_description    SUSE Linux Enterprise Server 10
            %define distro_releasetag     sles10
            %define distro_buildreq       gcc-c++ gdbm-devel gperf ncurses-devel openldap2-client zlib-devel
            %define distro_requires       aaa_base coreutils grep procps pwdutils
          %else
            %if "%susever" == "11"
              %define distro_description  SUSE Linux Enterprise Server 11
              %define distro_releasetag   sles11
              %define distro_buildreq     gcc-c++ gdbm-devel gperf ncurses-devel openldap2-client procps pwdutils zlib-devel
              %define distro_requires     aaa_base coreutils grep procps pwdutils
            %else
              %{error:SuSE %{susever} is unsupported}
            %endif
          %endif
        %else
          %{error:Unsupported distribution}
        %endif
      %endif
    %endif
  %endif
%else
  %define glibc_version %(/lib/libc.so.6 | grep stable | cut -d, -f1 | cut -c38-)
  %define distro_description            Generic Linux (glibc %{glibc_version})
  %define distro_releasetag             linux_glibc%{glibc_version}
  %define distro_buildreq               gcc-c++ gperf ncurses-devel perl  time zlib-devel
  %define distro_requires               coreutils grep procps /sbin/chkconfig /usr/sbin/useradd /usr/sbin/groupadd
%endif

# Avoid debuginfo RPMs, leaves binaries unstripped
%define debug_package   %{nil}

# Hack to work around bug in RHEL5 __os_install_post macro, wrong inverted
# test for __debug_package
%define __strip         /bin/true

##############################################################################
# Configuration based upon above user input, not to be set directly
##############################################################################

%if %{commercial}
%define license_files_server    %{src_dir}/LICENSE.mysql
%else
%define license_files_server    %{src_dir}/COPYING
%endif

##############################################################################
# Main spec file section
##############################################################################

Name:           mysql-connector-c%{product_suffix}
Summary:        MySQL Connector/C: a client library for connecting to the MySQL database server
Group:          Applications/Databases
Version:        @MYSQL_RPM_VERSION@
Release:        %{release}%{?distro_releasetag:.%{distro_releasetag}}
Distribution:   %{distro_description}
License:        Copyright (c) 2000, @MYSQL_COPYRIGHT_YEAR@, %{mysql_vendor}. All rights reserved. Under %{license_type} license as shown in the Description field.
Source:         http://www.mysql.com/Downloads/Connector-C/%{src_dir}.tar.gz
URL:            http://www.mysql.com/
Packager:       MySQL Release Engineering <mysql-build@oss.oracle.com>
Vendor:         %{mysql_vendor}
BuildRequires:  %{distro_buildreq}
Prefix:         /usr

# Think about what you use here since the first step is to
# run a rm -rf
BuildRoot:    %{_tmppath}/%{name}-%{version}-build

# From the manual
%description
This package contains the standard MySQL clients tools, client libraries
and administration tools.

For a description of MySQL see the base MySQL RPM or http://www.mysql.com/
The MySQL(TM) software delivers a very fast, multi-threaded, multi-user,
and robust SQL (Structured Query Language) database server. MySQL Server
is intended for mission-critical, heavy-load production systems as well
as for embedding into mass-deployed software. MySQL is a trademark of
%{mysql_vendor}

The MySQL software has Dual Licensing, which means you can use the MySQL
software free of charge under the GNU General Public License
(http://www.gnu.org/licenses/). You can also purchase commercial MySQL
licenses from %{mysql_vendor} if you do not wish to be bound by the terms of
the GPL. See the chapter "Licensing and Support" in the manual for
further info.

The MySQL web site (http://www.mysql.com/) provides the latest
news and information about the MySQL software. Also please see the
documentation and the manual for more information.

##############################################################################
# Sub package definition
##############################################################################

%package -n mysql-connector-c-devel%{product_suffix}
Summary:	MySQL - Development header files and libraries
Group:		Applications/Databases
Provides:	mysql-connector-c-devel
%if 0%{?commercial}
Obsoletes: mysql-connector-c-devel
%else
Obsoletes: mysql-connector-c-devel-commercial
%endif
Conflicts:	MySQL-devel
Conflicts:	mysql-devel mysql-embedded-devel mysql-devel-advanced mysql-embedded-devel-advanced
Conflicts:      MySQL-devel-classic MySQL-devel-community MySQL-devel-enterprise
Conflicts:      MySQL-devel-advanced MySQL-devel-advanced-gpl MySQL-devel-enterprise-gpl

%description -n mysql-connector-c-devel%{product_suffix}
This package contains the development header files and static libraries
necessary to develop MySQL client applications. MySQL is a trademark
of %{mysql_vendor}

The MySQL software has Dual Licensing, which means you can use the MySQL
software free of charge under the GNU General Public License
(http://www.gnu.org/licenses/). You can also purchase commercial MySQL
licenses from %{mysql_vendor} if you do not wish to be bound by the terms
of the GPL. See the chapter "Licensing and Support" in the manual for
further info.

The MySQL web site (http://www.mysql.com/) provides the latest
news and information about the MySQL software. Also please see the
documentation and the manual for more information.

# ----------------------------------------------------------------------------

%package -n mysql-connector-c-shared%{product_suffix}
Summary:	MySQL - Shared libraries
Group:		Applications/Databases
Provides:	mysql-connector-c-shared
%if 0%{?commercial}
Obsoletes: mysql-connector-c-shared
%else
Obsoletes: mysql-connector-c-shared-commercial
%endif
Conflicts:	MySQL-shared
Conflicts:	MySQL-shared-standard MySQL-shared-pro
Conflicts:	MySQL-shared-pro-cert MySQL-shared-pro-gpl
Conflicts:	MySQL-shared-pro-gpl-cert
Conflicts:	MySQL-shared-classic MySQL-shared-community MySQL-shared-enterprise
Conflicts:	MySQL-shared-advanced MySQL-shared-advanced-gpl MySQL-shared-enterprise-gpl

%description -n mysql-connector-c-shared%{product_suffix}
This package contains the shared libraries (*.so*) which certain
languages and applications need to dynamically load and use MySQL.
MySQL is a trademark of %{mysql_vendor}

The MySQL software has Dual Licensing, which means you can use the MySQL
software free of charge under the GNU General Public License
(http://www.gnu.org/licenses/). You can also purchase commercial MySQL
licenses from %{mysql_vendor} if you do not wish to be bound by the terms
of the GPL. See the chapter "Licensing and Support" in the manual for
further info.

The MySQL web site (http://www.mysql.com/) provides the latest
news and information about the MySQL software. Also please see the
documentation and the manual for more information.

##############################################################################

%prep
%setup -T -a 0 -c -n %{src_dir}

##############################################################################
%build

# Be strict about variables, bail at earliest opportunity, etc.
set -eu

# Optional package files
touch optional-files-devel

#
# Set environment in order of preference, MYSQL_BUILD_* first, then variable
# name, finally a default.  RPM_OPT_FLAGS is assumed to be a part of the
# default RPM build environment.
#

# This is a hack, $RPM_OPT_FLAGS on ia64 hosts contains flags which break
# the compile in cmd-line-utils/libedit - needs investigation, but for now
# we simply unset it and use those specified directly in cmake.
%if "%{_arch}" == "ia64"
RPM_OPT_FLAGS=
%endif

export PATH=${MYSQL_BUILD_PATH:-$PATH}
export CC=${MYSQL_BUILD_CC:-${CC:-gcc}}
export CXX=${MYSQL_BUILD_CXX:-${CXX:-g++}}
export CFLAGS=${MYSQL_BUILD_CFLAGS:-${CFLAGS:-$RPM_OPT_FLAGS}}
export CXXFLAGS=${MYSQL_BUILD_CXXFLAGS:-${CXXFLAGS:-$RPM_OPT_FLAGS -felide-constructors}}
export LDFLAGS=${MYSQL_BUILD_LDFLAGS:-${LDFLAGS:-}}
export CMAKE=${MYSQL_BUILD_CMAKE:-${CMAKE:-cmake}}
export MAKE_JFLAG=${MYSQL_BUILD_MAKE_JFLAG:-}

# By default, a build will include the bundeled "yaSSL" library for SSL.
# However, there may be a need to override.
# Protect against undefined variables if there is no override option.
%if %{undefined with_ssl}
%define ssl_option   %{nil}
%else
%define ssl_option   -DWITH_SSL=%{with_ssl}
%endif

# Build debug enabled binaries
mkdir debug
(
  cd debug
  # Attempt to remove any optimisation flags from the debug build
  CFLAGS=`echo " ${CFLAGS} " | \
            sed -e 's/ -O[0-9]* / /' \
                -e 's/ -unroll2 / /' \
                -e 's/ -ip / /' \
                -e 's/^ //' \
                -e 's/ $//'`
  CXXFLAGS=`echo " ${CXXFLAGS} " | \
              sed -e 's/ -O[0-9]* / /' \
                  -e 's/ -unroll2 / /' \
                  -e 's/ -ip / /' \
                  -e 's/^ //' \
                  -e 's/ $//'`
  # XXX: MYSQL_UNIX_ADDR should be in cmake/* but mysql_version is included before
  # XXX: install_layout so we can't just set it based on INSTALL_LAYOUT=RPM
  ${CMAKE} ../%{src_dir} -DBUILD_CONFIG=mysql_release -DINSTALL_LAYOUT=RPM \
           -DCMAKE_BUILD_TYPE=Debug \
           -DMYSQL_UNIX_ADDR="%{mysqldatadir}/mysql.sock" \
           -DFEATURE_SET="%{feature_set}" \
           %{ssl_option} \
           -DCOMPILATION_COMMENT="%{compilation_comment_debug}" \
           -DMYSQL_SERVER_SUFFIX="%{server_suffix}"
  echo BEGIN_DEBUG_CONFIG ; egrep '^#define' include/config.h ; echo END_DEBUG_CONFIG
  make ${MAKE_JFLAG} VERBOSE=1
)
# Build full release
mkdir release
(
  cd release
  # XXX: MYSQL_UNIX_ADDR should be in cmake/* but mysql_version is included before
  # XXX: install_layout so we can't just set it based on INSTALL_LAYOUT=RPM
  ${CMAKE} ../%{src_dir} -DBUILD_CONFIG=mysql_release -DINSTALL_LAYOUT=RPM \
           -DCMAKE_BUILD_TYPE=RelWithDebInfo \
           -DMYSQL_UNIX_ADDR="%{mysqldatadir}/mysql.sock" \
           -DFEATURE_SET="%{feature_set}" \
           %{ssl_option} \
           -DCOMPILATION_COMMENT="%{compilation_comment_release}" \
           -DMYSQL_SERVER_SUFFIX="%{server_suffix}"
  echo BEGIN_NORMAL_CONFIG ; egrep '^#define' include/config.h ; echo END_NORMAL_CONFIG
  make ${MAKE_JFLAG} VERBOSE=1
)

##############################################################################
%install

RBR=$RPM_BUILD_ROOT
MBD=$RPM_BUILD_DIR/%{src_dir}

# Ensure that needed directories exists
install -d $RBR%{_includedir}
install -d $RBR%{_libdir}
install -d $RBR%{_libdir}/mysql
install -d $RBR%{_mandir}

# Install all binaries
(
  cd $MBD/release
  make DESTDIR=$RBR install
)

# FIXME: at some point we should stop doing this and just install everything
# FIXME: directly into %{_libdir}/mysql - perhaps at the same time as renaming
# FIXME: the shared libraries to use libmysql*-$major.$minor.so syntax
mv -v $RBR/%{_libdir}/*.a $RBR/%{_libdir}/mysql/

# Remove binaries that in MySQL Server RPMs belongs to the server
# sub RPM, and then will conflict/overwrite
# FIXME move to lib/mysql or remove if/when trying to coexist
# with the server RPM?
#rm -f $RBR/%{_bindir}/my_print_defaults
#rm -f $RBR/%{_bindir}/perror

# ----------------------------------------------------------------------
# Clean up the BuildRoot after build is done
# ----------------------------------------------------------------------
%clean
[ "$RPM_BUILD_ROOT" != "/" ] && [ -d $RPM_BUILD_ROOT ] \
  && rm -rf $RPM_BUILD_ROOT;

##############################################################################
#  Files section
##############################################################################

%files -n mysql-connector-c-devel%{product_suffix} -f optional-files-devel
%defattr(-, root, root, 0755)
%attr(755, root, root) %{_bindir}/mysql_config
%attr(755, root, root) %{_bindir}/my_print_defaults
%attr(755, root, root) %{_bindir}/perror
%dir %attr(755, root, root) %{_includedir}/mysql
%dir %attr(755, root, root) %{_libdir}/mysql
%{_includedir}/mysql/*
%{_libdir}/mysql/libmysqlclient.a
%{_libdir}/mysql/libmysqlclient_r.a

# ----------------------------------------------------------------------------
%files -n mysql-connector-c-shared%{product_suffix}
%defattr(-, root, root, 0755)
%doc %{src_dir}/README
%doc %{src_dir}/Docs/ChangeLog
%doc %{license_files_server}
%doc %{src_dir}/Docs/INFO_SRC*
%doc release/Docs/INFO_BIN*
%{_libdir}/libmysql*.so*

%post -n mysql-connector-c-shared%{product_suffix}
/sbin/ldconfig

%postun -n mysql-connector-c-shared%{product_suffix}
/sbin/ldconfig

##############################################################################
# The spec file changelog only includes changes made to the spec file
# itself - note that they must be ordered by date (important when
# merging BK trees)
##############################################################################
%changelog
* Thu Aug 01 2013 Bjorn Munch <bjorn.munch@oracle.com>

- Added ChangeLog

* Fri Mar 22 2013 Kent Boortz <kent.boortz@oracle.com>

- Modified to fit MySQL Connector/C

* Mon Nov 05 2012 Joerg Bruehe <joerg.bruehe@oracle.com>

- Allow to override the default to use the bundled yaSSL by an option like
      --define="with_ssl /path/to/ssl"

* Wed Oct 10 2012 Bjorn Munch <bjorn.munch@oracle.com>

- Replace old my-*.cnf config file examples with template my-default.cnf

* Fri Oct 05 2012 Joerg Bruehe <joerg.bruehe@oracle.com>

- Let the installation use the new option "--random-passwords" of "mysql_install_db".
  (Bug# 12794345 Ensure root password)
- Fix an inconsistency: "new install" vs "upgrade" are told from the (non)existence
  of "$mysql_datadir/mysql" (holding table "mysql.user" and other system stuff).

* Tue Jul 24 2012 Joerg Bruehe <joerg.bruehe@oracle.com>

- Add a macro "runselftest":
  if set to 1 (default), the test suite will be run during the RPM build;
  this can be oveeridden via the command line by adding
      --define "runselftest 0"
  Failures of the test suite will NOT make the RPM build fail!

* Mon Jul 16 2012 Joerg Bruehe <joerg.bruehe@oracle.com>

- Add the man page for the "mysql_config_editor".

* Mon Jun 11 2012 Joerg Bruehe <joerg.bruehe@oracle.com>

- Make sure newly added "SPECIFIC-ULN/" directory does not disturb packaging.

* Wed Feb 29 2012 Brajmohan Saxena <brajmohan.saxena@oracle.com>

- Removal all traces of the readline library from mysql (BUG 13738013)

* Wed Sep 28 2011 Joerg Bruehe <joerg.bruehe@oracle.com>

- Fix duplicate mentioning of "mysql_plugin" and its manual page,
  it is better to keep alphabetic order in the files list (merging!).
  
* Wed Sep 14 2011 Joerg Bruehe <joerg.bruehe@oracle.com>

- Let the RPM capabilities ("obsoletes" etc) ensure that an upgrade may replace
  the RPMs of any configuration (of the current or the preceding release series)
  by the new ones. This is done by not using the implicitly generated capabilities
  (which include the configuration name) and relying on more generic ones which
  just list the function ("server", "client", ...).
  The implicit generation cannot be prevented, so all these capabilities must be
  explicitly listed in "Obsoletes:"

* Tue Sep 13 2011 Jonathan Perkin <jonathan.perkin@oracle.com>

- Add support for Oracle Linux 6 and Red Hat Enterprise Linux 6.  Due to
  changes in RPM behaviour ($RPM_BUILD_ROOT is removed prior to install)
  this necessitated a move of the libmygcc.a installation to the install
  phase, which is probably where it belonged in the first place.

* Tue Sep 13 2011 Joerg Bruehe <joerg.bruehe@oracle.com>

- "make_win_bin_dist" and its manual are dropped, cmake does it different.

* Thu Sep 08 2011 Daniel Fischer <daniel.fischer@oracle.com>

- Add mysql_plugin man page.

* Tue Aug 30 2011 Tor Didriksen <tor.didriksen@oracle.com>

- Set CXX=g++ by default to add a dependency on libgcc/libstdc++.
  Also, remove the use of the -fno-exceptions and -fno-rtti flags.
  TODO: update distro_buildreq/distro_requires

* Tue Aug 30 2011 Joerg Bruehe <joerg.bruehe@oracle.com>

- Add the manual page for "mysql_plugin" to the server package.

* Fri Aug 19 2011 Joerg Bruehe <joerg.bruehe@oracle.com>

- Null-upmerge the fix of bug#37165: This spec file is not affected.
- Replace "/var/lib/mysql" by the spec file variable "%{mysqldatadir}".

* Fri Aug 12 2011 Daniel Fischer <daniel.fischer@oracle.com>

- Source plugin library files list from cmake-generated file.

* Mon Jul 25 2011 Chuck Bell <chuck.bell@oracle.com>

- Added the mysql_plugin client - enables or disables plugins.

* Thu Jul 21 2011 Sunanda Menon <sunanda.menon@oracle.com>

- Fix bug#12561297: Added the MySQL embedded binary

* Thu Jul 07 2011 Joerg Bruehe <joerg.bruehe@oracle.com>

- Fix bug#45415: "rpm upgrade recreates test database"
  Let the creation of the "test" database happen only during a new installation,
  not in an RPM upgrade.
  This affects both the "mkdir" and the call of "mysql_install_db".

* Thu Feb 09 2011 Joerg Bruehe <joerg.bruehe@oracle.com>

- Fix bug#56581: If an installation deviates from the default file locations
  ("datadir" and "pid-file"), the mechanism to detect a running server (on upgrade)
  should still work, and use these locations.
  The problem was that the fix for bug#27072 did not check for local settings.
  
* Mon Jan 31 2011 Joerg Bruehe <joerg.bruehe@oracle.com>

- Install the new "manifest" files: "INFO_SRC" and "INFO_BIN".

* Tue Nov 23 2010 Jonathan Perkin <jonathan.perkin@oracle.com>

- EXCEPTIONS-CLIENT has been deleted, remove it from here too
- Support MYSQL_BUILD_MAKE_JFLAG environment variable for passing
  a '-j' argument to make.

* Mon Nov 1 2010 Georgi Kodinov <georgi.godinov@oracle.com>

- Added test authentication (WL#1054) plugin binaries

* Wed Oct 6 2010 Georgi Kodinov <georgi.godinov@oracle.com>

- Added example external authentication (WL#1054) plugin binaries

* Wed Aug 11 2010 Joerg Bruehe <joerg.bruehe@oracle.com>

- With a recent spec file cleanup, names have changed: A "-community" part was dropped.
  Reflect that in the "Obsoletes" specifications.
- Add a "triggerpostun" to handle the uninstall of the "-community" server RPM.
- This fixes bug#55015 "MySQL server is not restarted properly after RPM upgrade".

* Tue Jun 15 2010 Joerg Bruehe <joerg.bruehe@sun.com>

- Change the behaviour on installation and upgrade:
  On installation, do not autostart the server.
  *Iff* the server was stopped before the upgrade is started, this is taken as a
  sign the administrator is handling that manually, and so the new server will
  not be started automatically at the end of the upgrade.
  The start/stop scripts will still be installed, so the server will be started
  on the next machine boot.
  This is the 5.5 version of fixing bug#27072 (RPM autostarting the server).

* Tue Jun 1 2010 Jonathan Perkin <jonathan.perkin@oracle.com>

- Implement SELinux checks from distribution-specific spec file.

* Wed May 12 2010 Jonathan Perkin <jonathan.perkin@oracle.com>

- Large number of changes to build using CMake
- Introduce distribution-specific RPMs
- Drop debuginfo, build all binaries with debug/symbols
- Remove __os_install_post, use native macro
- Remove _unpackaged_files_terminate_build, make it an error to have
  unpackaged files
- Remove cluster RPMs

* Wed Mar 24 2010 Joerg Bruehe <joerg.bruehe@sun.com>

- Add "--with-perfschema" to the configure options.

* Mon Mar 22 2010 Joerg Bruehe <joerg.bruehe@sun.com>

- User "usr/lib*" to allow for both "usr/lib" and "usr/lib64",
  mask "rmdir" return code 1.
- Remove "ha_example.*" files from the list, they aren't built.

* Wed Mar 17 2010 Joerg Bruehe <joerg.bruehe@sun.com>

- Fix a wrong path name in handling the debug plugins.

* Wed Mar 10 2010 Joerg Bruehe <joerg.bruehe@sun.com>

- Take the result of the debug plugin build and put it into the optimized tree,
  so that it becomes part of the final installation;
  include the files in the packlist. Part of the fixes for bug#49022.

* Mon Mar 01 2010 Joerg Bruehe <joerg.bruehe@sun.com>

- Set "Oracle and/or its affiliates" as the vendor and copyright owner,
  accept upgrading from packages showing MySQL or Sun as vendor.

* Fri Feb 12 2010 Joerg Bruehe <joerg.bruehe@sun.com>

- Formatting changes:
  Have a consistent structure of separator lines and of indentation
  (8 leading blanks => tab).
- Introduce the variable "src_dir".
- Give the environment variables "MYSQL_BUILD_CC(CXX)" precedence
  over "CC" ("CXX").
- Drop the old "with_static" argument analysis, this is not supported
  in 5.1 since ages.
- Introduce variables to control the handlers individually, as well
  as other options.
- Use the new "--with-plugin" notation for the table handlers.
- Drop handling "/etc/rc.d/init.d/mysql", the switch to "/etc/init.d/mysql"
  was done back in 2002 already.
- Make "--with-zlib-dir=bundled" the default, add an option to disable it.
- Add missing manual pages to the file list.
- Improve the runtime check for "libgcc.a", protect it against being tried
  with the Intel compiler "icc".

* Mon Jan 11 2010 Joerg Bruehe <joerg.bruehe@sun.com>

- Change RPM file naming:
  - Suffix like "-m2", "-rc" becomes part of version as "_m2", "_rc".
  - Release counts from 1, not 0.

* Wed Dec 23 2009 Joerg Bruehe <joerg.bruehe@sun.com>

- The "semisync" plugin file name has lost its introductory "lib",
  adapt the file lists for the subpackages.
  This is a part missing from the fix for bug#48351.
- Remove the "fix_privilege_tables" manual, it does not exist in 5.5
  (and likely, the whole script will go, too).

* Mon Nov 16 2009 Joerg Bruehe <joerg.bruehe@sun.com>

- Fix some problems with the directives around "tcmalloc" (experimental),
  remove erroneous traces of the InnoDB plugin (that is 5.1 only).

* Fri Oct 06 2009 Magnus Blaudd <mvensson@mysql.com>

- Removed mysql_fix_privilege_tables

* Fri Oct 02 2009 Alexander Nozdrin <alexander.nozdrin@sun.com>

- "mysqlmanager" got removed from version 5.4, all references deleted.

* Fri Aug 28 2009 Joerg Bruehe <joerg.bruehe@sun.com>

- Merge up from 5.1 to 5.4: Remove handling for the InnoDB plugin.

* Thu Aug 27 2009 Joerg Bruehe <joerg.bruehe@sun.com>

- This version does not contain the "Instance manager", "mysqlmanager":
  Remove it from the spec file so that packaging succeeds.

* Mon Aug 24 2009 Jonathan Perkin <jperkin@sun.com>

- Add conditionals for bundled zlib and innodb plugin

* Fri Aug 21 2009 Jonathan Perkin <jperkin@sun.com>

- Install plugin libraries in appropriate packages.
- Disable libdaemon_example and ftexample plugins.

* Thu Aug 20 2009 Jonathan Perkin <jperkin@sun.com>

- Update variable used for mysql-test suite location to match source.

* Fri Nov 07 2008 Joerg Bruehe <joerg@mysql.com>

- Correct yesterday's fix, so that it also works for the last flag,
  and fix a wrong quoting: un-quoted quote marks must not be escaped.

* Thu Nov 06 2008 Kent Boortz <kent.boortz@sun.com>

- Removed "mysql_upgrade_shell"
- Removed some copy/paste between debug and normal build

* Thu Nov 06 2008 Joerg Bruehe <joerg@mysql.com>

- Modify CFLAGS and CXXFLAGS such that a debug build is not optimized.
  This should cover both gcc and icc flags.  Fixes bug#40546.

* Fri Aug 29 2008 Kent Boortz <kent@mysql.com>

- Removed the "Federated" storage engine option, and enabled in all

* Tue Aug 26 2008 Joerg Bruehe <joerg@mysql.com>

- Get rid of the "warning: Installed (but unpackaged) file(s) found:"
  Some generated files aren't needed in RPMs:
  - the "sql-bench/" subdirectory
  Some files were missing:
  - /usr/share/aclocal/mysql.m4  ("devel" subpackage)
  - Manual "mysqlbug" ("server" subpackage)
  - Program "innochecksum" and its manual ("server" subpackage)
  - Manual "mysql_find_rows" ("client" subpackage)
  - Script "mysql_upgrade_shell" ("client" subpackage)
  - Program "ndb_cpcd" and its manual ("ndb-extra" subpackage)
  - Manuals "ndb_mgm" + "ndb_restore" ("ndb-tools" subpackage)

* Mon Mar 31 2008 Kent Boortz <kent@mysql.com>

- Made the "Federated" storage engine an option
- Made the "Cluster" storage engine and sub packages an option

* Wed Mar 19 2008 Joerg Bruehe <joerg@mysql.com>

- Add the man pages for "ndbd" and "ndb_mgmd".

* Mon Feb 18 2008 Timothy Smith <tim@mysql.com>

- Require a manual upgrade if the alread-installed mysql-server is
  from another vendor, or is of a different major version.

* Wed May 02 2007 Joerg Bruehe <joerg@mysql.com>

- "ndb_size.tmpl" is not needed any more,
  "man1/mysql_install_db.1" lacked the trailing '*'.

* Sat Apr 07 2007 Kent Boortz <kent@mysql.com>

- Removed man page for "mysql_create_system_tables"

* Wed Mar 21 2007 Daniel Fischer <df@mysql.com>

- Add debug server.

* Mon Mar 19 2007 Daniel Fischer <df@mysql.com>

- Remove Max RPMs; the server RPMs contain a mysqld compiled with all
  features that previously only were built into Max.

* Fri Mar 02 2007 Joerg Bruehe <joerg@mysql.com>

- Add several man pages for NDB which are now created.

* Fri Jan 05 2007 Kent Boortz <kent@mysql.com>

- Put back "libmygcc.a", found no real reason it was removed.

- Add CFLAGS to gcc call with --print-libgcc-file, to make sure the
  correct "libgcc.a" path is returned for the 32/64 bit architecture.

* Mon Dec 18 2006 Joerg Bruehe <joerg@mysql.com>

- Fix the move of "mysqlmanager" to section 8: Directory name was wrong.

* Thu Dec 14 2006 Joerg Bruehe <joerg@mysql.com>

- Include the new man pages for "my_print_defaults" and "mysql_tzinfo_to_sql"
  in the server RPM.
- The "mysqlmanager" man page got moved from section 1 to 8.

* Thu Nov 30 2006 Joerg Bruehe <joerg@mysql.com>

- Call "make install" using "benchdir_root=%{_datadir}",
  because that is affecting the regression test suite as well.

* Thu Nov 16 2006 Joerg Bruehe <joerg@mysql.com>

- Explicitly note that the "MySQL-shared" RPMs (as built by MySQL AB)
  replace "mysql-shared" (as distributed by SuSE) to allow easy upgrading
  (bug#22081).

* Mon Nov 13 2006 Joerg Bruehe <joerg@mysql.com>

- Add "--with-partition" to all server builds.

- Use "--report-features" in one test run per server build.

* Tue Aug 15 2006 Joerg Bruehe <joerg@mysql.com>

- The "max" server is removed from packages, effective from 5.1.12-beta.
  Delete all steps to build, package, or install it.

* Mon Jul 10 2006 Joerg Bruehe <joerg@mysql.com>

- Fix a typing error in the "make" target for the Perl script to run the tests.

* Tue Jul 04 2006 Joerg Bruehe <joerg@mysql.com>

- Use the Perl script to run the tests, because it will automatically check
  whether the server is configured with SSL.

* Tue Jun 27 2006 Joerg Bruehe <joerg@mysql.com>

- move "mysqldumpslow" from the client RPM to the server RPM (bug#20216)

- Revert all previous attempts to call "mysql_upgrade" during RPM upgrade,
  there are some more aspects which need to be solved before this is possible.
  For now, just ensure the binary "mysql_upgrade" is delivered and installed.

* Thu Jun 22 2006 Joerg Bruehe <joerg@mysql.com>

- Close a gap of the previous version by explicitly using
  a newly created temporary directory for the socket to be used
  in the "mysql_upgrade" operation, overriding any local setting.

* Tue Jun 20 2006 Joerg Bruehe <joerg@mysql.com>

- To run "mysql_upgrade", we need a running server;
  start it in isolation and skip password checks.

* Sat May 20 2006 Kent Boortz <kent@mysql.com>

- Always compile for PIC, position independent code.

* Wed May 10 2006 Kent Boortz <kent@mysql.com>

- Use character set "all" when compiling with Cluster, to make Cluster
  nodes independent on the character set directory, and the problem
  that two RPM sub packages both wants to install this directory.

* Mon May 01 2006 Kent Boortz <kent@mysql.com>

- Use "./libtool --mode=execute" instead of searching for the
  executable in current directory and ".libs".

* Fri Apr 28 2006 Kent Boortz <kent@mysql.com>

- Install and run "mysql_upgrade"

* Wed Apr 12 2006 Jim Winstead <jimw@mysql.com>

- Remove sql-bench, and MySQL-bench RPM (will be built as an independent
  project from the mysql-bench repository)

* Tue Apr 11 2006 Jim Winstead <jimw@mysql.com>

- Remove old mysqltestmanager and related programs
* Sat Apr 01 2006 Kent Boortz <kent@mysql.com>

- Set $LDFLAGS from $MYSQL_BUILD_LDFLAGS

* Wed Mar 07 2006 Kent Boortz <kent@mysql.com>

- Changed product name from "Community Edition" to "Community Server"

* Mon Mar 06 2006 Kent Boortz <kent@mysql.com>

- Fast mutexes is now disabled by default, but should be
  used in Linux builds.

* Mon Feb 20 2006 Kent Boortz <kent@mysql.com>

- Reintroduced a max build
- Limited testing of 'debug' and 'max' servers
- Berkeley DB only in 'max'

* Mon Feb 13 2006 Joerg Bruehe <joerg@mysql.com>

- Use "-i" on "make test-force";
  this is essential for later evaluation of this log file.

* Thu Feb 09 2006 Kent Boortz <kent@mysql.com>

- Pass '-static' to libtool, link static with our own libraries, dynamic
  with system libraries.  Link with the bundled zlib.

* Wed Feb 08 2006 Kristian Nielsen <knielsen@mysql.com>

- Modified RPM spec to match new 5.1 debug+max combined community packaging.

* Sun Dec 18 2005 Kent Boortz <kent@mysql.com>

- Added "client/mysqlslap"

* Mon Dec 12 2005 Rodrigo Novo <rodrigo@mysql.com>

- Added zlib to the list of (static) libraries installed
- Added check against libtool wierdness (WRT: sql/mysqld || sql/.libs/mysqld)
- Compile MySQL with bundled zlib
- Fixed %packager name to "MySQL Production Engineering Team"

* Mon Dec 05 2005 Joerg Bruehe <joerg@mysql.com>

- Avoid using the "bundled" zlib on "shared" builds:
  As it is not installed (on the build system), this gives dependency
  problems with "libtool" causing the build to fail.
  (Change was done on Nov 11, but left uncommented.)

* Tue Nov 22 2005 Joerg Bruehe <joerg@mysql.com>

- Extend the file existence check for "init.d/mysql" on un-install
  to also guard the call to "insserv"/"chkconfig".

* Thu Oct 27 2005 Lenz Grimmer <lenz@grimmer.com>

- added more man pages

* Wed Oct 19 2005 Kent Boortz <kent@mysql.com>

- Made yaSSL support an option (off by default)

* Wed Oct 19 2005 Kent Boortz <kent@mysql.com>

- Enabled yaSSL support

* Sat Oct 15 2005 Kent Boortz <kent@mysql.com>

- Give mode arguments the same way in all places
- Moved copy of mysqld.a to "standard" build, but
  disabled it as we don't do embedded yet in 5.0

* Fri Oct 14 2005 Kent Boortz <kent@mysql.com>

- For 5.x, always compile with --with-big-tables
- Copy the config.log file to location outside
  the build tree

* Fri Oct 14 2005 Kent Boortz <kent@mysql.com>

- Removed unneeded/obsolete configure options
- Added archive engine to standard server
- Removed the embedded server from experimental server
- Changed suffix "-Max" => "-max"
- Changed comment string "Max" => "Experimental"

* Thu Oct 13 2005 Lenz Grimmer <lenz@mysql.com>

- added a usermod call to assign a potential existing mysql user to the
  correct user group (BUG#12823)
- Save the perror binary built during Max build so it supports the NDB
  error codes (BUG#13740)
- added a separate macro "mysqld_group" to be able to define the
  user group of the mysql user seperately, if desired.

* Thu Sep 29 2005 Lenz Grimmer <lenz@mysql.com>

- fixed the removing of the RPM_BUILD_ROOT in the %clean section (the
  $RBR variable did not get expanded, thus leaving old build roots behind)

* Thu Aug 04 2005 Lenz Grimmer <lenz@mysql.com>

- Fixed the creation of the mysql user group account in the postinstall
  section (BUG 12348)
- Fixed enabling the Archive storage engine in the Max binary

* Tue Aug 02 2005 Lenz Grimmer <lenz@mysql.com>

- Fixed the Requires: tag for the server RPM (BUG 12233)

* Fri Jul 15 2005 Lenz Grimmer <lenz@mysql.com>

- create a "mysql" user group and assign the mysql user account to that group
  in the server postinstall section. (BUG 10984)

* Tue Jun 14 2005 Lenz Grimmer <lenz@mysql.com>

- Do not build statically on i386 by default, only when adding either "--with
  static" or "--define '_with_static 1'" to the RPM build options. Static
  linking really only makes sense when linking against the specially patched
  glibc 2.2.5.

* Mon Jun 06 2005 Lenz Grimmer <lenz@mysql.com>

- added mysql_client_test to the "bench" subpackage (BUG 10676)
- added the libndbclient static and shared libraries (BUG 10676)

* Wed Jun 01 2005 Lenz Grimmer <lenz@mysql.com>

- use "mysqldatadir" variable instead of hard-coding the path multiple times
- use the "mysqld_user" variable on all occasions a user name is referenced
- removed (incomplete) Brazilian translations
- removed redundant release tags from the subpackage descriptions

* Wed May 25 2005 Joerg Bruehe <joerg@mysql.com>

- Added a "make clean" between separate calls to "BuildMySQL".

* Thu May 12 2005 Guilhem Bichot <guilhem@mysql.com>

- Removed the mysql_tableinfo script made obsolete by the information schema

* Wed Apr 20 2005 Lenz Grimmer <lenz@mysql.com>

- Enabled the "blackhole" storage engine for the Max RPM

* Wed Apr 13 2005 Lenz Grimmer <lenz@mysql.com>

- removed the MySQL manual files (html/ps/texi) - they have been removed
  from the MySQL sources and are now available seperately.

* Mon Apr 4 2005 Petr Chardin <petr@mysql.com>

- old mysqlmanager, mysqlmanagerc and mysqlmanager-pwger renamed into
  mysqltestmanager, mysqltestmanager and mysqltestmanager-pwgen respectively

* Fri Mar 18 2005 Lenz Grimmer <lenz@mysql.com>

- Disabled RAID in the Max binaries once and for all (it has finally been
  removed from the source tree)

* Sun Feb 20 2005 Petr Chardin <petr@mysql.com>

- Install MySQL Instance Manager together with mysqld, touch mysqlmanager
  password file

* Mon Feb 14 2005 Lenz Grimmer <lenz@mysql.com>

- Fixed the compilation comments and moved them into the separate build sections
  for Max and Standard

* Mon Feb 7 2005 Tomas Ulin <tomas@mysql.com>

- enabled the "Ndbcluster" storage engine for the max binary
- added extra make install in ndb subdir after Max build to get ndb binaries
- added packages for ndbcluster storage engine

* Fri Jan 14 2005 Lenz Grimmer <lenz@mysql.com>

- replaced obsoleted "BuildPrereq" with "BuildRequires" instead

* Thu Jan 13 2005 Lenz Grimmer <lenz@mysql.com>

- enabled the "Federated" storage engine for the max binary

* Tue Jan 04 2005 Petr Chardin <petr@mysql.com>

- ISAM and merge storage engines were purged. As well as appropriate
  tools and manpages (isamchk and isamlog)

* Thu Dec 31 2004 Lenz Grimmer <lenz@mysql.com>

- enabled the "Archive" storage engine for the max binary
- enabled the "CSV" storage engine for the max binary
- enabled the "Example" storage engine for the max binary

* Thu Aug 26 2004 Lenz Grimmer <lenz@mysql.com>

- MySQL-Max now requires MySQL-server instead of MySQL (BUG 3860)

* Fri Aug 20 2004 Lenz Grimmer <lenz@mysql.com>

- do not link statically on IA64/AMD64 as these systems do not have
  a patched glibc installed

* Tue Aug 10 2004 Lenz Grimmer <lenz@mysql.com>

- Added libmygcc.a to the devel subpackage (required to link applications
  against the the embedded server libmysqld.a) (BUG 4921)

* Mon Aug 09 2004 Lenz Grimmer <lenz@mysql.com>

- Added EXCEPTIONS-CLIENT to the "devel" package

* Thu Jul 29 2004 Lenz Grimmer <lenz@mysql.com>

- disabled OpenSSL in the Max binaries again (the RPM packages were the
  only exception to this anyway) (BUG 1043)

* Wed Jun 30 2004 Lenz Grimmer <lenz@mysql.com>

- fixed server postinstall (mysql_install_db was called with the wrong
  parameter)

* Thu Jun 24 2004 Lenz Grimmer <lenz@mysql.com>

- added mysql_tzinfo_to_sql to the server subpackage
- run "make clean" instead of "make distclean"

* Mon Apr 05 2004 Lenz Grimmer <lenz@mysql.com>

- added ncurses-devel to the build prerequisites (BUG 3377)

* Thu Feb 12 2004 Lenz Grimmer <lenz@mysql.com>

- when using gcc, _always_ use CXX=gcc
- replaced Copyright with License field (Copyright is obsolete)

* Tue Feb 03 2004 Lenz Grimmer <lenz@mysql.com>

- added myisam_ftdump to the Server package

* Tue Jan 13 2004 Lenz Grimmer <lenz@mysql.com>

- link the mysql client against libreadline instead of libedit (BUG 2289)

* Mon Dec 22 2003 Lenz Grimmer <lenz@mysql.com>

- marked /etc/logrotate.d/mysql as a config file (BUG 2156)

* Fri Dec 13 2003 Lenz Grimmer <lenz@mysql.com>

- fixed file permissions (BUG 1672)

* Thu Dec 11 2003 Lenz Grimmer <lenz@mysql.com>

- made testing for gcc3 a bit more robust

* Fri Dec 05 2003 Lenz Grimmer <lenz@mysql.com>

- added missing file mysql_create_system_tables to the server subpackage

* Fri Nov 21 2003 Lenz Grimmer <lenz@mysql.com>

- removed dependency on MySQL-client from the MySQL-devel subpackage
  as it is not really required. (BUG 1610)

* Fri Aug 29 2003 Lenz Grimmer <lenz@mysql.com>

- Fixed BUG 1162 (removed macro names from the changelog)
- Really fixed BUG 998 (disable the checking for installed but
  unpackaged files)

* Tue Aug 05 2003 Lenz Grimmer <lenz@mysql.com>

- Fixed BUG 959 (libmysqld not being compiled properly)
- Fixed BUG 998 (RPM build errors): added missing files to the
  distribution (mysql_fix_extensions, mysql_tableinfo, mysqldumpslow,
  mysql_fix_privilege_tables.1), removed "-n" from install section.

* Wed Jul 09 2003 Lenz Grimmer <lenz@mysql.com>

- removed the GIF Icon (file was not included in the sources anyway)
- removed unused variable shared_lib_version
- do not run automake before building the standard binary
  (should not be necessary)
- add server suffix '-standard' to standard binary (to be in line
  with the binary tarball distributions)
- Use more RPM macros (_exec_prefix, _sbindir, _libdir, _sysconfdir,
  _datadir, _includedir) throughout the spec file.
- allow overriding CC and CXX (required when building with other compilers)

* Fri May 16 2003 Lenz Grimmer <lenz@mysql.com>

- re-enabled RAID again

* Wed Apr 30 2003 Lenz Grimmer <lenz@mysql.com>

- disabled MyISAM RAID (--with-raid) - it throws an assertion which
  needs to be investigated first.

* Mon Mar 10 2003 Lenz Grimmer <lenz@mysql.com>

- added missing file mysql_secure_installation to server subpackage
  (BUG 141)

* Tue Feb 11 2003 Lenz Grimmer <lenz@mysql.com>

- re-added missing pre- and post(un)install scripts to server subpackage
- added config file /etc/my.cnf to the file list (just for completeness)
- make sure to create the datadir with 755 permissions

* Mon Jan 27 2003 Lenz Grimmer <lenz@mysql.com>

- removed unused CC and CXX variables
- CFLAGS and CXXFLAGS should honor RPM_OPT_FLAGS

* Fri Jan 24 2003 Lenz Grimmer <lenz@mysql.com>

- renamed package "MySQL" to "MySQL-server"
- fixed Copyright tag
- added mysql_waitpid to client subpackage (required for mysql-test-run)

* Wed Nov 27 2002 Lenz Grimmer <lenz@mysql.com>

- moved init script from /etc/rc.d/init.d to /etc/init.d (the majority of
  Linux distributions now support this scheme as proposed by the LSB either
  directly or via a compatibility symlink)
- Use new "restart" init script action instead of starting and stopping
  separately
- Be more flexible in activating the automatic bootup - use insserv (on
  older SuSE versions) or chkconfig (Red Hat, newer SuSE versions and
  others) to create the respective symlinks

* Wed Sep 25 2002 Lenz Grimmer <lenz@mysql.com>

- MySQL-Max now requires MySQL >= 4.0 to avoid version mismatches
  (mixing 3.23 and 4.0 packages)

* Fri Aug 09 2002 Lenz Grimmer <lenz@mysql.com>

- Turn off OpenSSL in MySQL-Max for now until it works properly again
- enable RAID for the Max binary instead
- added compatibility link: safe_mysqld -> mysqld_safe to ease the
  transition from 3.23

* Thu Jul 18 2002 Lenz Grimmer <lenz@mysql.com>

- Reworked the build steps a little bit: the Max binary is supposed
  to include OpenSSL, which cannot be linked statically, thus trying
  to statically link against a special glibc is futile anyway
- because of this, it is not required to make yet another build run
  just to compile the shared libs (saves a lot of time)
- updated package description of the Max subpackage
- clean up the BuildRoot directory afterwards

* Mon Jul 15 2002 Lenz Grimmer <lenz@mysql.com>

- Updated Packager information
- Fixed the build options: the regular package is supposed to
  include InnoDB and linked statically, while the Max package
  should include BDB and SSL support

* Fri May 03 2002 Lenz Grimmer <lenz@mysql.com>

- Use more RPM macros (e.g. infodir, mandir) to make the spec
  file more portable
- reorganized the installation of documentation files: let RPM
  take care of this
- reorganized the file list: actually install man pages along
  with the binaries of the respective subpackage
- do not include libmysqld.a in the devel subpackage as well, if we
  have a special "embedded" subpackage
- reworked the package descriptions

* Mon Oct  8 2001 Monty

- Added embedded server as a separate RPM

* Fri Apr 13 2001 Monty

- Added mysqld-max to the distribution

* Tue Jan 2  2001  Monty

- Added mysql-test to the bench package

* Fri Aug 18 2000 Tim Smith <tim@mysql.com>

- Added separate libmysql_r directory; now both a threaded
  and non-threaded library is shipped.

* Wed Sep 28 1999 David Axmark <davida@mysql.com>

- Added the support-files/my-example.cnf to the docs directory.

- Removed devel dependency on base since it is about client
  development.

* Wed Sep 8 1999 David Axmark <davida@mysql.com>

- Cleaned up some for 3.23.

* Thu Jul 1 1999 David Axmark <davida@mysql.com>

- Added support for shared libraries in a separate sub
  package. Original fix by David Fox (dsfox@cogsci.ucsd.edu)

- The --enable-assembler switch is now automatically disables on
  platforms there assembler code is unavailable. This should allow
  building this RPM on non i386 systems.

* Mon Feb 22 1999 David Axmark <david@detron.se>

- Removed unportable cc switches from the spec file. The defaults can
  now be overridden with environment variables. This feature is used
  to compile the official RPM with optimal (but compiler version
  specific) switches.

- Removed the repetitive description parts for the sub rpms. Maybe add
  again if RPM gets a multiline macro capability.

- Added support for a pt_BR translation. Translation contributed by
  Jorge Godoy <jorge@bestway.com.br>.

* Wed Nov 4 1998 David Axmark <david@detron.se>

- A lot of changes in all the rpm and install scripts. This may even
  be a working RPM :-)

* Sun Aug 16 1998 David Axmark <david@detron.se>

- A developers changelog for MySQL is available in the source RPM. And
  there is a history of major user visible changed in the Reference
  Manual.  Only RPM specific changes will be documented here.
