# By default, Zenmap will be built using whatever version of Python is
# the default on your system. To change this, use something like
#     --define "__python /usr/bin/python2.5"

%define name zenmap
%define version 6.46
%define release 1
%define _prefix /usr

# Find where Python modules are installed. See
# http://fedoraproject.org/wiki/Packaging/Python.
%{!?python_sitelib: %define python_sitelib %(%{__python} -c "from distutils.sysconfig import get_python_lib; print get_python_lib()")}

Summary: Multi-platform graphical Nmap frontend and results viewer
Name: %{name}
Version: %{version}
Release: %{release}
Epoch: 2
License: http://nmap.org/man/man-legal.html
Group: Applications/System
Source0: http://nmap.org/dist/nmap-%{version}.tgz
URL: http://nmap.org
BuildArch: noarch

# Disable automatic dependency calculation because we want to provide
# packages for more than one version of Python. Without this, RPM will
# grep through the installed files and automatically add things like
#   Requires: python(abi) = 2.4
# setup.py takes care of adjusting sys.path to point to wherever the
# modules were installed so it's not dependent on any particular ABI.
AutoReqProv: no
Requires: python >= 2.4, nmap, pygtk2

Prefix: %{_prefix}
BuildRoot: %{_tmppath}/%{name}-root

%description
Zenmap is an Nmap frontend. It is meant to be useful for advanced users
and to make Nmap easy to use by beginners. It was originally derived
from Umit, an Nmap GUI created as part of the Google Summer of Code.

%prep
%setup -q -n nmap-%{version}

%build
# Cannot use configure macro because noarch-redhat-linux is not
# recognized by the auto tools in the tarball.  Upgrading to the
# latest GNU CVS config.sub/config.guess on 3/15/08 didn't fix it.  So
# I'm using this approach, as is done by other projects, such as
# http://mono.ximian.com/monobuild/snapshot/snapshot_packages/noarch/xsp/96614/xsp.spec
# -Fyodor
./configure --prefix=%{_prefix} \
	    --libexecdir=%{_prefix}/lib \
	    --bindir=%{_prefix}/bin \
	    --datadir=%{_prefix}/share \
	    --libdir=%{_prefix}/lib \
	    --mandir=%{_prefix}/share/man \
	    --infodir=%{_prefix}/share/info \
	    --sysconfdir=%{_sysconfdir} \
	    --without-openssl \
	    --with-zenmap PYTHON="%{__python}"
make build-zenmap DESTDIR=$RPM_BUILD_ROOT

%install
[ "$RPM_BUILD_ROOT" != "/" ] && rm -rf $RPM_BUILD_ROOT
make install-zenmap DESTDIR=$RPM_BUILD_ROOT
# Don't package the uninstaller with the RPM.
rm -f $RPM_BUILD_ROOT%{_bindir}/uninstall_zenmap
gzip $RPM_BUILD_ROOT%{_mandir}/man1/* || :

%clean
[ "$RPM_BUILD_ROOT" != "/" ] && rm -rf $RPM_BUILD_ROOT

%files
%defattr(-,root,root)
%doc %{_mandir}/man1/zenmap.1.gz
%{_bindir}/zenmap
%{_bindir}/nmapfe
%{_bindir}/xnmap
# This gets the modules and the .egg-info file if it was installed.
%{python_sitelib}/*
%{_datadir}/zenmap
%{_datadir}/applications/*.desktop

%changelog

* Sat Jun 06 2009 Fyodor (fyodor(a)insecure.org)
- Removed changelog entries as SVN is a more authoritative source. Execute:
- svn log --username guest --password "" svn://svn.insecure.org/nmap/zenmap.spec.in

