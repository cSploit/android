LOCAL_PATH := $(call my-dir)
MY_LOCAL_PATH := $(LOCAL_PATH)
include $(CLEAR_VARS)

LOCAL_MODULE := arpspoof

LOCAL_SRC_FILES := arpspoof/arp.c  \
                   arpspoof/arpspoof.c \
		   arpspoof/ensure_death.c
                   
APP_OPTIM := release
LOCAL_C_INCLUDES := libpcap libnet/include include
LOCAL_STATIC_LIBRARIES := libpcap libnet
include $(BUILD_EXECUTABLE)
include $(MY_LOCAL_PATH)/libpcap/Android.mk
include $(MY_LOCAL_PATH)/libnet/Android.mk
