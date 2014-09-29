## LICENSE
##
##

LOCAL_PATH := $(call my-dir)

handlers:= $(wildcard $(LOCAL_PATH)/*.c)
handlers:= $(handlers:$(LOCAL_PATH)/%.c=%)

define NEW_HANDLER
include $$(CLEAR_VARS)

LOCAL_CFLAGS:= -Wall -Werror
#LOCAL_SHARED_LIBRARIES:= dSploitCommon

LOCAL_ALLOW_UNDEFINED_SYMBOLS:= true

LOCAL_C_INCLUDES:= \
  dSploitCommon/ \
  dSploitd/

LOCAL_SRC_FILES:= $1.c

LOCAL_MODULE:= handlers_$1

include $$(BUILD_SHARED_LIBRARY)

endef 

$(foreach h, $(handlers), $(eval $(call NEW_HANDLER,$(h))))