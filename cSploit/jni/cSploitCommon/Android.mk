## LICENSE
##
##

LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_CFLAGS:= -Wall -Werror
LOCAL_EXPORT_LDLIBS:= -ldl
  
LOCAL_SRC_FILES:= $(wildcard $(LOCAL_PATH)/*.c)

LOCAL_MODULE:= cSploitCommon

include $(BUILD_SHARED_LIBRARY)

include $(CLEAR_VARS)

LOCAL_CFLAGS:= -Wall -Werror
LOCAL_EXPORT_LDLIBS:= -ldl
  
LOCAL_SRC_FILES:= $(wildcard $(LOCAL_PATH)/*.c)

LOCAL_MODULE:= cSploitCommon_static

include $(BUILD_STATIC_LIBRARY)