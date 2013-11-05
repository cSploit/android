# # check out the 'android build cookbook' page
# # for some good definitions
# 
# LOCAL_PATH:= $(call my-dir)
# include $(CLEAR_VARS)
# include $(LOCAL_PATH)/Config.mk
# 
# LOCAL_MODULE:= libpgport
# 
# #LOCAL_REQUIRED_MODULES:= libssl libcrypto
# #LOCAL_SHARED_LIBRARIES+= libssl libcrypto
# 
# LOCAL_ARM_MODE:= arm
# 
# LOCAL_SRC_FILES:= $(SRC_FILES)
# 
# LOCAL_CFLAGS:= -I$(LOCAL_PATH) \
# 	-I$(LOCAL_PATH)/../include \
# 	-I$(LOCAL_PATH)/../backend
# 
# include $(BUILD_STATIC_LIBRARY)
