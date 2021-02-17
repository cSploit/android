LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_ALLOW_UNDEFINED_SYMBOLS := true

LOCAL_CFLAGS := -DHAVE_CONFIG_H

LOCAL_C_INCLUDES := $(ettercap_pl_includes)

LOCAL_SRC_FILES := pptp_chapms1.c

LOCAL_MODULE := ec_pptp_chapms1

include $(BUILD_SHARED_LIBRARY)