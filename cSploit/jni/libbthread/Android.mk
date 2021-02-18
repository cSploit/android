LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES:=\
	pt-cancel.c\
	pt-docancel.c\
	pt-setcancelstate.c\
	pt-setcanceltype.c\
	pt-testcancel.c\
	pt-init.c

LOCAL_C_INCLUDES:=libbthread/
	
LOCAL_CFLAGS:= -ffunction-sections -fdata-sections

LOCAL_MODULE:= libbthread

include $(BUILD_STATIC_LIBRARY)

include $(CLEAR_VARS)

LOCAL_SRC_FILES:=\
	pt-test.c
	
LOCAL_C_INCLUDES:=libbthread/

LOCAL_CFLAGS:= -ffunction-sections -fdata-sections

LOCAL_MODULE:= bthread_test

include $(BUILD_EXECUTABLE)