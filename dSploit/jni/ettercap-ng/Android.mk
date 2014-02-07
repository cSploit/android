LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

subdirs := $(addprefix $(LOCAL_PATH)/,$(addsuffix /Android.mk, src))

LOCAL_CFLAGS += -O2 -g -DHAVE_CONFIG_H -static -ffunction-sections -fdata-sections

LOCAL_LDFLAGS += -lz -Wl,--gc-sections

LOCAL_C_INCLUDES := \
	ettercap-ng/include \
	ettercap-ng/src/interfaces/text \
	libpcap \
	libnet/include \
	include \
	openssl \
	openssl/include\
	libbthread

LOCAL_SRC_FILES:= \
        src/ec_send.c \
        src/ec_inject.c \
        src/ec_socket.c \
        src/ec_checksum.c \
        src/ec_fingerprint.c \
        src/ec_debug.c \
        src/ec_session.c \
        src/ec_ui.c \
        src/ec_update.c \
        src/ec_services.c \
        src/interfaces/daemon/ec_daemon.c \
        src/interfaces/text/ec_text.c \
        src/interfaces/text/ec_text_plugin.c \
        src/interfaces/text/ec_text_conn.c \
        src/interfaces/text/ec_text_display.c \
        src/interfaces/text/ec_text_profile.c \
        src/ec_sniff_bridge.c \
        src/ec_sslwrap.c \
        src/dissectors/ec_portmap.c \
        src/dissectors/ec_rcon.c \
        src/dissectors/ec_irc.c \
        src/dissectors/ec_dns.c \
        src/dissectors/ec_rlogin.c \
        src/dissectors/ec_pop.c \
        src/dissectors/ec_ssh.c \
        src/dissectors/ec_http.c \
        src/dissectors/ec_telnet.c \
        src/dissectors/ec_vnc.c \
        src/dissectors/ec_ldap.c \
        src/dissectors/ec_napster.c \
        src/dissectors/ec_socks.c \
        src/dissectors/ec_ftp.c \
        src/dissectors/ec_rip.c \
        src/dissectors/ec_snmp.c \
        src/dissectors/ec_imap.c \
        src/dissectors/ec_msn.c \
        src/dissectors/ec_icq.c \
        src/dissectors/ec_cvs.c \
        src/dissectors/ec_mysql.c \
        src/dissectors/ec_x11.c \
        src/dissectors/ec_dhcp.c \
        src/dissectors/ec_ymsg.c \
        src/dissectors/ec_vrrp.c \
        src/dissectors/ec_smtp.c \
        src/dissectors/ec_ospf.c \
        src/dissectors/ec_mountd.c \
        src/dissectors/ec_nntp.c \
        src/dissectors/ec_smb.c \
        src/dissectors/ec_bgp.c \
        src/dissectors/ec_TN3270.c\
				src/dissectors/ec_gg.c\
				src/dissectors/ec_iscsi.c\
				src/dissectors/ec_mongodb.c\
				src/dissectors/ec_o5logon.c\
				src/dissectors/ec_postgresql.c\
				src/dissectors/ec_radius.c\
        src/ec_hook.c \
        src/ec_parser.c \
        src/ec_passive.c \
        src/ec_mitm.c \
        src/ec_log.c \
        src/ec_dispatcher.c \
        src/ec_sniff.c \
        src/ec_conntrack.c \
        src/ec_sniff_unified.c \
        src/ec_decode.c \
        src/ec_stats.c \
        src/ec_inet.c \
        src/ec_manuf.c \
        src/ec_interfaces.c \
        src/ec_profiles.c \
        src/mitm/ec_port_stealing.c \
        src/mitm/ec_icmp_redirect.c \
        src/mitm/ec_dhcp_spoofing.c \
        src/mitm/ec_arp_poisoning.c \
        src/ec_connbuf.c \
        src/ec_poll.c \
        src/ec_resolv.c \
        src/ec_threads.c \
        src/missing/strlcat.c \
        src/missing/strlcpy.c \
        src/ec_strings.c \
        src/ec_filter.c \
        src/ec_globals.c \
        src/ec_hash.c \
        src/os/ec_linux.c \
        src/ec_file.c \
        src/ec_main.c \
        src/ec_format.c \
        src/ec_conf.c \
        src/ec_streambuf.c \
        src/ec_scan.c \
        src/ec_error.c \
        src/ec_packet.c \
        src/ec_signals.c \
        src/protocols/ec_wifi.c \
        src/protocols/ec_vlan.c \
        src/protocols/ec_prism.c \
        src/protocols/ec_cooked.c \
        src/protocols/ec_icmp.c \
        src/protocols/ec_ppp.c \
        src/protocols/ec_gre.c \
        src/protocols/ec_ip.c \
        src/protocols/ec_eth.c \
        src/protocols/ec_tcp.c \
        src/protocols/ec_fddi.c \
        src/protocols/ec_ip6.c \
        src/protocols/ec_arp.c \
        src/protocols/ec_udp.c \
        src/protocols/ec_rawip.c \
        src/protocols/ec_tr.c \
        src/ec_dissect.c \
        src/ec_capture.c \
        src/ec_plugins.c

LOCAL_STATIC_LIBRARIES:= \
	libpcap \
	libnet \
	libssl \
	libcrypto\
	libexpat\
	libbthread

LOCAL_MODULE:= ettercap

include $(BUILD_EXECUTABLE)