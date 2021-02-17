LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES:=\
    libnet/src/libnet_advanced.c \
    libnet/src/libnet_asn1.c \
    libnet/src/libnet_build_802.1q.c \
    libnet/src/libnet_build_802.1x.c \
    libnet/src/libnet_build_802.2.c \
    libnet/src/libnet_build_802.3.c \
    libnet/src/libnet_build_arp.c \
    libnet/src/libnet_build_bgp.c \
    libnet/src/libnet_build_cdp.c \
    libnet/src/libnet_build_data.c \
    libnet/src/libnet_build_dhcp.c \
    libnet/src/libnet_build_dns.c \
    libnet/src/libnet_build_ethernet.c \
    libnet/src/libnet_build_fddi.c \
    libnet/src/libnet_build_gre.c \
    libnet/src/libnet_build_hsrp.c \
    libnet/src/libnet_build_icmp.c \
    libnet/src/libnet_build_igmp.c \
    libnet/src/libnet_build_ip.c \
    libnet/src/libnet_build_ipsec.c \
    libnet/src/libnet_build_isl.c \
    libnet/src/libnet_build_link.c \
    libnet/src/libnet_build_mpls.c \
    libnet/src/libnet_build_ntp.c \
    libnet/src/libnet_build_ospf.c \
    libnet/src/libnet_build_rip.c \
    libnet/src/libnet_build_rpc.c \
    libnet/src/libnet_build_sebek.c \
    libnet/src/libnet_build_snmp.c \
    libnet/src/libnet_build_stp.c \
    libnet/src/libnet_build_tcp.c \
    libnet/src/libnet_build_token_ring.c \
    libnet/src/libnet_build_udp.c \
    libnet/src/libnet_build_vrrp.c \
    libnet/src/libnet_checksum.c \
    libnet/src/libnet_cq.c \
    libnet/src/libnet_crc.c \
    libnet/src/libnet_error.c \
    libnet/src/libnet_if_addr.c \
    libnet/src/libnet_init.c \
    libnet/src/libnet_internal.c \
    libnet/src/libnet_link_linux.c \
    libnet/src/libnet_pblock.c \
    libnet/src/libnet_port_list.c \
    libnet/src/libnet_prand.c \
    libnet/src/libnet_raw.c \
    libnet/src/libnet_resolve.c \
    libnet/src/libnet_version.c \
    libnet/src/libnet_write.c

LOCAL_CFLAGS:=-O2 -g
LOCAL_CFLAGS+=-DHAVE_CONFIG_H -D_U_="__attribute__((unused))" -Dlinux -D__GLIBC__ -D_GNU_SOURCE\
-ffunction-sections -fdata-sections

LOCAL_MODULE:= libnet

include $(BUILD_STATIC_LIBRARY)
