## LICENSE
##
##

LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_CFLAGS:= -Wall -Werror
LOCAL_EXPORT_LDLIBS:= -ldl
  
LOCAL_SRC_FILES:= $(wildcard $(LOCAL_PATH)/*.c)

LOCAL_MODULE:= dSploitCommon

include $(BUILD_SHARED_LIBRARY)

include $(CLEAR_VARS)

LOCAL_CFLAGS:= -Wall -Werror
LOCAL_EXPORT_LDLIBS:= -ldl
  
LOCAL_SRC_FILES:= $(wildcard $(LOCAL_PATH)/*.c)

LOCAL_MODULE:= dSploitCommon_static

include $(BUILD_STATIC_LIBRARY)