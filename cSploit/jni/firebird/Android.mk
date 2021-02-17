LOCAL_PATH := $(call my-dir)

#merged libcommon and libfbclient
include $(CLEAR_VARS)

LOCAL_CFLAGS:= -DFB_SEND_FLAGS=MSG_NOSIGNAL -DLINUX -DAMD64 \
-fmessage-length=0 -fno-omit-frame-pointer -fexceptions

LOCAL_CPPFLAGS:= -fno-rtti

# fixed flags
LOCAL_CFLAGS+= -ffunction-sections -fdata-sections

LOCAL_C_INCLUDES:= \
	$(LOCAL_PATH)/src/include/gen\
	$(LOCAL_PATH)/src/include\
	external/icu4c/common\
	external/ncurses/include\
	external/libtommath\
	external/icu4c/i18n
LOCAL_SRC_FILES:= \
	src/common/security.cpp\
	src/common/isc_sync.cpp\
	src/common/StatusArg.cpp\
	src/common/TextType.cpp\
	src/common/dsc.cpp\
	src/common/utils.cpp\
	src/common/IntlParametersBlock.cpp\
	src/common/UtilSvc.cpp\
	src/common/db_alias.cpp\
	src/common/xdr.cpp\
	src/common/ThreadStart.cpp\
	src/common/CharSet.cpp\
	src/common/pretty.cpp\
	src/common/IntlUtil.cpp\
	src/common/Auth.cpp\
	src/common/thd.cpp\
	src/common/isc_file.cpp\
	src/common/fb_exception.cpp\
	src/common/ThreadData.cpp\
	src/common/dllinst.cpp\
	src/common/cvt.cpp\
	src/common/enc.cpp\
	src/common/BigInteger.cpp\
	src/common/unicode_util.cpp\
	src/common/ScanDir.cpp\
	src/common/isc.cpp\
	src/common/StatusHolder.cpp\
	src/common/StatementMetadata.cpp\
	src/common/call_service.cpp\
	src/common/MsgMetadata.cpp\
	src/common/sdl.cpp\
	src/common/sha.cpp\
	src/common/os/posix/isc_ipc.cpp\
	src/common/os/posix/SyncSignals.cpp\
	src/common/os/posix/os_utils.cpp\
	src/common/os/posix/path_utils.cpp\
	src/common/os/posix/mod_loader.cpp\
	src/common/os/posix/guid.cpp\
	src/common/os/posix/divorce.cpp\
	src/common/os/posix/fbsyslog.cpp\
	src/common/classes/NoThrowTimeStamp.cpp\
	src/common/classes/SyncObject.cpp\
	src/common/classes/ClumpletWriter.cpp\
	src/common/classes/ClumpletReader.cpp\
	src/common/classes/DbImplementation.cpp\
	src/common/classes/BlrWriter.cpp\
	src/common/classes/UserBlob.cpp\
	src/common/classes/BaseStream.cpp\
	src/common/classes/semaphore.cpp\
	src/common/classes/fb_string.cpp\
	src/common/classes/TempFile.cpp\
	src/common/classes/locks.cpp\
	src/common/classes/alloc.cpp\
	src/common/classes/SafeArg.cpp\
	src/common/classes/MsgPrint.cpp\
	src/common/classes/MetaName.cpp\
	src/common/classes/InternalMessageBuffer.cpp\
	src/common/classes/Switches.cpp\
	src/common/classes/ImplementHelper.cpp\
	src/common/classes/Synchronize.cpp\
	src/common/classes/init.cpp\
	src/common/classes/timestamp.cpp\
	src/common/config/ConfigCache.cpp\
	src/common/config/config_file.cpp\
	src/common/config/dir_list.cpp\
	src/common/config/config.cpp\
	src/common/config/os/posix/config_root.cpp\
	src/common/config/os/posix/binreloc.c\
	src/yvalve/MasterImplementation.cpp\
	src/yvalve/alt.cpp\
	src/yvalve/preparse.cpp\
	src/yvalve/perf.cpp\
	src/yvalve/user_dsql.cpp\
	src/yvalve/gds.cpp\
	src/yvalve/why.cpp\
	src/yvalve/PluginManager.cpp\
	src/yvalve/keywords.cpp\
	src/yvalve/utl.cpp\
	src/yvalve/DistributedTransaction.cpp\
	temp/Release/yvalve/blob.cpp\
	temp/Release/yvalve/array.cpp\
	src/remote/remote.cpp\
	src/remote/protocol.cpp\
	src/remote/merge.cpp\
	src/remote/inet.cpp\
	src/remote/parser.cpp\
	src/auth/SecureRemotePassword/srp.cpp\
	src/remote/client/interface.cpp\
	src/remote/client/BlrFromMessage.cpp\
	src/auth/SecureRemotePassword/client/SrpClient.cpp\
	src/auth/SecurityDatabase/LegacyClient.cpp\
	src/plugins/crypt/arc4/Arc4.cpp
LOCAL_STATIC_LIBRARIES:= \
	libncurses\
	libtommath
LOCAL_MODULE := libfbclient

include $(BUILD_STATIC_LIBRARY)

#original path: gen/Release/firebird/bin/firebird
include $(CLEAR_VARS)


LOCAL_CPPFLAGS:= -DFB_SEND_FLAGS=MSG_NOSIGNAL -DLINUX -DAMD64 \
-fmessage-length=0 -fno-omit-frame-pointer -fno-rtti 

LOCAL_LDFLAGS:= -Wl,--version-script,$(LOCAL_PATH)/gen/empty.vers 

# fixed flags
LOCAL_CFLAGS+= -ffunction-sections -fdata-sections
LOCAL_LDFLAGS+=  -Wl,--gc-sections

LOCAL_C_INCLUDES:= \
	$(LOCAL_PATH)/src/include/gen\
	$(LOCAL_PATH)/src/include\
	external/icu4c/common\
	external/ncurses/include\
	external/libtommath\
	external/icu4c/i18n
LOCAL_SRC_FILES:= \
	src/remote/server/server.cpp\
	src/remote/server/os/posix/inet_server.cpp\
	src/auth/SecureRemotePassword/server/SrpServer.cpp
LOCAL_STATIC_LIBRARIES:= \
	libfbclient\
	libncurses\
	libtommath
LOCAL_MODULE := firebird

include $(BUILD_EXECUTABLE)

