LOCAL_PATH := $(call my-dir)

#original path: lib/libidn.la
include $(CLEAR_VARS)

LOCAL_CFLAGS:= -DHAVE_CONFIG_H -DLIBIDN_BUILDING \
-DLOCALEDIR=\"/opt/android-ndk/platforms/android-18/arch-arm/usr/share/locale\" \
-fvisibility=hidden -ffunction-sections -fdata-sections

LOCAL_C_INCLUDES:= \
	$(LOCAL_PATH)/lib\
	$(LOCAL_PATH)\
	$(LOCAL_PATH)/lib/gl\
	external/libiconv/include
LOCAL_SRC_FILES:= \
	lib/gl/c-ctype.c\
	lib/gl/c-strcasecmp.c\
	lib/gl/c-strncasecmp.c\
	lib/gl/striconv.c\
	lib/gl/unistr/u8-mbtoucr.c\
	lib/gl/unistr/u8-uctomb.c\
	lib/gl/unistr/u8-uctomb-aux.c\
	lib/gl/strverscmp.c\
	lib/nfkc.c\
	lib/toutf8.c\
	lib/version.c\
	lib/stringprep.c\
	lib/rfc3454.c\
	lib/profiles.c\
	lib/punycode.c\
	lib/idna.c\
	lib/pr29.c\
	lib/idn-free.c\
	lib/strerror-idna.c\
	lib/strerror-pr29.c\
	lib/strerror-punycode.c\
	lib/strerror-stringprep.c\
	lib/strerror-tld.c\
	lib/tld.c\
	lib/tlds.c
LOCAL_STATIC_LIBRARIES:= \
	libiconv
LOCAL_MODULE := libidn

include $(BUILD_STATIC_LIBRARY)

