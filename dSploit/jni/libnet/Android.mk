LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES:=\
	src/libnet_advanced.c \
    src/libnet_asn1.c \
    src/libnet_build_802.1q.c \
    src/libnet_build_802.1x.c \
    src/libnet_build_802.2.c \
    src/libnet_build_802.3.c \
    src/libnet_build_arp.c \
    src/libnet_build_bgp.c \
    src/libnet_build_cdp.c \
    src/libnet_build_data.c \
    src/libnet_build_dhcp.c \
    src/libnet_build_dns.c \
    src/libnet_build_ethernet.c \
    src/libnet_build_fddi.c \
    src/libnet_build_gre.c \
    src/libnet_build_hsrp.c \
    src/libnet_build_icmp.c \
    src/libnet_build_igmp.c \
    src/libnet_build_ip.c \
    src/libnet_build_ipsec.c \
    src/libnet_build_isl.c \
    src/libnet_build_link.c \
    src/libnet_build_mpls.c \
    src/libnet_build_ntp.c \
    src/libnet_build_ospf.c \
    src/libnet_build_rip.c \
    src/libnet_build_rpc.c \
    src/libnet_build_sebek.c \
    src/libnet_build_snmp.c \
    src/libnet_build_stp.c \
    src/libnet_build_tcp.c \
    src/libnet_build_token_ring.c \
    src/libnet_build_udp.c \
    src/libnet_build_vrrp.c \
    src/libnet_checksum.c \
    src/libnet_cq.c \
    src/libnet_crc.c \
    src/libnet_error.c \
    src/libnet_if_addr.c \
    src/libnet_init.c \
    src/libnet_internal.c \
    src/libnet_link_linux.c \
    src/libnet_pblock.c \
    src/libnet_port_list.c \
    src/libnet_prand.c \
    src/libnet_raw.c \
    src/libnet_resolve.c \
    src/libnet_version.c \
    src/libnet_write.c

LOCAL_CFLAGS:=-O2 -g
LOCAL_CFLAGS+=-DHAVE_CONFIG_H -D_U_="__attribute__((unused))" -Dlinux -D__GLIBC__ -D_GNU_SOURCE

LOCAL_MODULE:= libnet

include $(BUILD_STATIC_LIBRARY)
