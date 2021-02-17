
# $Id: Makefile.am,v 1.84 2004/11/05 10:01:14 alor Exp $

include $(top_srcdir)/Makefile.am.common

SUBDIRS = os interfaces

bin_PROGRAMS = ettercap

EXTRA_DIST = ettercap.rc

ettercap_SOURCES = ec_capture.c \
                   ec_checksum.c \
                   ec_conf.c \
                   ec_connbuf.c \
                   ec_conntrack.c \
                   ec_debug.c \
                   ec_decode.c \
                   ec_dispatcher.c \
                   ec_dissect.c \
                   ec_error.c \
                   ec_file.c \
                   ec_filter.c \
                   ec_fingerprint.c \
                   ec_format.c \
                   ec_globals.c \
                   ec_hash.c \
                   ec_hook.c \
                   ec_inet.c \
                   ec_inject.c \
                   ec_interfaces.c \
                   ec_log.c \
                   ec_main.c \
                   ec_manuf.c \
                   ec_mitm.c \
                   ec_packet.c \
                   ec_parser.c \
                   ec_passive.c \
                   ec_plugins.c \
                   ec_poll.c \
                   ec_profiles.c \
                   ec_resolv.c \
                   ec_scan.c \
                   ec_send.c \
                   ec_services.c \
                   ec_session.c \
                   ec_signals.c \
                   ec_socket.c \
                   ec_sniff.c \
                   ec_sniff_bridge.c \
                   ec_sniff_unified.c \
                   ec_sslwrap.c \
                   ec_stats.c \
                   ec_streambuf.c \
                   ec_strings.c \
                   ec_threads.c \
                   ec_ui.c \
                   ec_update.c 

#
# these cannot be compiled as a lib because
# they use __init functions
#

ettercap_SOURCES += mitm/ec_arp_poisoning.c \
                    mitm/ec_dhcp_spoofing.c \
                    mitm/ec_icmp_redirect.c \
                    mitm/ec_port_stealing.c

ettercap_SOURCES += protocols/ec_arp.c \
                    protocols/ec_eth.c \
                    protocols/ec_fddi.c \
				        protocols/ec_cooked.c \
				        protocols/ec_gre.c \
				        protocols/ec_icmp.c \
				        protocols/ec_ip.c \
				        protocols/ec_ip6.c \
				        protocols/ec_ppp.c \
				        protocols/ec_prism.c \
				        protocols/ec_rawip.c \
				        protocols/ec_tcp.c \
				        protocols/ec_tr.c \
				        protocols/ec_vlan.c \
				        protocols/ec_udp.c \
				        protocols/ec_wifi.c
               

ettercap_SOURCES += dissectors/ec_bgp.c \
                    dissectors/ec_cvs.c \
                    dissectors/ec_dhcp.c 

if HAVE_DN_EXPAND
ettercap_SOURCES += dissectors/ec_dns.c
endif

ettercap_SOURCES += dissectors/ec_ftp.c \
				        dissectors/ec_http.c \
				        dissectors/ec_imap.c \
				        dissectors/ec_icq.c \
				        dissectors/ec_irc.c \
				        dissectors/ec_ldap.c \
				        dissectors/ec_mountd.c \
				        dissectors/ec_msn.c \
				        dissectors/ec_mysql.c \
				        dissectors/ec_napster.c \
				        dissectors/ec_nntp.c \
				        dissectors/ec_ospf.c \
				        dissectors/ec_pop.c \
				        dissectors/ec_portmap.c \
				        dissectors/ec_rcon.c \
				        dissectors/ec_rip.c \
				        dissectors/ec_rlogin.c \
				        dissectors/ec_smb.c \
				        dissectors/ec_smtp.c \
				        dissectors/ec_snmp.c 
if OPENSSL
ettercap_SOURCES += dissectors/ec_ssh.c
endif

ettercap_SOURCES += dissectors/ec_socks.c \
				        dissectors/ec_telnet.c \
				        dissectors/ec_vnc.c \
				        dissectors/ec_vrrp.c \
				        dissectors/ec_x11.c \
				        dissectors/ec_ymsg.c 


ettercap_CFLAGS = @EC_CFLAGS@

ettercap_LDADD = @LIBOBJS@ \
                 os/libec_os.a \
                 interfaces/daemon/libec_daemon.a \
                 interfaces/text/libec_text.a 

if NCURSES
ettercap_LDADD += interfaces/curses/libec_curses.a 
endif

if GTK
ettercap_LDADD += interfaces/gtk/libec_gtk.a
endif

ettercap_LDFLAGS = -export-dynamic @EC_LIBS@ 

CLEANFILES = *.log *.out core* missing/*.o *~


## MISSING FUNCTIONS

missing/strlcat.o:
	$(COMPILE) @EC_LIBS@ -c missing/strlcat.c -o missing/strlcat.o

missing/strlcpy.o:
	$(COMPILE) @EC_LIBS@ -c missing/strlcpy.c -o missing/strlcpy.o
         
missing/strsep.o:
	$(COMPILE) @EC_LIBS@ -c missing/strsep.c -o missing/strsep.o
               
missing/strcasestr.o:
	$(COMPILE) @EC_LIBS@ -c missing/strcasestr.c -o missing/strcasestr.o
                     
missing/scandir.o:
	$(COMPILE) @EC_LIBS@ -c missing/scandir.c -o missing/scandir.o
                           
missing/memmem.o:
	$(COMPILE) @EC_LIBS@ -c missing/memmem.c -o missing/memmem.o
                                 
missing/memcmp.o:
	$(COMPILE) @EC_LIBS@ -c missing/memcmp.c -o missing/memcmp.o

missing/basename.o:
	$(COMPILE) @EC_LIBS@ -c missing/basename.c -o missing/basename.o
                                       
missing/getopt.o:
	$(COMPILE) @EC_LIBS@ -c missing/getopt.c -o missing/getopt.o
                                             
missing/getopt_long.o:
	$(COMPILE) @EC_LIBS@ -c missing/getopt_long.c -o missing/getopt_long.o

# vim:ts=4:noexpandtab
