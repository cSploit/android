LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

subdirs := $(addprefix $(LOCAL_PATH)/,$(addsuffix /Android.mk, \
	toolutil \
	ctestfw  \
	makeconv \
	genrb    \
	genuca   \
	genbrk   \
	genctd   \
	gennames \
	genpname \
	gencnval \
	gensprep \
	genccode \
	gencmn   \
	icupkg   \
	pkgdata  \
	genprops \
	gencase  \
	genbidi  \
	gennorm  \
	icuswap  \
	))

include $(subdirs)
