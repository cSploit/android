LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

subdirs := $(addprefix $(LOCAL_PATH)/,$(addsuffix /Android.mk, \
	port \
	interfaces/libpq \
	))
#removed: bin/psql
PGSQL_SRCDIR:=$(LOCAL_PATH)

include $(subdirs)
