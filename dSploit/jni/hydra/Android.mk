LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES:= hydra.c hydra-vnc.c hydra-pcnfs.c hydra-rexec.c hydra-nntp.c hydra-socks5.c \
      hydra-telnet.c hydra-cisco.c hydra-http.c hydra-ftp.c hydra-imap.c \
      hydra-pop3.c hydra-smb.c hydra-icq.c hydra-cisco-enable.c hydra-ldap.c \
      hydra-mysql.c hydra-mssql.c hydra-xmpp.c hydra-http-proxy-urlenum.c \
      hydra-snmp.c hydra-cvs.c hydra-smtp.c hydra-smtp-enum.c hydra-sapr3.c hydra-ssh.c \
      hydra-sshkey.c hydra-teamspeak.c hydra-postgres.c hydra-rsh.c hydra-rlogin.c \
      hydra-oracle-listener.c hydra-svn.c hydra-pcanywhere.c hydra-sip.c \
      hydra-oracle.c hydra-vmauthd.c hydra-asterisk.c hydra-firebird.c hydra-afp.c hydra-ncp.c \
      hydra-oracle-sid.c hydra-http-proxy.c hydra-http-form.c hydra-irc.c \
      hydra-rdp.c crc32.c d3des.c bfg.c ntlm.c sasl.c hmacmd5.c hydra-mod.c

LOCAL_CFLAGS:=-O3 -DLIBOPENSSL -DNO_RINDEX -DHAVE_MATH_H -DLIBFIREBIRD -DLIBIDN -DHAVE_PCRE
LOCAL_CFLAGS+=-DHAVE_PR29_H -DLIBMYSQLCLIENT -DLIBNCP -DLIBPOSTGRES -DLIBSVN -DLIBSSH
LOCAL_CFLAGS+=-fdata-sections -ffunction-sections -static
LOCAL_LDFLAGS:=-lm -Wl,--gc-sections

LOCAL_C_INCLUDES:= 	hydra\
					openssl/include\
					libssh/include\
					libidn/lib\
					mysql/include\
					subversion/subversion/include\
					apr/include\
					firebird/include\
					ncpfs/include\
					pcre\
					

LOCAL_STATIC_LIBRARIES:=libssl \
						libcrypto \
						libapr-1 \
						libaprutil-1 \
						libfbclient \
						libidn \
						libmysqlclient \
						libclientlib\
						libdbug\
						libstrings\
						libvio\
						libmysys\
						libmysys_ssl\
						libzlib\
						libyassl\
						libtaocrypt\
						libncp \
						libpcre \
						libpcrecpp \
						libpcreposix \
						libpgcommon \
						libpgport \
						libpq \
						libssh \
						libssh_threads \
						libsvn_auth_gnome_keyring-1 \
						libsvn_auth_kwallet-1 \
						libsvn_client-1 \
						libsvn_delta-1 \
						libsvn_diff-1 \
						libsvn_fs-1 \
						libsvn_fs_base-1 \
						libsvn_fs_fs-1 \
						libsvn_fs_util-1 \
						libsvn_ra-1 \
						libsvn_ra_local-1 \
						libsvn_ra_neon-1 \
						libsvn_ra_serf-1 \
						libsvn_ra_svn-1 \
						libsvn_repos-1 \
						libsvn_subr-1 \
						libsvn_wc-1 \
						libsvnjavahl-1\
						libexpat\
						libiconv
					
LOCAL_MODULE:= hydra

include $(BUILD_EXECUTABLE)
