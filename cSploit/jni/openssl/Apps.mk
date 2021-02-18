# Copyright 2006 The Android Open Source Project

LOCAL_PATH:= $(call my-dir)

local_c_includes :=
local_c_flags :=

local_additional_dependencies := $(LOCAL_PATH)/android-config.mk $(LOCAL_PATH)/Apps.mk

include $(LOCAL_PATH)/Apps-config.mk

include $(CLEAR_VARS)
LOCAL_MODULE:= openssl
LOCAL_CLANG := true
LOCAL_MODULE_TAGS := optional
LOCAL_SRC_FILES := $(target_src_files)
LOCAL_SHARED_LIBRARIES := libssl libcrypto
LOCAL_C_INCLUDES := $(target_c_includes)
LOCAL_CFLAGS := $(target_c_flags)
LOCAL_ADDITIONAL_DEPENDENCIES := $(local_additional_dependencies)
include $(LOCAL_PATH)/android-config.mk
include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)
LOCAL_MODULE:= openssl
LOCAL_MODULE_TAGS := optional
LOCAL_SRC_FILES := $(host_src_files)
LOCAL_SHARED_LIBRARIES := libssl-host libcrypto-host
LOCAL_C_INCLUDES := $(host_c_includes)
LOCAL_CFLAGS := $(host_c_flags)
LOCAL_ADDITIONAL_DEPENDENCIES := $(local_additional_dependencies)
include $(LOCAL_PATH)/android-config.mk
include $(BUILD_HOST_EXECUTABLE)
