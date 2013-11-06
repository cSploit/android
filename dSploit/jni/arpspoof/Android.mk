LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE := arpspoof

LOCAL_SRC_FILES := hashmap.c \
									 arp.c \
									 net.c \
                   arpspoof.c
                   
APP_OPTIM := release

LOCAL_CFLAGS:= -static -ffunction-sections -fdata-sections
LOCAL_LDFLAGS += -Wl,--gc-sections

LOCAL_C_INCLUDES := libpcap libnet/include include
LOCAL_STATIC_LIBRARIES := libpcap libnet

include $(BUILD_EXECUTABLE)