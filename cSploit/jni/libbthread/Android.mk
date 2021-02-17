# Copyright (C) 2002 Free Software Foundation, Inc.
# This file is part of the GNU C Library.
#
# The GNU C Library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Library General Public License as
# published by the Free Software Foundation; either version 2 of the
# License, or (at your option) any later version.
#
# The GNU C Library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Library General Public License for more details.
#
# You should have received a copy of the GNU Library General Public
# License along with the GNU C Library; see the file COPYING.LIB.  If not,
# write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
# Boston, MA 02111-1307, USA.

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
