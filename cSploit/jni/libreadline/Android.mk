LOCAL_PATH := $(call my-dir)

#original path: libreadline.a
include $(CLEAR_VARS)

LOCAL_CFLAGS:= -DHAVE_CONFIG_H -DRL_LIBRARY_VERSION='"6.3"'

# fixed flags
LOCAL_CFLAGS+= -ffunction-sections -fdata-sections

LOCAL_C_INCLUDES:= \
	$(LOCAL_PATH)
LOCAL_SRC_FILES:= \
	readline.c\
	vi_mode.c\
	funmap.c\
	keymaps.c\
	parens.c\
	search.c\
	rltty.c\
	complete.c\
	bind.c\
	isearch.c\
	display.c\
	signals.c\
	util.c\
	kill.c\
	undo.c\
	macro.c\
	input.c\
	callback.c\
	terminal.c\
	text.c\
	nls.c\
	misc.c\
	history.c\
	histexpand.c\
	histfile.c\
	histsearch.c\
	shell.c\
	mbutil.c\
	tilde.c\
	colors.c\
	parse-colors.c\
	xmalloc.c\
	xfree.c\
	compat.c
LOCAL_STATIC_LIBRARIES:= \
	ncurses
LOCAL_MODULE := libreadline

include $(BUILD_STATIC_LIBRARY)


#original path: libhistory.a
include $(CLEAR_VARS)

LOCAL_CFLAGS:= -DHAVE_CONFIG_H -DRL_LIBRARY_VERSION='"6.3"' 

# fixed flags
LOCAL_CFLAGS+= -ffunction-sections -fdata-sections

LOCAL_C_INCLUDES:= \
	$(LOCAL_PATH)
LOCAL_SRC_FILES:= \
	history.c\
	histexpand.c\
	histfile.c\
	histsearch.c\
	shell.c\
	mbutil.c\
	xmalloc.c\
	xfree.c
LOCAL_MODULE := libhistory

include $(BUILD_STATIC_LIBRARY)

