LOCAL_PATH := $(call my-dir)

#original path: src/dblib/.libs/libsybdb.a
include $(CLEAR_VARS)

LOCAL_CFLAGS:= -DHAVE_CONFIG_H -DUNIXODBC -D_REENTRANT -D_THREAD_SAFE \
-DNDEBUG=1 -DOPENSSL_NO_DEPRECATED

# fixed flags
LOCAL_CFLAGS+= -ffunction-sections -fdata-sections

LOCAL_C_INCLUDES:= \
	$(LOCAL_PATH)/src/dblib\
	$(LOCAL_PATH)/include\
	$(LOCAL_PATH)/generated/android/include\
	$(LOCAL_PATH)/generated/android/src/tds\
	external/libiconv/include\
	external/openssl/include
LOCAL_SRC_FILES:= \
	src/replacements/iconv.c\
	src/replacements/gettimeofday.c\
	src/replacements/fakepoll.c\
	src/replacements/getpassarg.c\
	src/replacements/socketpair.c\
	src/replacements/win_mutex.c\
	src/replacements/win_cond.c\
	src/replacements/getaddrinfo.c\
	src/replacements/readpassphrase.c\
	src/tds/mem.c\
	src/tds/token.c\
	src/tds/util.c\
	src/tds/login.c\
	src/tds/read.c\
	src/tds/write.c\
	src/tds/convert.c\
	src/tds/numeric.c\
	src/tds/config.c\
	src/tds/query.c\
	src/tds/iconv.c\
	src/tds/locale.c\
	src/tds/threadsafe.c\
	src/tds/vstrbuild.c\
	src/tds/tdsstring.c\
	src/tds/getmac.c\
	src/tds/data.c\
	src/tds/net.c\
	src/tds/tds_checks.c\
	src/tds/log.c\
	src/tds/bulk.c\
	src/tds/packet.c\
	src/tds/stream.c\
	src/tds/challenge.c\
	src/tds/md4.c\
	src/tds/md5.c\
	src/tds/des.c\
	src/tds/gssapi.c\
	src/tds/hmac_md5.c\
	src/dblib/dblib.c\
	src/dblib/dbutil.c\
	src/dblib/rpc.c\
	src/dblib/bcp.c\
	src/dblib/xact.c\
	src/dblib/dbpivot.c
LOCAL_STATIC_LIBRARIES:= \
	libiconv
LOCAL_SHARED_LIBRARIES:= \
	libssl\
	libcrypto
LOCAL_MODULE := libsybdb

include $(BUILD_STATIC_LIBRARY)

