LOCAL_PATH := $(call my-dir)
MY_LOCAL_PATH := $(LOCAL_PATH)
include $(CLEAR_VARS)

include $(MY_LOCAL_PATH)/thomson/Android.mk
include $(MY_LOCAL_PATH)/arpspoof/Android.mk
include $(MY_LOCAL_PATH)/libpcap/Android.mk
include $(MY_LOCAL_PATH)/libnet/Android.mk
include $(MY_LOCAL_PATH)/openssl/Android.mk
include $(MY_LOCAL_PATH)/tcpdump/Android.mk
