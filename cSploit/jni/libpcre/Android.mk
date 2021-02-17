LOCAL_PATH := $(call my-dir)

#original path: libpcre.la
include $(CLEAR_VARS)

LOCAL_CFLAGS:= -DHAVE_CONFIG_H -ffunction-sections -fdata-sections

LOCAL_C_INCLUDES:= \
	$(LOCAL_PATH)
LOCAL_SRC_FILES:= \
	pcre_byte_order.c\
	pcre_compile.c\
	pcre_config.c\
	pcre_dfa_exec.c\
	pcre_exec.c\
	pcre_fullinfo.c\
	pcre_get.c\
	pcre_globals.c\
	pcre_jit_compile.c\
	pcre_maketables.c\
	pcre_newline.c\
	pcre_ord2utf8.c\
	pcre_refcount.c\
	pcre_string_utils.c\
	pcre_study.c\
	pcre_tables.c\
	pcre_ucd.c\
	pcre_valid_utf8.c\
	pcre_version.c\
	pcre_xclass.c\
	pcre_chartables.c
LOCAL_MODULE := libpcre

include $(BUILD_STATIC_LIBRARY)


#original path: libpcreposix.la
include $(CLEAR_VARS)

LOCAL_CFLAGS:= -DHAVE_CONFIG_H -ffunction-sections -fdata-sections

LOCAL_C_INCLUDES:= \
	$(LOCAL_PATH)
LOCAL_SRC_FILES:= \
	pcreposix.c
LOCAL_STATIC_LIBRARIES:= \
	libpcre
LOCAL_MODULE := libpcreposix

include $(BUILD_STATIC_LIBRARY)


#original path: libpcrecpp.la
include $(CLEAR_VARS)

APP_STL := gnustl_static
LOCAL_CFLAGS:= -DHAVE_CONFIG_H -ffunction-sections -fdata-sections

LOCAL_C_INCLUDES:= \
	$(LOCAL_PATH)
LOCAL_SRC_FILES:= \
	pcrecpp.cc\
	pcre_scanner.cc\
	pcre_stringpiece.cc
LOCAL_STATIC_LIBRARIES:= \
	libpcre
LOCAL_MODULE := libpcrecpp

include $(BUILD_STATIC_LIBRARY)


#original path: libpcre16.la
include $(CLEAR_VARS)

LOCAL_CFLAGS:= -DHAVE_CONFIG_H -ffunction-sections -fdata-sections

LOCAL_C_INCLUDES:= \
	$(LOCAL_PATH)
LOCAL_SRC_FILES:= \
	pcre16_byte_order.c\
	pcre16_chartables.c\
	pcre16_compile.c\
	pcre16_config.c\
	pcre16_dfa_exec.c\
	pcre16_exec.c\
	pcre16_fullinfo.c\
	pcre16_get.c\
	pcre16_globals.c\
	pcre16_jit_compile.c\
	pcre16_maketables.c\
	pcre16_newline.c\
	pcre16_ord2utf16.c\
	pcre16_refcount.c\
	pcre16_string_utils.c\
	pcre16_study.c\
	pcre16_tables.c\
	pcre16_ucd.c\
	pcre16_utf16_utils.c\
	pcre16_valid_utf16.c\
	pcre16_version.c\
	pcre16_xclass.c\
	pcre_chartables.c
LOCAL_MODULE := libpcre16

include $(BUILD_STATIC_LIBRARY)


#original path: libpcre32.la
include $(CLEAR_VARS)

LOCAL_CFLAGS:= -DHAVE_CONFIG_H -ffunction-sections -fdata-sections

LOCAL_C_INCLUDES:= \
	$(LOCAL_PATH)
LOCAL_SRC_FILES:= \
	pcre32_byte_order.c\
	pcre32_chartables.c\
	pcre32_compile.c\
	pcre32_config.c\
	pcre32_dfa_exec.c\
	pcre32_exec.c\
	pcre32_fullinfo.c\
	pcre32_get.c\
	pcre32_globals.c\
	pcre32_jit_compile.c\
	pcre32_maketables.c\
	pcre32_newline.c\
	pcre32_ord2utf32.c\
	pcre32_refcount.c\
	pcre32_string_utils.c\
	pcre32_study.c\
	pcre32_tables.c\
	pcre32_ucd.c\
	pcre32_utf32_utils.c\
	pcre32_valid_utf32.c\
	pcre32_version.c\
	pcre32_xclass.c\
	pcre_chartables.c
LOCAL_MODULE := libpcre32

include $(BUILD_STATIC_LIBRARY)

