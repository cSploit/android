LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

subdirs := $(addprefix $(LOCAL_PATH)/,$(addsuffix /Android.mk, src))

LOCAL_CFLAGS += -O2 -g -DHAVE_CONFIG_H

include $(subdirs)
