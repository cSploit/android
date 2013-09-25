LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
include $(LOCAL_PATH)/android-config.mk


LOCAL_CFLAGS :=  -DNO_WINDOWS_BRAINDEATH -DOPENSSL_BN_ASM_MONT -DSHA1_AS 

ifeq ($(TARGET_ARCH),arm) 
    LOCAL_SRC_FILES := sha/sha1-armv4-large.S 
endif 
ifeq ($(TARGET_ARCH),x86) 
    LOCAL_CFLAGS :=$(LOCAL_CFLAGS) -msse2 -m32 -march=i686 -mtune=atom 
    LOCAL_SRC_FILES := sha/sha1-586.S
 
endif 
ifeq ($(TARGET_ARCH),mips) 
    LOCAL_CFLAGS :=$(LOCAL_CFLAGS) -march=mips1
    LOCAL_SRC_FILES := sha/sha1-mips.S
 
endif 

LOCAL_SRC_FILES := $(LOCAL_SRC_FILES) \
	sha/sha1dgst.c \
	thomsonDic.c

LOCAL_C_INCLUDES := $(LOCAL_PATH)/include
LOCAL_LDLIBS := -llog

LOCAL_MODULE:= thomson

include $(BUILD_SHARED_LIBRARY)
