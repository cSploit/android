## LICENSE
##
##

LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_CFLAGS:= -Wall -Werror -D_U_="__attribute__((unused))"
LOCAL_SHARED_LIBRARIES:= dSploitCommon
# LOCAL_WHOLE_STATIC_LIBRARIES:= dSploitCommon_static
LOCAL_LDLIBS:= -llog

LOCAL_C_INCLUDES:= \
  dSploitCommon/ \
  dSploitHandlers/
  
LOCAL_SRC_FILES:= $(wildcard $(LOCAL_PATH)/*.c)

LOCAL_MODULE:= dSploitClient

include $(BUILD_SHARED_LIBRARY)
