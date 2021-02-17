LOCAL_PATH := $(call my-dir)

#original path: src/.libs/libedit.a
include $(CLEAR_VARS)

LOCAL_CFLAGS:= -DHAVE_CONFIG_H 

GENERATED_DIR := generated/android

GENERATED_SRCS := \
	fcns.c\
	help.c

GENERATED_SRCS := $(addprefix $(GENERATED_DIR)/src/, $(GENERATED_SRCS))
	
define make_include
test -d include/editline || mkdir -p include/editline
cp src/editline/readline.h include/editline/readline.h
endef

$(info $(shell $(LOCAL_PATH)/$(GENERATED_DIR)/make-include.sh $(LOCAL_PATH)))

# fixed flags
LOCAL_CFLAGS+= -ffunction-sections -fdata-sections

LOCAL_C_INCLUDES:= \
	$(LOCAL_PATH)/src\
	$(LOCAL_PATH)\
	$(LOCAL_PATH)/$(GENERATED_DIR)/src\
	$(LOCAL_PATH)/$(GENERATED_DIR)\
	external/ncurses/include
LOCAL_SRC_FILES:= \
	src/chared.c\
	src/common.c\
	src/el.c\
	src/emacs.c\
	src/hist.c\
	src/keymacro.c\
	src/map.c\
	src/chartype.c\
	src/parse.c\
	src/prompt.c\
	src/read.c\
	src/refresh.c\
	src/search.c\
	src/sig.c\
	src/terminal.c\
	src/tty.c\
	src/vi.c\
	src/fgetln.c\
	src/wcsdup.c\
	src/tokenizer.c\
	src/history.c\
	src/filecomplete.c\
	src/readline.c\
	src/vis.c\
	src/unvis.c
	
LOCAL_SRC_FILES += $(GENERATED_SRCS)

LOCAL_STATIC_LIBRARIES:= \
	ncurses
	
LOCAL_MODULE := libedit

include $(BUILD_STATIC_LIBRARY)


