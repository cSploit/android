#!/usr/bin/env ruby

ANDROID_MK='LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_ALLOW_UNDEFINED_SYMBOLS := true

LOCAL_CFLAGS := -DHAVE_CONFIG_H

LOCAL_C_INCLUDES := $(ettercap_pl_includes)

LOCAL_SRC_FILES := @SOURCES@

LOCAL_MODULE := @NAME@

include $(BUILD_SHARED_LIBRARY)'

ARGV.each do |d|
	next if !Dir.exists? d
	
	android_mk_path="#{d}/Android.mk"
	makefile_am_path="#{d}/Makefile.am"
	
	if !File.exists? makefile_am_path then
		warn "#{makefile_am_path} does not exists"
		next
	end
	
	plugin_sources=plugin_name=nil
	
	File.read(makefile_am_path).each_line do |l|
		case l
			when /^ec_[a-z0-9_]+_SOURCES\s*=\s*(.*)/
				plugin_sources=$1
			when /=\s*(ec_.*)\./
				plugin_name=$1
		end
	end
	
	content=ANDROID_MK.sub("@SOURCES@",plugin_sources).sub("@NAME@",plugin_name)
	
	File.write(android_mk_path,content)

end