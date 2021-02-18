top_dir := $(call my-dir)

ettercap_pl_includes := \
	$(top_dir)/include\
	external/libpcre\
	external/libpcap\
	external/libnet/libnet/include\
	external/libbthread\
	external/libcurl/include
	

include $(call all-subdir-makefiles)

LOCAL_PATH := $(top_dir)

include $(CLEAR_VARS)

LOCAL_CFLAGS += -O2 -g -DHAVE_CONFIG_H -DOPENSSL_NO_DEPRECATED -ffunction-sections\
	-fdata-sections

LOCAL_LDFLAGS += -rdynamic  -Wl,-export-dynamic -Wl,--no-gc-sections

LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/include \
	$(LOCAL_PATH)/src/interfaces/text \
	external/libpcap \
	external/libnet/libnet/include \
	external/openssl/include\
	external/libpcre\
	external/libbthread\
	external/libifaddrs

LOCAL_SRC_FILES:= \
        src/ec_send.c \
        src/ec_inject.c \
        src/ec_socket.c \
        src/ec_checksum.c \
        src/ec_fingerprint.c \
        src/ec_debug.c \
        src/ec_session.c \
        src/ec_ui.c \
        src/ec_interfaces.c\
        src/protocols/ec_icmp6.c\
        src/mitm/ec_ip6nd_poison.c\
        src/dissectors/ec_dns.c\
        src/dissectors/ec_ssh.c\
        src/dissectors/ec_gg.c\
        src/ec_main.c\
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
        src/dissectors/ec_rlogin.c \
        src/dissectors/ec_pop.c \
        src/dissectors/ec_http.c \
        src/dissectors/ec_telnet.c \
        src/dissectors/ec_vnc.c \
        src/dissectors/ec_ldap.c \
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
        src/dissectors/ec_mdns.c\
        src/dissectors/ec_nbns.c\
				src/dissectors/ec_iscsi.c\
				src/dissectors/ec_mongodb.c\
				src/dissectors/ec_o5logon.c\
				src/dissectors/ec_postgresql.c\
				src/dissectors/ec_radius.c\
				src/ec_encryption.c\
				src/ec_encryption_ccmp.c\
				src/ec_encryption_tkip.c\
				src/ec_exit.c\
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
        src/ec_network.c\
        src/ec_format.c \
        src/ec_conf.c \
        src/ec_streambuf.c \
        src/ec_scan.c \
        src/ec_error.c \
        src/ec_packet.c \
        src/ec_signals.c \
        src/protocols/ec_wifi.c \
        src/protocols/ec_vlan.c \
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
        src/protocols/ec_erf.c \
        src/protocols/ec_mpls.c \
        src/protocols/ec_pap.c \
        src/protocols/ec_ppi.c \
        src/protocols/ec_pppoe.c \
        src/protocols/ec_wifi_eapol.c \
        src/protocols/ec_wifi_prism.c \
        src/protocols/ec_wifi_radiotap.c \
        src/ec_dissect.c \
        src/ec_capture.c \
        src/ec_plugins.c

LOCAL_STATIC_LIBRARIES:= \
	libpcap \
	libnet \
	libpcre\
	libexpat\
	libbthread\
	libifaddrs
	
LOCAL_SHARED_LIBRARIES:= \
	libssl\
	libcrypto\
	libz

LOCAL_MODULE:= ettercap

include $(BUILD_EXECUTABLE)