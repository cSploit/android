# $Id: Makefile.am,v 1.6 2004/11/09 07:05:07 mike Exp $
#
# Libnet automake information file
# Copyright (c) 1998 - 2004 Mike D. Schiffman <mike@infonexus.com>
# All rights reserved.
#
# Process this file with automake to produce a Makefile.in script.

include $(top_srcdir)/Makefile.am.common

EXTRA_DIST = libnet_dll.c

lib_LTLIBRARIES = libnet.la

libnet_la_SOURCES = libnet_asn1.c \
			libnet_build_802.1q.c \
			libnet_build_802.1x.c \
			libnet_build_802.2.c \
			libnet_build_802.3.c \
			libnet_build_arp.c \
			libnet_build_bgp.c \
			libnet_build_cdp.c \
			libnet_build_data.c \
			libnet_build_dhcp.c \
			libnet_build_dns.c \
			libnet_build_ethernet.c \
			libnet_build_fddi.c \
			libnet_build_gre.c \
			libnet_build_hsrp.c \
			libnet_build_icmp.c \
			libnet_build_igmp.c \
			libnet_build_ip.c \
			libnet_build_ipsec.c \
			libnet_build_isl.c \
			libnet_build_link.c \
			libnet_build_mpls.c \
			libnet_build_ntp.c \
			libnet_build_ospf.c \
			libnet_build_rip.c \
			libnet_build_rpc.c \
			libnet_build_sebek.c \
			libnet_build_snmp.c \
			libnet_build_stp.c \
			libnet_build_tcp.c \
			libnet_build_token_ring.c \
			libnet_build_udp.c \
			libnet_build_vrrp.c \
			libnet_advanced.c \
			libnet_checksum.c \
			libnet_cq.c \
			libnet_crc.c \
			libnet_error.c \
			libnet_if_addr.c \
			libnet_init.c \
			libnet_internal.c \
			libnet_pblock.c \
			libnet_port_list.c \
			libnet_prand.c \
			libnet_raw.c \
			libnet_resolve.c \
			libnet_version.c \
			libnet_write.c

libnet_la_LIBADD = @LTLIBOBJS@

# Note: this version-info should NOT simply be the version of the libnet
# package. See:
#   http://www.sourceware.org/autobook/autobook/autobook_91.html
# for the rules.
#
# Summary:
#
# Form is current:revision:age.
#
# New APIs, backwards compatible, increment current and age.
# Bug fix, no new APIs, increment revision.
#
# Note:
# 1.1.2.1-fork is 5:0:4
# 1.1.3 will be 6:0:5  -> new interfaces, backwards compatible
# 1.1.4 was 6:0:5 -> probably an error on my part
# 1.1.5 is 7:0:6 -> new APIs, backwards compat

libnet_la_LDFLAGS = -version-info 7:0:6 -Wl

