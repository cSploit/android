LIBPCAP 1.x.y
=============

[![Build Status](https://travis-ci.org/the-tcpdump-group/tcpdump.svg?branch=master)](https://travis-ci.org/the-tcpdump-group/libpcap)

[![Build Status](https://ci.appveyor.com/api/projects/status/github/the-tcpdump-group/tcpdump?branch=master&svg=true)](https://ci.appveyor.com/project/guyharris/libpcap)

Now maintained by "The Tcpdump Group"

  https://www.tcpdump.org

formerly from 	Lawrence Berkeley National Laboratory
		Network Research Group <libpcap@ee.lbl.gov>
		ftp://ftp.ee.lbl.gov/old/libpcap-0.4a7.tar.Z

To report a security issue please send an e-mail to security@tcpdump.org.

To report bugs and other problems, contribute patches, request a
feature, provide generic feedback etc please see the file
[CONTRIBUTING.md](CONTRIBUTING.md) in the libpcap source tree root.

The directory doc/ has README files about specific operating systems and
options.

Anonymous Git is available via:

  https://github.com/the-tcpdump-group/libpcap.git

This directory contains source code for libpcap, a system-independent
interface for user-level packet capture.  libpcap provides a portable
framework for low-level network monitoring.  Applications include
network statistics collection, security monitoring, network debugging,
etc.  Since almost every system vendor provides a different interface
for packet capture, and since we've developed several tools that
require this functionality, we've created this system-independent API
to ease in porting and to alleviate the need for several
system-dependent packet capture modules in each application.

For some platforms there are README.{system} files that discuss issues
with the OS's interface for packet capture on those platforms, such as
how to enable support for that interface in the OS, if it's not built in
by default.

The libpcap interface supports a filtering mechanism based on the
architecture in the BSD packet filter.  BPF is described in the 1993
Winter Usenix paper ``The BSD Packet Filter: A New Architecture for
User-level Packet Capture''.  A compressed PostScript version can be
found at

  https://www.tcpdump.org/papers/bpf-usenix93.ps.Z

and a gzipped version can be found at

  https://www.tcpdump.org/papers/bpf-usenix93.ps.gz

A PDF version can be found at

  https://www.tcpdump.org/papers/bpf-usenix93.pdf

Although most packet capture interfaces support in-kernel filtering,
libpcap utilizes in-kernel filtering only for the BPF interface.
On systems that don't have BPF, all packets are read into user-space
and the BPF filters are evaluated in the libpcap library, incurring
added overhead (especially, for selective filters).  Ideally, libpcap
would translate BPF filters into a filter program that is compatible
with the underlying kernel subsystem, but this is not yet implemented.

BPF is standard in 4.4BSD, BSD/OS, NetBSD, FreeBSD, OpenBSD, DragonFly
BSD, and macOS; an older, modified and undocumented version is standard
in AIX.  {DEC OSF/1, Digital UNIX, Tru64 UNIX} uses the packetfilter
interface but has been extended to accept BPF filters (which libpcap
utilizes).  Also, you can add BPF filter support to Ultrix using the
kernel source and/or object patches available in:

  https://www.tcpdump.org/other/bpfext42.tar.Z

Linux has a number of BPF based systems, and libpcap does not support
any of the eBPF mechanisms as yet, although it supports many of the
memory mapped receive mechanisms.
See the [README.linux](doc/README.linux.md) file for more information.

Note to Linux distributions and *BSD systems that include libpcap:

There's now a rule to make a shared library, which should work on Linux
and *BSD, among other platforms.

It sets the soname of the library to "libpcap.so.1"; this is what it
should be, *NOT* libpcap.so.1.x or libpcap.so.1.x.y or something such as
that.

We've been maintaining binary compatibility between libpcap releases for
quite a while; there's no reason to tie a binary linked with libpcap to
a particular release of libpcap.
