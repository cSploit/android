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
##
##

LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_CFLAGS:= -Wall -Werror -DHAVE_NET_ETHERNET_H -DHAVE_LINUX_IF_ETHER_H \
  -DHAVE_NET_IF_ARP_H -DHAVE_NETINET_IP_H -DHAVE_NETINET_UDP_H -DPROFILE -DBUG81370
LOCAL_STATIC_LIBRARIES:= cSploitCommon_static cares

LOCAL_C_INCLUDES:= \
  cSploitCommon/\
  libcares/
  
LOCAL_SRC_FILES:= $(wildcard $(LOCAL_PATH)/*.c)

LOCAL_MODULE:= network-radar

include $(BUILD_EXECUTABLE)
