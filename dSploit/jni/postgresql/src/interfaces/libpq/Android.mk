# check out the 'android build cookbook' page
# for some good definitions

LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)
include $(LOCAL_PATH)/Config.mk

LOCAL_MODULE:= libpq
# only heavily used modules should only prelinked
LOCAL_PRELINK_MODULE:= false

LOCAL_ARM_MODE:= arm

LOCAL_REQUIRED_MODULES:= libssl libcrypto	

LOCAL_STATIC_LIBRARIES+= libssl libcrypto

LOCAL_SRC_FILES:= $(SRC_FILES)

LOCAL_CFLAGS:= -DFRONTEND -DSO_MAJOR_VERSION=5 -DUNSAFE_STAT_OK \

LOCAL_C_INCLUDES:= $(INCLUDES)
	
# need to add openssl/ssl.h openssl/crypto.h because
# they are not 'installed' into the out/ directory

LOCAL_CFLAGS+= -I$(LOCAL_PATH)/../../../../openssl/include/

# TODO: figure out rpath (maybe it's already added?)
# LOCAL_LDFLAGS:= -Wl,--version-script=$(LOCAL_PATH)/exports.list

include $(BUILD_STATIC_LIBRARY)
