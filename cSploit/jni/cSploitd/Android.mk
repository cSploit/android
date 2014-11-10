## LICENSE
##
##

LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_CFLAGS:= -Wall -Werror
LOCAL_LDFLAGS:= -Wl,-export-dynamic -rdynamic
LOCAL_STATIC_LIBRARIES:= cSploitCommon_static

LOCAL_C_INCLUDES:= \
  cSploitCommon/
  
LOCAL_SRC_FILES:= $(wildcard $(LOCAL_PATH)/*.c)

LOCAL_MODULE:= cSploitd

include $(BUILD_EXECUTABLE)