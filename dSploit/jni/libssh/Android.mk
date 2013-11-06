LOCAL_PATH := $(call my-dir)

#original path: libs/armeabi-v7a/libssh.so.4.2.3
include $(CLEAR_VARS)

LOCAL_CFLAGS:= -DANDROID -DLIBSSH_EXPORTS -D_FORTIFY_SOURCE=2 \
-ffunction-sections -fdata-sections

LOCAL_C_INCLUDES:= \
	libssh/androidbuild/src\
	libssh/src\
	libssh/include\
	libssh/androidbuild\
	openssl/include
LOCAL_SRC_FILES:= \
	src/agent.c\
	src/auth.c\
	src/base64.c\
	src/buffer.c\
	src/callbacks.c\
	src/channels.c\
	src/client.c\
	src/config.c\
	src/connect.c\
	src/crc32.c\
	src/crypt.c\
	src/dh.c\
	src/error.c\
	src/getpass.c\
	src/gcrypt_missing.c\
	src/gzip.c\
	src/init.c\
	src/kex.c\
	src/keyfiles.c\
	src/keys.c\
	src/known_hosts.c\
	src/legacy.c\
	src/libcrypto.c\
	src/libgcrypt.c\
	src/log.c\
	src/match.c\
	src/messages.c\
	src/misc.c\
	src/options.c\
	src/packet.c\
	src/pcap.c\
	src/pki.c\
	src/poll.c\
	src/session.c\
	src/scp.c\
	src/socket.c\
	src/string.c\
	src/threads.c\
	src/wrapper.c\
	src/sftp.c\
	src/sftpserver.c\
	src/server.c\
	src/bind.c
LOCAL_STATIC_LIBRARIES:= \
	libssl\
	libcrypto
LOCAL_MODULE := libssh

include $(BUILD_STATIC_LIBRARY)

