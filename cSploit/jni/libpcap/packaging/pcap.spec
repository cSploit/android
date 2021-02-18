%define prefix   /usr
%define version 0.9.4

Summary: packet capture library
Name: libpcap
Version: %version
Release: 1
Group: Development/Libraries
Copyright: BSD
Source: libpcap-0.9.4.tar.gz
BuildRoot: /tmp/%{name}-buildroot
URL: http://www.tcpdump.org

%description
Packet-capture library LIBPCAP 0.9.4
Now maintained by "The Tcpdump Group"
See http://www.tcpdump.org
Please send inquiries/comments/reports to tcpdump-workers@tcpdump.org

%prep
%setup

%post
ldconfig

%build
CFLAGS="$RPM_OPT_FLAGS" ./configure --prefix=%prefix
make

%install
rm -rf $RPM_BUILD_ROOT
mkdir -p $RPM_BUILD_ROOT/usr/{lib,include}
mkdir -p $RPM_BUILD_ROOT/usr/share/man
mkdir -p $RPM_BUILD_ROOT/usr/include/net
mkdir -p $RPM_BUILD_ROOT/usr/man/man3
make install DESTDIR=$RPM_BUILD_ROOT mandir=/usr/share/man
cd $RPM_BUILD_ROOT/usr/lib
V1=`echo 0.9.4 | sed 's/\\.[^\.]*$//g'`
V2=`echo 0.9.4 | sed 's/\\.[^\.]*\.[^\.]*$//g'`
ln -sf libpcap.so.0.9.4 libpcap.so.$V1
if test "$V2" -ne "$V1"; then
    ln -sf libpcap.so.$V1 libpcap.so.$V2
    ln -sf libpcap.so.$V2 libpcap.so
else
    ln -sf libpcap.so.$V1 libpcap.so
fi

#install -m 755 -o root libpcap.a $RPM_BUILD_ROOT/usr/lib
#install -m 644 -o root pcap.3 $RPM_BUILD_ROOT/usr/man/man3
#install -m 644 -o root pcap.h $RPM_BUILD_ROOT/usr/include
#install -m 644 -o root pcap-bpf.h $RPM_BUILD_ROOT/usr/include/net
#install -m 644 -o root pcap-namedb.h $RPM_BUILD_ROOT/usr/include

%clean
rm -rf $RPM_BUILD_ROOT

%files
%defattr(-,root,root)
%doc LICENSE CHANGES INSTALL.txt README.linux TODO VERSION CREDITS packaging/pcap.spec
/usr/lib/libpcap.a
/usr/share/man/man3/*
/usr/include/pcap.h
/usr/include/pcap-bpf.h
/usr/include/pcap-namedb.h
/usr/lib/libpcap.so*
