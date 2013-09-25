LOCAL_PATH := $(call my-dir)
MY_LOCAL_PATH := $(LOCAL_PATH)
include $(CLEAR_VARS)

LOCAL_SRC_FILES:=\
	addrtoname.c\
	af.c\
	cpack.c\
	gmpls.c\
	oui.c\
	gmt2local.c\
	ipproto.c\
	nlpid.c\
	l2vpn.c\
	machdep.c\
	parsenfsfh.c\
	print-802_11.c\
	print-ap1394.c\
	print-ah.c\
	print-arcnet.c\
	print-aodv.c\
	print-arp.c\
	print-ascii.c\
	print-atalk.c\
	print-atm.c\
	print-beep.c\
	print-bfd.c\
	print-bgp.c\
	print-bootp.c\
	print-cdp.c\
	print-chdlc.c\
	print-cip.c\
	print-cnfp.c\
	print-dccp.c\
	print-dhcp6.c\
	print-decnet.c\
	print-domain.c\
	print-dvmrp.c\
	print-enc.c\
	print-egp.c\
	print-eap.c\
	print-eigrp.c\
	print-esp.c\
	print-ether.c\
	print-fddi.c\
	print-fr.c\
	print-frag6.c\
	print-gre.c\
	print-hsrp.c\
	print-icmp.c\
	print-icmp6.c\
	print-igmp.c\
	print-igrp.c\
	print-ip.c\
	print-ip6.c\
	print-ip6opts.c\
	print-ipcomp.c\
	print-ipfc.c\
	print-ipx.c\
	print-isakmp.c\
	print-isoclns.c\
	print-juniper.c\
	print-krb.c\
	print-l2tp.c\
	print-lane.c\
	print-ldp.c\
	print-llc.c\
	print-lmp.c\
	print-lspping.c\
	print-lwres.c\
	print-mobile.c\
	print-mobility.c\
	print-mpls.c\
	print-msdp.c\
	print-nfs.c\
	print-ntp.c\
	print-null.c\
	print-olsr.c\
	print-ospf.c\
	print-ospf6.c\
	print-pgm.c\
	print-pim.c\
	print-ppp.c\
	print-pppoe.c\
	print-pptp.c\
	print-radius.c\
	print-raw.c\
	print-rip.c\
	print-ripng.c\
	print-rsvp.c\
	print-rt6.c\
	print-rx.c\
	print-sctp.c\
	print-sip.c\
	print-sl.c\
	print-sll.c\
	print-slow.c\
	print-snmp.c\
	print-stp.c\
	print-sunatm.c\
	print-sunrpc.c\
	print-symantec.c\
	print-syslog.c\
	print-tcp.c\
	print-telnet.c\
	print-tftp.c\
	print-timed.c\
	print-token.c\
	print-udp.c\
	print-vjc.c\
	print-vrrp.c\
	print-wb.c\
	print-zephyr.c\
	setsignal.c\
	tcpdump.c\
	util.c\
	version.c\
	print-smb.c\
	smbutil.c\
	missing/strlcat.c\
	missing/strlcpy.c

LOCAL_CFLAGS := -O2 -g
LOCAL_CFLAGS += -DHAVE_CONFIG_H -D_U_="__attribute__((unused))"

LOCAL_C_INCLUDES += \
	libpcap libnet/include include openssl openssl/include

LOCAL_STATIC_LIBRARIES := libpcap libssl

LOCAL_MODULE := tcpdump

include $(BUILD_EXECUTABLE)
