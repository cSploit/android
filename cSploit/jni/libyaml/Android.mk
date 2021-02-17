LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
 
LOCAL_CFLAGS := -DYAML_VERSION_STRING=\"0.1.4\" \
				-DYAML_VERSION_MAJOR=0 \
				-DYAML_VERSION_MINOR=1 \
				-DYAML_VERSION_PATCH=4

LOCAL_C_INCLUDES := $(LOCAL_PATH)/include
				
LOCAL_SRC_FILES  := \
	src/api.c \
	src/reader.c\
	src/scanner.c \
	src/parser.c \
	src/loader.c \
	src/writer.c \
	src/emitter.c \
	src/dumper.c

LOCAL_MODULE     := libyaml

include $(BUILD_STATIC_LIBRARY)