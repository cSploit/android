# $Id: Makefile.am,v 1.6 2004/11/09 07:05:07 mike Exp $
#
# Libnet automake information file
# Copyright (c) 1998 - 2004 Mike D. Schiffman <mike@infonexus.com>
# All rights reserved.
#
# Process this file with automake to produce a Makefile.in script.

include $(top_srcdir)/Makefile.am.common

samples = arp cdp dhcp_discover get_addr icmp_timestamp icmp_unreach \
                  ieee isl ntp ospf_hello ospf_lsa ping_of_death stp synflood \
                  tcp1 tcp2 udp1 udp2 synflood6 \
                  icmp6_echoreq \
                  icmp6_unreach \
                  icmp_echo_cq \
                  smurf dot1x dns rpc_tcp rpc_udp mpls icmp_timeexceed \
                  fddi_tcp1 fddi_tcp2 tring_tcp1 tring_tcp2 icmp_redirect \
                  bgp4_hdr bgp4_open bgp4_update bgp4_notification gre \
                  synflood6_frag tftp ip_link ip_raw sebek hsrp \
                  test_ipv4 test_ipv6_icmpv4 test_ipv4_options
if INSTALL_SAMPLES
bin_PROGRAMS = $(samples)
else
noinst_PROGRAMS = $(samples)
endif

arp_SOURCES             = arp.c
cdp_SOURCES             = cdp.c
dot1x_SOURCES           = dot1x.c
dhcp_discover_SOURCES   = dhcp_discover.c
dns_SOURCES             = dns.c
fddi_tcp1_SOURCES       = fddi_tcp1.c
fddi_tcp2_SOURCES       = fddi_tcp2.c
tring_tcp1_SOURCES      = tring_tcp1.c
tring_tcp2_SOURCES      = tring_tcp2.c
get_addr_SOURCES        = get_addr.c
icmp_echo_cq_SOURCES    = icmp_echo_cq.c
icmp_redirect_SOURCES   = icmp_redirect.c
icmp_timestamp_SOURCES  = icmp_timestamp.c
icmp_timeexceed_SOURCES = icmp_timeexceed.c
icmp_unreach_SOURCES    = icmp_unreach.c
ieee_SOURCES            = ieee.c
isl_SOURCES             = isl.c
ntp_SOURCES             = ntp.c
ospf_hello_SOURCES      = ospf_hello.c
ospf_lsa_SOURCES        = ospf_lsa.c
ping_of_death_SOURCES   = ping_of_death.c
mpls_SOURCES            = mpls.c
rpc_tcp_SOURCES         = rpc_tcp.c
rpc_udp_SOURCES         = rpc_udp.c
stp_SOURCES             = stp.c
synflood_SOURCES        = synflood.c
tcp1_SOURCES            = tcp1.c
tcp2_SOURCES            = tcp2.c
tftp_SOURCES            = tftp.c
udp1_SOURCES            = udp1.c
udp2_SOURCES            = udp2.c
icmp6_echoreq_SOURCES   = icmp6_echoreq.c
icmp6_unreach_SOURCES   = icmp6_unreach.c
smurf_SOURCES           = smurf.c
synflood6_SOURCES       = synflood6.c
synflood6_frag_SOURCES  = synflood6_frag.c
bgp4_hdr_SOURCES        = bgp4_hdr.c
bgp4_open_SOURCES       = bgp4_open.c
bgp4_update_SOURCES     = bgp4_update.c
bgp4_notification_SOURCES = bgp4_notification.c
gre_SOURCES		= gre.c
ip_raw_SOURCES          = ip_raw.c
ip_link_SOURCES		= ip_link.c
sebek_SOURCES           = sebek.c
hsrp_SOURCES            = hsrp.c
test_ipv4_SOURCES       = test_ipv4.c
test_ipv4_options_SOURCES = test_ipv4_options.c
test_ipv6_icmpv4_SOURCES = test_ipv6_icmpv4.c

EXTRA_DIST = libnet_test.h

LDADD 			= $(top_builddir)/src/libnet.la
