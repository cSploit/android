## cSploit - a simple penetration testing suite
## Copyright (C) 2014  Massimo Dragano aka tux_mind <tux_mind@csploit.org>
## 
## cSploit is free software: you can redistribute it and/or modify
## it under the terms of the GNU General Public License as published by
## the Free Software Foundation, either version 3 of the License, or
## (at your option) any later version.
## 
## cSploit is distributed in the hope that it will be useful,
## but WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
## GNU General Public License for more details.
## 
## You should have received a copy of the GNU General Public License
## along with cSploit.  If not, see <http://www.gnu.org/licenses/>.

LOCAL_PATH := $(call my-dir)

handlers:= $(wildcard $(LOCAL_PATH)/*.c)
handlers:= $(handlers:$(LOCAL_PATH)/%.c=%)

define NEW_HANDLER
include $$(CLEAR_VARS)

LOCAL_CFLAGS:= -Wall -Werror
#LOCAL_SHARED_LIBRARIES:= cSploitCommon

LOCAL_ALLOW_UNDEFINED_SYMBOLS:= true

LOCAL_C_INCLUDES:= \
  cSploitCommon/ \
  cSploitDaemon/

LOCAL_SRC_FILES:= $1.c

LOCAL_MODULE:= handlers_$1

include $$(BUILD_SHARED_LIBRARY)

endef 

$(foreach h, $(handlers), $(eval $(call NEW_HANDLER,$(h))))
