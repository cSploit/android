LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_CFLAGS:= -DFRONTEND -DSO_MAJOR_VERSION=5 -DUNSAFE_STAT_OK -ffunction-sections \
-fdata-sections -DOPENSSL_NO_DEPRECATED

LOCAL_C_INCLUDES:= 	$(LOCAL_PATH)/src/backend\
										$(LOCAL_PATH)/src/port\
										$(LOCAL_PATH)/generated/bionic/src/port\
										$(LOCAL_PATH)/src/include\
										$(LOCAL_PATH)/generated/bionic/src/include\
										external/openssl/include

LOCAL_SRC_FILES := 	src/interfaces/libpq/fe-auth.c\
										src/interfaces/libpq/fe-connect.c\
										src/interfaces/libpq/fe-exec.c\
										src/interfaces/libpq/fe-misc.c\
										src/interfaces/libpq/fe-print.c\
										src/interfaces/libpq/fe-lobj.c\
										src/interfaces/libpq/fe-protocol2.c\
										src/interfaces/libpq/fe-protocol3.c\
										src/interfaces/libpq/pqexpbuffer.c\
										src/interfaces/libpq/pqsignal.c\
										src/interfaces/libpq/fe-secure.c\
										src/interfaces/libpq/libpq-events.c\
										src/backend/libpq/md5.c\
										src/backend/libpq/ip.c\
										src/backend/utils/mb/wchar.c\
										src/backend/utils/mb/encnames.c\
										src/port/noblock.c\
										src/port/pgstrcasecmp.c\
										src/port/thread.c\
										src/port/strlcpy.c\
										src/port/chklocale.c\
										src/port/dirmod.c\
										src/port/exec.c\
										src/port/path.c\
										src/port/pgsleep.c\
										src/port/qsort.c\
										src/port/inet_net_ntop.c\
										src/port/qsort_arg.c\
										src/port/sprompt.c\
										src/port/strlcat.c\
										src/port/getpeereid.c\
										src/port/snprintf.c\
										src/backend/storage/file/copydir.c

LOCAL_SHARED_LIBRARIES:= libssl libcrypto

LOCAL_MODULE:= libpq

include $(BUILD_STATIC_LIBRARY)
