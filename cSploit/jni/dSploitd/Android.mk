## LICENSE
##
##

LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_CFLAGS:= -Wall -Werror
LOCAL_LDFLAGS:= -Wl,-export-dynamic -rdynamic
LOCAL_STATIC_LIBRARIES:= dSploitCommon_static

LOCAL_C_INCLUDES:= \
  dSploitCommon/
  
LOCAL_SRC_FILES:= $(wildcard $(LOCAL_PATH)/*.c)

LOCAL_MODULE:= dSploitd

include $(BUILD_EXECUTABLE)