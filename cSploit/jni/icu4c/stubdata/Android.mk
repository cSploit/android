LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_CFLAGS:= -D_REENTRANT -DU_COMMON_IMPLEMENTATION -O3 \
'-DICU_DATA_DIR="/usr/icu"' '-DICU_DATA_DIR_PREFIX_ENV_VAR="ANDROID_ROOT"' \
-ffunction-sections -fdata-sections

LOCAL_C_INCLUDES:= \
        icu/source/common
LOCAL_SRC_FILES:= \
        stubdata.c
LOCAL_MODULE := libicudata

include $(BUILD_STATIC_LIBRARY)