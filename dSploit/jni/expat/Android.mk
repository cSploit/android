LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_CFLAGS:= -Wall -Wmissing-prototypes -Wstrict-prototypes -fexceptions -ffunction-sections -fdata-sections -DHAVE_EXPAT_CONFIG_H

LOCAL_CXXFLAGS:=-Wall -fexceptions -ffunction-sections -fdata-sections

LOCAL_C_INCLUDES:=	expat\
					expat/lib

LOCAL_SRC_FILES:= 	lib/xmlparse.c\
					lib/xmltok.c\
					lib/xmlrole.c
					
LOCAL_MODULE:= libexpat

include $(BUILD_STATIC_LIBRARY)