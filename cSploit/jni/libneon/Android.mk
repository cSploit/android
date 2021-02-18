LOCAL_PATH := $(call my-dir)

#original path: src/libneon.la
include $(CLEAR_VARS)

LOCAL_CFLAGS:= -DHAVE_CONFIG_H -DNE_PRIVATE=extern -ffunction-sections \
	-fdata-sections -DOPENSSL_NO_DEPRECATED

LOCAL_C_INCLUDES:= \
	external/openssl/include\
	external/libexpat/lib\
	$(LOCAL_PATH)
LOCAL_SRC_FILES:= \
	src/ne_request.c\
	src/ne_session.c\
	src/ne_basic.c\
	src/ne_string.c\
	src/ne_uri.c\
	src/ne_dates.c\
	src/ne_alloc.c\
	src/ne_md5.c\
	src/ne_utils.c\
	src/ne_socket.c\
	src/ne_auth.c\
	src/ne_redirect.c\
	src/ne_compress.c\
	src/ne_i18n.c\
	src/ne_pkcs11.c\
	src/ne_socks.c\
	src/ne_ntlm.c\
	src/ne_207.c\
	src/ne_xml.c\
	src/ne_props.c\
	src/ne_locks.c\
	src/ne_xmlreq.c\
	src/ne_oldacl.c\
	src/ne_acl3744.c\
	src/ne_openssl.c
LOCAL_SHARED_LIBRARIES:= \
	libcrypto\
	libssl\
	libexpat\
	z
	
LOCAL_MODULE := libneon

include $(BUILD_STATIC_LIBRARY)

