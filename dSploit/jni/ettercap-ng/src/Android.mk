LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
        ec_send.c \
        ec_inject.c \
        ec_socket.c \
        ec_checksum.c \
        ec_fingerprint.c \
        ec_debug.c \
        ec_session.c \
        ec_ui.c \
        ec_update.c \
        ec_services.c \
        interfaces/daemon/ec_daemon.c \
        interfaces/text/ec_text.c \
        interfaces/text/ec_text_plugin.c \
        interfaces/text/ec_text_conn.c \
        interfaces/text/ec_text_display.c \
        interfaces/text/ec_text_profile.c \
        ec_sniff_bridge.c \
        ec_sslwrap.c \
        dissectors/ec_portmap.c \
        dissectors/ec_rcon.c \
        dissectors/ec_irc.c \
        dissectors/ec_dns.c \
        dissectors/ec_rlogin.c \
        dissectors/ec_pop.c \
        dissectors/ec_ssh.c \
        dissectors/ec_http.c \
        dissectors/ec_telnet.c \
        dissectors/ec_vnc.c \
        dissectors/ec_ldap.c \
        dissectors/ec_napster.c \
        dissectors/ec_socks.c \
        dissectors/ec_ftp.c \
        dissectors/ec_rip.c \
        dissectors/ec_snmp.c \
        dissectors/ec_imap.c \
        dissectors/ec_msn.c \
        dissectors/ec_icq.c \
        dissectors/ec_cvs.c \
        dissectors/ec_mysql.c \
        dissectors/ec_x11.c \
        dissectors/ec_dhcp.c \
        dissectors/ec_ymsg.c \
        dissectors/ec_vrrp.c \
        dissectors/ec_smtp.c \
        dissectors/ec_ospf.c \
        dissectors/ec_mountd.c \
        dissectors/ec_nntp.c \
        dissectors/ec_smb.c \
        dissectors/ec_bgp.c \
        ec_hook.c \
        ec_parser.c \
        ec_passive.c \
        ec_mitm.c \
        ec_log.c \
        ec_dispatcher.c \
        ec_sniff.c \
        ec_conntrack.c \
        ec_sniff_unified.c \
        ec_decode.c \
        ec_stats.c \
        ec_inet.c \
        ec_manuf.c \
        ec_interfaces.c \
        ec_profiles.c \
        mitm/ec_port_stealing.c \
        mitm/ec_icmp_redirect.c \
        mitm/ec_dhcp_spoofing.c \
        mitm/ec_arp_poisoning.c \
        ec_connbuf.c \
        ec_poll.c \
        ec_resolv.c \
        ec_threads.c \
        missing/strlcat.c \
        missing/strlcpy.c \
        ec_strings.c \
        ec_filter.c \
        ec_globals.c \
        ec_hash.c \
        os/ec_linux.c \
        ec_file.c \
        ec_main.c \
        ec_format.c \
        ec_conf.c \
        ec_streambuf.c \
        ec_scan.c \
        ec_error.c \
        ec_packet.c \
        ec_signals.c \
        protocols/ec_wifi.c \
        protocols/ec_vlan.c \
        protocols/ec_prism.c \
        protocols/ec_cooked.c \
        protocols/ec_icmp.c \
        protocols/ec_ppp.c \
        protocols/ec_gre.c \
        protocols/ec_ip.c \
        protocols/ec_eth.c \
        protocols/ec_tcp.c \
        protocols/ec_fddi.c \
        protocols/ec_ip6.c \
        protocols/ec_arp.c \
        protocols/ec_udp.c \
        protocols/ec_rawip.c \
        protocols/ec_tr.c \
        ec_dissect.c \
        ec_capture.c \
        ec_plugins.c

include $(LOCAL_PATH)/../android-config.mk
        
LOCAL_C_INCLUDES += \
	ettercap-ng/include \
	ettercap-ng/src/interfaces/text \
	libpcap \
	libnet/include \
	include \
	openssl \
	openssl/include

LOCAL_STATIC_LIBRARIES+= \
	libpcap \
	libnet \
	libssl \
	libcrypto

LOCAL_CFLAGS += -O2 -g -DHAVE_CONFIG_H

LOCAL_MODULE:= ettercap

include $(BUILD_EXECUTABLE)

#removed stuff
#        interfaces/curses/ec_curses.c \
#        interfaces/curses/ec_curses_view.c \
#        interfaces/curses/ec_curses_start.c \
#        interfaces/curses/ec_curses_plugins.c \
#        interfaces/curses/ec_curses_mitm.c \
#        interfaces/curses/ec_curses_targets.c \
#        interfaces/curses/widgets/wdg.c \
#        interfaces/curses/widgets/wdg_dialog.c \
#        interfaces/curses/widgets/wdg_menu.c \
#        interfaces/curses/widgets/wdg_window.c \
#        interfaces/curses/widgets/wdg_dynlist.c \
#        interfaces/curses/widgets/wdg_file.c \
#        interfaces/curses/widgets/wdg_input.c \
#        interfaces/curses/widgets/wdg_percentage.c \
#        interfaces/curses/widgets/wdg_debug.c \
#        interfaces/curses/widgets/wdg_compound.c \
#        interfaces/curses/widgets/wdg_error.c \
#        interfaces/curses/widgets/wdg_panel.c \
#        interfaces/curses/widgets/wdg_list.c \
#        interfaces/curses/widgets/wdg_scroll.c \
#        interfaces/curses/ec_curses_view_profiles.c \
#        interfaces/curses/ec_curses_hosts.c \
#        interfaces/curses/ec_curses_help.c \
#        interfaces/curses/ec_curses_filters.c \
#        interfaces/curses/ec_curses_offline.c \
#        interfaces/curses/ec_curses_view_connections.c \
#        interfaces/curses/ec_curses_live.c \
#        interfaces/curses/ec_curses_logging.c \
#        interfaces/gtk/ec_gtk_conf.c \
#        interfaces/gtk/ec_gtk_start.c \
#        interfaces/gtk/ec_gtk_view.c \
#        interfaces/gtk/ec_gtk_offline.c \
#        interfaces/gtk/ec_gtk_view_connections.c \
#        interfaces/gtk/ec_gtk_help.c \
#        interfaces/gtk/ec_gtk_filters.c \
#        interfaces/gtk/ec_gtk_mitm.c \
#        interfaces/gtk/ec_gtk_plugins.c \
#        interfaces/gtk/ec_gtk_menus.c \
#        interfaces/gtk/ec_gtk_targets.c \
#        interfaces/gtk/ec_gtk_view_profiles.c \
#        interfaces/gtk/ec_gtk.c \
#        interfaces/gtk/ec_gtk_hosts.c \
#        interfaces/gtk/ec_gtk_logging.c \
#        interfaces/gtk/ec_gtk_live.c \
			