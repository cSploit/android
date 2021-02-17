# To build a static RPM, add
#     --define "static 1"
# to the rpmbuild command line. To build without Ncat, add
#     --define "buildncat 0"
# To build without Nping, add
#     --define "buildnping 0"
#
# To specify openssl dir, add something like:
#     --define "openssl /usr/local/ssl"

%define name nmap
%define version 6.46
%define release 1
%define _prefix /usr

# Find where Python modules are installed. See
# http://fedoraproject.org/wiki/Packaging/Python.
%{!?python_sitelib: %define python_sitelib %(%{__python} -c "from distutils.sysconfig import get_python_lib; print get_python_lib()")}

Summary: Network exploration tool and security scanner
Name: %{name}
Version: %{version}
Release: %{release}
Epoch: 2
License: http://nmap.org/man/man-legal.html
Group: Applications/System
Source0: http://nmap.org/dist/%{name}-%{version}.tgz
URL: http://nmap.org

# Disable automatic dependency calculation because we want to provide
# packages for more than one version of Python. Without this, RPM will
# grep through the installed files and automatically add things like
#   Requires: python(abi) = 2.4
# setup.py takes care of adjusting sys.path to point to wherever the
# modules were installed so it's not dependent on any particular ABI.
AutoReqProv: no
# For Ndiff.
Requires: python >= 2.4

# RPM can't be relocatable until I stop storing path info in the binary.
# Prefix: %{_prefix}
BuildRoot: %{_tmppath}/%{name}-root

%description

Nmap ("Network Mapper") is a free and open source utility
for network exploration or security auditing. Many systems and network
administrators also find it useful for tasks such as network
inventory, managing service upgrade schedules, and monitoring host or
service uptime. Nmap uses raw IP packets in novel ways to determine
what hosts are available on the network, what services (application
name and version) those hosts are offering, what operating systems
(and OS versions) they are running, what type of packet
filters/firewalls are in use, and dozens of other characteristics. It
was designed to rapidly scan large networks, but works fine against
single hosts. Nmap runs on all major computer operating systems, and
both console and graphical versions are available.

%prep
%setup -q

%build

%configure --with-openssl=%{openssl} --without-zenmap --with-ndiff --with-nmap-update --with-libdnet=included --with-libpcap=included --with-libpcre=included --with-liblua=included
%if "%{buildncat}" == "0"
%configure --without-ncat
%endif
%if "%{buildnping}" == "0"
%configure --without-nping
%endif
%if "%{static}" == "1"
make static
%else
make
%endif

%install
[ "$RPM_BUILD_ROOT" != "/" ] && rm -rf $RPM_BUILD_ROOT
make install DESTDIR=$RPM_BUILD_ROOT
strip $RPM_BUILD_ROOT%{_bindir}/* || :
gzip $RPM_BUILD_ROOT%{_mandir}/man1/* || :

%clean
[ "$RPM_BUILD_ROOT" != "/" ] && rm -rf $RPM_BUILD_ROOT

%files 
%defattr(-,root,root)
%doc COPYING
%doc docs/README
%doc docs/nmap.usage.txt
%doc %{_prefix}/share/man/man1/nmap.1.gz
%doc %{_prefix}/share/man/*/man1/nmap.1.gz
%{_bindir}/nmap
%{_datadir}/nmap

%{_bindir}/ndiff
# Ndiff is now a module and has e.g. /usr/lib/python2.4/site-packages/ndiff.py, /usr/lib/python2.4/site-packages/ndiff.pyc 
%{python_sitelib}/ndiff*
%doc %{_prefix}/share/man/man1/ndiff.1.gz

# Ncat subpackage
%if "%{buildncat}" != "0"
%package -n ncat
Summary: Nmap's Netcat replacement
Group: Applications/System

%description -n ncat
Ncat is a feature packed networking utility which will read and
write data across a network from the command line.  It uses both
TCP and UDP for communication and is designed to be a reliable
back-end tool to instantly provide network connectivity to other
applications and users. Ncat will not only work with IPv4 and IPv6
but provides the user with a virtually limitless number of potential
uses.

%files -n ncat
%defattr(-,root,root)
%doc %{_prefix}/share/man/man1/ncat.1.gz
%{_bindir}/ncat
%{_datadir}/ncat

%endif

# Nping subpackage
%if "%{buildnping}" != "0"
%package -n nping
# If this 0. prefix is removed it must also be removed from Makefile.in.
Version: 0.%{version}
Summary: Nping packet generator
Group: Applications/System

%description -n nping
Nping is an open source tool for network packet generation, response
analysis and response time measurement. Nping allows to generate network
packets of a wide range of protocols, letting users to tune virtually
any field of the protocol headers. While Nping can be used as a simple
ping utility to detect active hosts, it can also be used as a raw packet
generator for network stack stress tests, ARP poisoning, Denial of
Service attacks, route tracing, etc.

%files -n nping
%defattr(-,root,root)
%doc %{_prefix}/share/man/man1/nping.1.gz
%{_bindir}/nping

%endif

# nmap-update subpackage
%package -n nmap-update
Summary: Nmap's updater
Group: Applications/System

Requires: subversion
BuildRequires: subversion-devel

%description -n nmap-update
nmap-update gets the latest versions of architecture-independent
files, such as scripts and databases, for the installed version
of Nmap.

%files -n nmap-update
%defattr(-,root,root)
%doc %{_prefix}/share/man/man1/nmap-update.1.gz
%{_bindir}/nmap-update

%changelog

* Sat Jun 06 2009 Fyodor (fyodor(a)insecure.org)
- Removed changelog entries as SVN is a more authoritative source. Execute:
- svn log --username guest --password "" svn://svn.insecure.org/nmap/nmap.spec.in

