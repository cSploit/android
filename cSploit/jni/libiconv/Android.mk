LOCAL_PATH := $(call my-dir)

#original path: libcharset/lib/libcharset.la
include $(CLEAR_VARS)

LOCAL_CFLAGS:= -DLIBDIR=\"/system/lib\" -DBUILDING_LIBCHARSET -DBUILDING_DLL \
-DENABLE_RELOCATABLE=1 -DIN_LIBRARY -DNO_XMALLOC -DINSTALLDIR=\"/system/lib\"\
-Dset_relocation_prefix=libcharset_set_relocation_prefix -Drelocate=libcharset_relocate\
-DHAVE_CONFIG_H -DPIC -ffunction-sections -fdata-sections

LOCAL_C_INCLUDES:= \
	$(LOCAL_PATH)/libcharset/lib\
	$(LOCAL_PATH)/libcharset/include\
	$(LOCAL_PATH)
LOCAL_SRC_FILES:= \
	libcharset/lib/localcharset.c\
	libcharset/lib/relocatable.c
LOCAL_MODULE := libcharset

include $(BUILD_STATIC_LIBRARY)


#original path: lib/libiconv.la
include $(CLEAR_VARS)

LOCAL_CFLAGS:= -DLIBDIR=\"/system/lib\" -DBUILDING_LIBICONV -DBUILDING_DLL \
-DENABLE_RELOCATABLE=1 -DIN_LIBRARY -DINSTALLDIR=\"/system/lib\" \
-DNO_XMALLOC -Dset_relocation_prefix=libiconv_set_relocation_prefix \
-Drelocate=libiconv_relocate -DHAVE_CONFIG_H -DPIC -ffunction-sections -fdata-sections

LOCAL_C_INCLUDES:= \
	$(LOCAL_PATH)/lib\
	$(LOCAL_PATH)/include
LOCAL_SRC_FILES:= \
	lib/iconv.c\
	libcharset/lib/localcharset.c\
	lib/relocatable.c
LOCAL_MODULE := libiconv

include $(BUILD_STATIC_LIBRARY)

