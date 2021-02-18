LOCAL_PATH := $(call my-dir)

#original path: libs/armeabi-v7a/libssh.so.4.2.3
include $(CLEAR_VARS)

LOCAL_CFLAGS:= -DANDROID -DLIBSSH_EXPORTS -D_FORTIFY_SOURCE=2 \
-ffunction-sections -fdata-sections -DOPENSSL_NO_DEPRECATED

LOCAL_C_INCLUDES:= \
	$(LOCAL_PATH)/src\
	$(LOCAL_PATH)/include\
	$(LOCAL_PATH)/androidbuild\
	external/openssl/include
LOCAL_SRC_FILES:= \
	src/agent.c\
	src/auth.c\
	src/base64.c\
	src/bind.c\
	src/buffer.c\
	src/callbacks.c\
	src/channels.c\
	src/client.c\
	src/config.c\
	src/connect.c\
	src/crc32.c\
	src/curve25519.c\
	src/curve25519_ref.c\
	src/dh.c\
	src/ecdh.c\
	src/error.c\
	src/gcrypt_missing.c\
	src/getpass.c\
	src/gzip.c\
	src/init.c\
	src/kex1.c\
	src/kex.c\
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
	src/packet_cb.c\
	src/packet_crypt.c\
	src/pcap.c\
	src/pki.c\
	src/pki_crypto.c\
	src/pki_gcrypt.c\
	src/poll.c\
	src/scp.c\
	src/server.c\
	src/session.c\
	src/sftp.c\
	src/sftpserver.c\
	src/socket.c\
	src/string.c\
	src/threads.c\
	src/wrapper.c
LOCAL_SHARED_LIBRARIES:= \
	libssl\
	libcrypto\
	z
LOCAL_MODULE := libssh

include $(BUILD_STATIC_LIBRARY)
