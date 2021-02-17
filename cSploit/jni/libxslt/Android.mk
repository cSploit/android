LOCAL_PATH:= $(call my-dir)

# We need to build this for both the device (as a shared library)
# and the host (as a static library for tools to use).

common_SRC_FILES := \
	libxslt/attrvt.c \
	libxslt/namespaces.c \
	libxslt/security.c \
	libxslt/xsltlocale.c \
	libxslt/extensions.c \
	libxslt/numbers.c \
	libxslt/extra.c \
	libxslt/keys.c \
	libxslt/attributes.c \
	libxslt/imports.c \
	libxslt/variables.c \
	libxslt/xsltutils.c \
	libxslt/xslt.c \
	libxslt/transform.c \
	libxslt/preproc.c \
	libxslt/templates.c \
	libxslt/documents.c \
	libxslt/functions.c \
	libxslt/pattern.c

common_C_INCLUDES += \
	$(LOCAL_PATH)/libxslt

LOCAL_STATIC_LIBRARIES := libxml2
# For the device
# =====================================================

include $(CLEAR_VARS)

LOCAL_SRC_FILES := $(common_SRC_FILES)
LOCAL_C_INCLUDES += $(common_C_INCLUDES) external/libxml2/include external/icu4c/common
LOCAL_SHARED_LIBRARIES += $(common_SHARED_LIBRARIES)
LOCAL_STATIC_LIBRARIES := libxml2 icuuc icui18n
LOCAL_MODULE:= libxslt

include $(BUILD_STATIC_LIBRARY)


# For the host
# ========================================================

include $(CLEAR_VARS)
LOCAL_SRC_FILES := $(common_SRC_FILES)
LOCAL_C_INCLUDES += $(common_C_INCLUDES) external/libxml2/include external/icu4c/common
LOCAL_SHARED_LIBRARIES += $(common_SHARED_LIBRARIES)
LOCAL_MODULE:= libxslt
include $(BUILD_HOST_STATIC_LIBRARY)

common_SRC_FILES := \
	libexslt/exslt.c				\
	libexslt/common.c			\
	libexslt/crypto.c			\
	libexslt/math.c				\
	libexslt/sets.c				\
	libexslt/functions.c			\
	libexslt/strings.c			\
	libexslt/date.c				\
	libexslt/saxon.c				\
	libexslt/dynamic.c

common_C_INCLUDES += \
	$(LOCAL_PATH)/libexslt
	
LOCAL_STATIC_LIBRARIES := libxml2

# For the device
# =====================================================

include $(CLEAR_VARS)

LOCAL_SRC_FILES := $(common_SRC_FILES)
LOCAL_C_INCLUDES += $(common_C_INCLUDES) external/libxml2/include external/icu4c/common
LOCAL_SHARED_LIBRARIES += $(common_SHARED_LIBRARIES)
LOCAL_STATIC_LIBRARIES := libxslt libxml2 icuuc icui18n
LOCAL_MODULE:= libexslt

include $(BUILD_STATIC_LIBRARY)
	
