LOCAL_PATH := $(call my-dir)

# #original path: extern/editline/src/libedit.la
# include $(CLEAR_VARS)
# 
# LOCAL_CFLAGS:= -DHAVE_CONFIG_H 
# 
# LOCAL_C_INCLUDES:= \
# 	firebird/extern/editline/src\
# 	firebird/extern/editline\
# 	icu/androidbuild/icu_build/include\
# 	libtommath\
# 	ncurses/include
# LOCAL_STATIC_LIBRARIES:= \
# 	libncurses
# LOCAL_MODULE := libedit
# 
# include $(BUILD_STATIC_LIBRARY)


#original path: extern/editline/src/.libs/libedit.a
include $(CLEAR_VARS)

LOCAL_CFLAGS:= -DHAVE_CONFIG_H -ffunction-sections -fdata-sections

LOCAL_C_INCLUDES:= \
	firebird/extern/editline/src\
	firebird/extern/editline\
	icu/androidbuild/icu_build/include\
	libtommath\
	ncurses/include
LOCAL_SRC_FILES:= \
	extern/editline/src/chared.c\
	extern/editline/src/common.c\
	extern/editline/src/el.c\
	extern/editline/src/emacs.c\
	extern/editline/src/hist.c\
	extern/editline/src/key.c\
	extern/editline/src/map.c\
	extern/editline/src/parse.c\
	extern/editline/src/prompt.c\
	extern/editline/src/read.c\
	extern/editline/src/refresh.c\
	extern/editline/src/search.c\
	extern/editline/src/sig.c\
	extern/editline/src/term.c\
	extern/editline/src/tty.c\
	extern/editline/src/vi.c\
	extern/editline/src/fgetln.c\
	extern/editline/src/strlcat.c\
	extern/editline/src/strlcpy.c\
	extern/editline/src/unvis.c\
	extern/editline/src/vis.c\
	extern/editline/src/tokenizer.c\
	extern/editline/src/history.c\
	extern/editline/src/filecomplete.c\
	extern/editline/src/readline.c\
	extern/editline/src/fcns.c\
	extern/editline/src/help.c
LOCAL_MODULE := libedit

include $(BUILD_STATIC_LIBRARY)


#original path: gen/firebird/lib/libfbstatic.a
include $(CLEAR_VARS)

LOCAL_CFLAGS:= -DBOOT_BUILD -DNAMESPACE=Vulcan -DNDEBUG -DLINUX -DARM \
-fPIC -fsigned-char -fmessage-length=0 -ffunction-sections -fdata-sections
LOCAL_CPPFLAGS:= -fexceptions -ffunction-sections -fdata-sections

LOCAL_C_INCLUDES:= \
	icu/androidbuild/icu_build/include\
	libtommath\
	ncurses/include\
	firebird/src/include/gen\
	firebird/src/include\
	firebird/src/vulcan
LOCAL_SRC_FILES:= \
	src/jrd/os/posix/config_root.cpp\
	src/jrd/os/posix/path_utils.cpp\
	src/jrd/os/posix/mod_loader.cpp\
	src/jrd/os/posix/fbsyslog.cpp\
	src/jrd/os/posix/guid.cpp\
	src/jrd/os/posix/os_utils.cpp\
	src/common/dllinst.cpp\
	extern/binreloc/binreloc.c\
	src/jrd/alt.cpp\
	src/jrd/db_alias.cpp\
	src/jrd/dsc.cpp\
	src/jrd/enc.cpp\
	src/jrd/gds.cpp\
	src/jrd/isc.cpp\
	src/jrd/isc_file.cpp\
	src/jrd/os/posix/isc_ipc.cpp\
	src/jrd/perf.cpp\
	src/jrd/sdl.cpp\
	src/jrd/status.cpp\
	src/jrd/ThreadData.cpp\
	src/jrd/ThreadStart.cpp\
	src/jrd/utl.cpp\
	src/jrd/why.cpp\
	src/common/cvt.cpp\
	src/jrd/blob_filter.cpp\
	src/jrd/cvt.cpp\
	temp/boot/jrd/dpm.cpp\
	temp/boot/jrd/dyn.cpp\
	temp/boot/jrd/dyn_def.cpp\
	temp/boot/jrd/dyn_del.cpp\
	temp/boot/jrd/dyn_mod.cpp\
	temp/boot/jrd/dyn_util.cpp\
	temp/boot/jrd/fun.cpp\
	temp/boot/jrd/grant.cpp\
	temp/boot/jrd/ini.cpp\
	temp/boot/jrd/met.cpp\
	temp/boot/jrd/pcmet.cpp\
	temp/boot/jrd/scl.cpp\
	src/jrd/CharSet.cpp\
	src/jrd/Collation.cpp\
	src/jrd/DatabaseSnapshot.cpp\
	src/jrd/VirtualTable.cpp\
	src/jrd/RecordBuffer.cpp\
	src/jrd/blb.cpp\
	src/jrd/btn.cpp\
	src/jrd/btr.cpp\
	src/jrd/builtin.cpp\
	src/jrd/GlobalRWLock.cpp\
	src/jrd/cch.cpp\
	src/jrd/cmp.cpp\
	src/jrd/cvt2.cpp\
	src/jrd/DataTypeUtil.cpp\
	temp/boot/jrd/dfw.cpp\
	src/jrd/UserManagement.cpp\
	src/jrd/divorce.cpp\
	src/jrd/err.cpp\
	src/jrd/event.cpp\
	src/jrd/evl.cpp\
	src/jrd/exe.cpp\
	src/jrd/ext.cpp\
	src/jrd/execute_statement.cpp\
	src/jrd/filters.cpp\
	src/jrd/flu.cpp\
	src/jrd/functions.cpp\
	src/jrd/idx.cpp\
	src/jrd/inf.cpp\
	src/jrd/intl.cpp\
	src/jrd/intl_builtin.cpp\
	src/jrd/IntlManager.cpp\
	src/jrd/IntlUtil.cpp\
	src/jrd/isc_sync.cpp\
	src/jrd/jrd.cpp\
	src/jrd/Database.cpp\
	src/jrd/lck.cpp\
	src/jrd/mov.cpp\
	src/jrd/nav.cpp\
	src/jrd/opt.cpp\
	src/jrd/Optimizer.cpp\
	src/jrd/pag.cpp\
	src/jrd/par.cpp\
	src/jrd/ods.cpp\
	src/jrd/pwd.cpp\
	src/jrd/PreparedStatement.cpp\
	src/jrd/RandomGenerator.cpp\
	src/jrd/Relation.cpp\
	src/jrd/ResultSet.cpp\
	src/jrd/rlck.cpp\
	src/jrd/rpb_chain.cpp\
	src/jrd/rse.cpp\
	src/jrd/sdw.cpp\
	src/jrd/shut.cpp\
	src/jrd/sort.cpp\
	src/jrd/sqz.cpp\
	src/jrd/svc.cpp\
	src/jrd/SysFunction.cpp\
	src/jrd/TempSpace.cpp\
	src/jrd/tpc.cpp\
	src/jrd/tra.cpp\
	src/jrd/validation.cpp\
	src/jrd/vio.cpp\
	src/jrd/nodebug.cpp\
	src/jrd/nbak.cpp\
	src/jrd/sha.cpp\
	src/jrd/os/posix/unix.cpp\
	src/jrd/TextType.cpp\
	src/jrd/unicode_util.cpp\
	src/jrd/RuntimeStatistics.cpp\
	src/jrd/DebugInterface.cpp\
	src/jrd/extds/ExtDS.cpp\
	src/jrd/extds/InternalDS.cpp\
	src/jrd/extds/IscDS.cpp\
	src/jrd/trace/TraceConfigStorage.cpp\
	src/jrd/trace/TraceLog.cpp\
	src/jrd/trace/TraceManager.cpp\
	src/jrd/trace/TraceObjects.cpp\
	src/gpre/pretty.cpp\
	temp/boot/dsql/array.cpp\
	temp/boot/dsql/blob.cpp\
	src/dsql/preparse.cpp\
	src/dsql/user_dsql.cpp\
	src/dsql/utld.cpp\
	src/dsql/keywords.cpp\
	temp/boot/dsql/metd.cpp\
	src/dsql/ddl.cpp\
	src/dsql/dsql.cpp\
	src/dsql/errd.cpp\
	src/dsql/gen.cpp\
	src/dsql/hsh.cpp\
	src/dsql/make.cpp\
	src/dsql/movd.cpp\
	src/dsql/parse.cpp\
	src/dsql/Parser.cpp\
	src/dsql/pass1.cpp\
	src/dsql/misc_func.cpp\
	temp/boot/dsql/DdlNodes.cpp\
	src/dsql/StmtNodes.cpp\
	src/lock/lock.cpp\
	src/remote/interface.cpp\
	src/remote/inet.cpp\
	src/remote/merge.cpp\
	src/remote/parser.cpp\
	src/remote/protocol.cpp\
	src/remote/remote.cpp\
	src/remote/xdr.cpp\
	src/common/config/config.cpp\
	src/common/config/config_file.cpp\
	src/common/config/dir_list.cpp\
	src/common/classes/ClumpletReader.cpp\
	src/common/classes/ClumpletWriter.cpp
LOCAL_MODULE := libfbstatic

include $(BUILD_STATIC_LIBRARY)


#original path: gen/firebird/intl/libfbintl.so
include $(CLEAR_VARS)

LOCAL_CPPFLAGS:= -DSUPERCLIENT -DNAMESPACE=Vulcan -DNDEBUG -DLINUX -DARM \
-fPIC -fsigned-char -fmessage-length=0 -ffunction-sections -fdata-sections

LOCAL_C_INCLUDES:= \
	icu/androidbuild/icu_build/include\
	libtommath\
	ncurses/include\
	firebird/src/include/gen\
	firebird/src/include\
	firebird/src/vulcan
LOCAL_SRC_FILES:= \
	src/intl/ld.cpp\
	src/intl/cv_narrow.cpp\
	src/intl/cs_narrow.cpp\
	src/intl/lc_ascii.cpp\
	src/intl/lc_narrow.cpp\
	src/intl/lc_iso8859_1.cpp\
	src/intl/lc_iso8859_2.cpp\
	src/intl/lc_iso8859_13.cpp\
	src/intl/lc_dos.cpp\
	src/intl/cs_unicode_ucs2.cpp\
	src/intl/lc_unicode_ucs2.cpp\
	src/intl/cs_unicode_fss.cpp\
	src/intl/cv_unicode_fss.cpp\
	src/intl/cs_big5.cpp\
	src/intl/cv_big5.cpp\
	src/intl/lc_big5.cpp\
	src/intl/cs_gb2312.cpp\
	src/intl/cv_gb2312.cpp\
	src/intl/lc_gb2312.cpp\
	src/intl/cs_jis.cpp\
	src/intl/cv_jis.cpp\
	src/intl/lc_jis.cpp\
	src/intl/cs_ksc.cpp\
	src/intl/cv_ksc.cpp\
	src/intl/lc_ksc.cpp\
	src/intl/cs_icu.cpp\
	src/intl/cv_icu.cpp\
	src/intl/lc_icu.cpp\
	src/jrd/IntlUtil.cpp\
	src/jrd/unicode_util.cpp\
	src/jrd/CharSet.cpp\
	src/jrd/os/posix/mod_loader.cpp\
	src/common/fb_exception.cpp\
	src/common/thd.cpp\
	src/common/classes/MetaName.cpp\
	src/common/StatusHolder.cpp\
	src/common/classes/init.cpp\
	src/common/StatusArg.cpp\
	src/common/classes/alloc.cpp\
	src/common/classes/locks.cpp\
	src/common/classes/fb_string.cpp\
	src/common/classes/timestamp.cpp
LOCAL_STATIC_LIBRARIES:= \
	libicuuc\
	libicudata\
	libicui18n\
	libncurses
LOCAL_MODULE := libfbintl

include $(BUILD_SHARED_LIBRARY)


#original path: gen/firebird/lib/libib_util.so
include $(CLEAR_VARS)

LOCAL_CPPFLAGS:= -DNAMESPACE=Vulcan -DNDEBUG -DLINUX -DARM -fPIC \
-fsigned-char -fmessage-length=0 -ffunction-sections -fdata-sections

LOCAL_C_INCLUDES:= \
	icu/androidbuild/icu_build/include\
	libtommath\
	ncurses/include\
	firebird/src/include/gen\
	firebird/src/include\
	firebird/src/vulcan
LOCAL_SRC_FILES:= \
	src/extlib/ib_util.cpp
LOCAL_MODULE := libib_util

include $(BUILD_SHARED_LIBRARY)


#original path: gen/firebird/UDF/ib_udf.so
include $(CLEAR_VARS)

LOCAL_CPPFLAGS:= -DNAMESPACE=Vulcan -DNDEBUG -DLINUX -DARM -fPIC \
-fsigned-char -fmessage-length=0 -ffunction-sections -fdata-sections

LOCAL_C_INCLUDES:= \
	icu/androidbuild/icu_build/include\
	libtommath\
	ncurses/include\
	firebird/src/include/gen\
	firebird/src/include\
	firebird/src/vulcan
LOCAL_SRC_FILES:= \
	src/extlib/ib_udf.cpp
LOCAL_STATIC_LIBRARIES:= \
	libib_util
LOCAL_MODULE := ib_udf

include $(BUILD_SHARED_LIBRARY)


#original path: gen/firebird/lib/libfbembed.so.2.5.2
include $(CLEAR_VARS)

LOCAL_CFLAGS:= -DBOOT_BUILD -DNAMESPACE=Vulcan -DNDEBUG -DLINUX -DARM \
-fPIC -fsigned-char -fmessage-length=0 -ffunction-sections -fdata-sections

LOCAL_C_INCLUDES:= \
	icu/androidbuild/icu_build/include\
	libtommath\
	ncurses/include\
	firebird/src/include/gen\
	firebird/src/include\
	firebird/src/vulcan
LOCAL_SRC_FILES:= \
	src/jrd/os/posix/config_root.cpp\
	src/jrd/os/posix/path_utils.cpp\
	src/jrd/os/posix/mod_loader.cpp\
	src/jrd/os/posix/fbsyslog.cpp\
	src/jrd/os/posix/guid.cpp\
	src/jrd/os/posix/os_utils.cpp\
	src/common/dllinst.cpp\
	extern/binreloc/binreloc.c\
	src/jrd/alt.cpp\
	src/jrd/db_alias.cpp\
	src/jrd/dsc.cpp\
	src/jrd/enc.cpp\
	src/jrd/gds.cpp\
	src/jrd/isc.cpp\
	src/jrd/isc_file.cpp\
	src/jrd/os/posix/isc_ipc.cpp\
	src/jrd/perf.cpp\
	src/jrd/sdl.cpp\
	src/jrd/status.cpp\
	src/jrd/ThreadData.cpp\
	src/jrd/ThreadStart.cpp\
	src/jrd/utl.cpp\
	src/jrd/why.cpp\
	src/common/cvt.cpp\
	src/jrd/blob_filter.cpp\
	src/jrd/cvt.cpp\
	temp/std/jrd/dpm.cpp\
	temp/std/jrd/dyn.cpp\
	temp/std/jrd/dyn_def.cpp\
	temp/std/jrd/dyn_del.cpp\
	temp/std/jrd/dyn_mod.cpp\
	temp/std/jrd/dyn_util.cpp\
	temp/std/jrd/fun.cpp\
	temp/std/jrd/grant.cpp\
	temp/std/jrd/ini.cpp\
	temp/std/jrd/met.cpp\
	temp/std/jrd/pcmet.cpp\
	temp/std/jrd/scl.cpp\
	src/jrd/CharSet.cpp\
	src/jrd/Collation.cpp\
	src/jrd/DatabaseSnapshot.cpp\
	src/jrd/VirtualTable.cpp\
	src/jrd/RecordBuffer.cpp\
	src/jrd/blb.cpp\
	src/jrd/btn.cpp\
	src/jrd/btr.cpp\
	src/jrd/builtin.cpp\
	src/jrd/GlobalRWLock.cpp\
	src/jrd/cch.cpp\
	src/jrd/cmp.cpp\
	src/jrd/cvt2.cpp\
	src/jrd/DataTypeUtil.cpp\
	temp/std/jrd/dfw.cpp\
	src/jrd/UserManagement.cpp\
	src/jrd/divorce.cpp\
	src/jrd/err.cpp\
	src/jrd/event.cpp\
	src/jrd/evl.cpp\
	src/jrd/exe.cpp\
	src/jrd/ext.cpp\
	src/jrd/execute_statement.cpp\
	src/jrd/filters.cpp\
	src/jrd/flu.cpp\
	src/jrd/functions.cpp\
	src/jrd/idx.cpp\
	src/jrd/inf.cpp\
	src/jrd/intl.cpp\
	src/jrd/intl_builtin.cpp\
	src/jrd/IntlManager.cpp\
	src/jrd/IntlUtil.cpp\
	src/jrd/isc_sync.cpp\
	src/jrd/jrd.cpp\
	src/jrd/Database.cpp\
	src/jrd/lck.cpp\
	src/jrd/mov.cpp\
	src/jrd/nav.cpp\
	src/jrd/opt.cpp\
	src/jrd/Optimizer.cpp\
	src/jrd/pag.cpp\
	src/jrd/par.cpp\
	src/jrd/ods.cpp\
	src/jrd/pwd.cpp\
	src/jrd/PreparedStatement.cpp\
	src/jrd/RandomGenerator.cpp\
	src/jrd/Relation.cpp\
	src/jrd/ResultSet.cpp\
	src/jrd/rlck.cpp\
	src/jrd/rpb_chain.cpp\
	src/jrd/rse.cpp\
	src/jrd/sdw.cpp\
	src/jrd/shut.cpp\
	src/jrd/sort.cpp\
	src/jrd/sqz.cpp\
	src/jrd/svc.cpp\
	src/jrd/SysFunction.cpp\
	src/jrd/TempSpace.cpp\
	src/jrd/tpc.cpp\
	src/jrd/tra.cpp\
	src/jrd/validation.cpp\
	src/jrd/vio.cpp\
	src/jrd/nodebug.cpp\
	src/jrd/nbak.cpp\
	src/jrd/sha.cpp\
	src/jrd/os/posix/unix.cpp\
	src/jrd/TextType.cpp\
	src/jrd/unicode_util.cpp\
	src/jrd/RuntimeStatistics.cpp\
	src/jrd/DebugInterface.cpp\
	src/jrd/extds/ExtDS.cpp\
	src/jrd/extds/InternalDS.cpp\
	src/jrd/extds/IscDS.cpp\
	src/jrd/trace/TraceConfigStorage.cpp\
	src/jrd/trace/TraceLog.cpp\
	src/jrd/trace/TraceManager.cpp\
	src/jrd/trace/TraceObjects.cpp\
	src/gpre/pretty.cpp\
	temp/std/dsql/array.cpp\
	temp/std/dsql/blob.cpp\
	src/dsql/preparse.cpp\
	src/dsql/user_dsql.cpp\
	src/dsql/utld.cpp\
	src/dsql/keywords.cpp\
	temp/std/dsql/metd.cpp\
	src/dsql/ddl.cpp\
	src/dsql/dsql.cpp\
	src/dsql/errd.cpp\
	src/dsql/gen.cpp\
	src/dsql/hsh.cpp\
	src/dsql/make.cpp\
	src/dsql/movd.cpp\
	src/dsql/parse.cpp\
	src/dsql/Parser.cpp\
	src/dsql/pass1.cpp\
	src/dsql/misc_func.cpp\
	temp/std/dsql/DdlNodes.cpp\
	src/dsql/StmtNodes.cpp\
	src/lock/lock.cpp\
	src/remote/interface.cpp\
	src/remote/inet.cpp\
	src/remote/merge.cpp\
	src/remote/parser.cpp\
	src/remote/protocol.cpp\
	src/remote/remote.cpp\
	src/remote/xdr.cpp\
	src/utilities/gsec/call_service.cpp\
	temp/std/utilities/gsec/security.cpp\
	src/utilities/gsec/gsec.cpp\
	temp/std/utilities/gstat/dba.cpp\
	src/utilities/gstat/ppg.cpp\
	src/common/classes/ClumpletReader.cpp\
	src/common/classes/ClumpletWriter.cpp\
	src/common/config/config.cpp\
	src/common/config/config_file.cpp\
	src/common/config/dir_list.cpp\
	src/burp/burp.cpp\
	temp/std/burp/backup.cpp\
	temp/std/burp/restore.cpp\
	src/burp/mvol.cpp\
	src/burp/misc.cpp\
	src/burp/canonical.cpp\
	src/alice/alice.cpp\
	src/alice/exe.cpp\
	temp/std/alice/alice_meta.cpp\
	src/alice/tdr.cpp\
	src/utilities/nbackup.cpp\
	src/jrd/trace/TraceCmdLine.cpp\
	src/jrd/trace/TraceService.cpp\
	src/remote/inet_server.cpp\
	src/remote/server.cpp\
	src/common/classes/alloc.cpp\
	src/common/classes/locks.cpp\
	src/common/classes/semaphore.cpp\
	src/common/classes/fb_string.cpp\
	src/common/classes/timestamp.cpp\
	src/common/classes/PublicHandle.cpp\
	src/common/classes/TempFile.cpp\
	src/common/classes/UserBlob.cpp\
	src/common/classes/SafeArg.cpp\
	src/common/classes/MsgPrint.cpp\
	src/common/classes/BaseStream.cpp\
	src/common/fb_exception.cpp\
	src/common/thd.cpp\
	src/common/classes/MetaName.cpp\
	src/common/StatusHolder.cpp\
	src/common/classes/init.cpp\
	src/common/StatusArg.cpp\
	src/common/utils.cpp\
	src/config/AdminException.cpp\
	src/config/Args.cpp\
	src/config/ArgsException.cpp\
	src/config/ConfObj.cpp\
	src/config/ConfObject.cpp\
	src/config/ConfigFile.cpp\
	src/config/Configuration.cpp\
	src/config/Element.cpp\
	src/config/FileName.cpp\
	src/config/InputFile.cpp\
	src/config/InputStream.cpp\
	src/config/Lex.cpp\
	src/config/ScanDir.cpp\
	src/config/Stream.cpp\
	src/config/StreamSegment.cpp\
	src/vulcan/PathName.cpp\
	src/vulcan/RefObject.cpp
LOCAL_STATIC_LIBRARIES:= \
	libncurses\
	libicuuc\
	libicudata\
	libicui18n
LOCAL_MODULE := libfbembed

include $(BUILD_SHARED_LIBRARY)


#original path: gen/firebird/lib/libfbclient.so.2.5.2
include $(CLEAR_VARS)

LOCAL_CFLAGS:= -DBOOT_BUILD -DNAMESPACE=Vulcan -DNDEBUG -DLINUX -DARM \
-fPIC -fsigned-char -fmessage-length=0 -ffunction-sections -fdata-sections
LOCAL_CPPFLAGS:= -DSUPERCLIENT -fexceptions -ffunction-sections -fdata-sections

LOCAL_C_INCLUDES:= \
	icu/androidbuild/icu_build/include\
	libtommath\
	ncurses/include\
	firebird/src/include/gen\
	firebird/src/include\
	firebird/src/vulcan
LOCAL_SRC_FILES:= \
	src/jrd/alt.cpp\
	src/jrd/db_alias.cpp\
	src/jrd/dsc.cpp\
	src/jrd/enc.cpp\
	src/jrd/gds.cpp\
	src/jrd/isc.cpp\
	src/jrd/isc_file.cpp\
	src/jrd/os/posix/isc_ipc.cpp\
	src/jrd/perf.cpp\
	src/jrd/sdl.cpp\
	src/jrd/status.cpp\
	src/jrd/ThreadData.cpp\
	src/jrd/ThreadStart.cpp\
	src/jrd/utl.cpp\
	src/jrd/why.cpp\
	src/common/cvt.cpp\
	temp/superclient/dsql/array.cpp\
	temp/superclient/dsql/blob.cpp\
	src/dsql/preparse.cpp\
	src/dsql/user_dsql.cpp\
	src/dsql/utld.cpp\
	src/dsql/keywords.cpp\
	src/remote/interface.cpp\
	src/remote/inet.cpp\
	src/remote/merge.cpp\
	src/remote/parser.cpp\
	src/remote/protocol.cpp\
	src/remote/remote.cpp\
	src/remote/xdr.cpp\
	src/gpre/pretty.cpp\
	src/utilities/gsec/call_service.cpp\
	src/common/fb_exception.cpp\
	src/common/thd.cpp\
	src/common/classes/MetaName.cpp\
	src/common/StatusHolder.cpp\
	src/common/classes/init.cpp\
	src/common/StatusArg.cpp\
	src/common/utils.cpp\
	src/common/classes/alloc.cpp\
	src/common/classes/locks.cpp\
	src/common/classes/semaphore.cpp\
	src/common/classes/fb_string.cpp\
	src/common/classes/timestamp.cpp\
	src/common/classes/PublicHandle.cpp\
	src/common/classes/TempFile.cpp\
	src/common/classes/SafeArg.cpp\
	src/common/classes/MsgPrint.cpp\
	src/common/classes/BaseStream.cpp\
	src/jrd/os/posix/config_root.cpp\
	src/jrd/os/posix/path_utils.cpp\
	src/jrd/os/posix/mod_loader.cpp\
	src/jrd/os/posix/fbsyslog.cpp\
	src/jrd/os/posix/guid.cpp\
	src/jrd/os/posix/os_utils.cpp\
	src/common/dllinst.cpp\
	extern/binreloc/binreloc.c\
	src/common/config/config.cpp\
	src/common/config/config_file.cpp\
	src/common/config/dir_list.cpp\
	src/common/classes/ClumpletReader.cpp\
	src/common/classes/ClumpletWriter.cpp
LOCAL_STATIC_LIBRARIES:= \
	libncurses
LOCAL_MODULE := libfbclient

# include $(BUILD_SHARED_LIBRARY)
include $(BUILD_STATIC_LIBRARY)

#original path: gen/firebird/UDF/fbudf.so
include $(CLEAR_VARS)

LOCAL_CPPFLAGS:= -DSUPERCLIENT -DNAMESPACE=Vulcan -DNDEBUG -DLINUX -DARM \
-fPIC -fsigned-char -fmessage-length=0 -ffunction-sections -fdata-sections

LOCAL_C_INCLUDES:= \
	icu/androidbuild/icu_build/include\
	libtommath\
	ncurses/include\
	firebird/src/include/gen\
	firebird/src/include\
	firebird/src/vulcan
LOCAL_SRC_FILES:= \
	src/extlib/fbudf/fbudf.cpp\
	src/common/classes/timestamp.cpp
LOCAL_STATIC_LIBRARIES:= \
	libib_util
LOCAL_MODULE := fbudf

include $(BUILD_SHARED_LIBRARY)


#original path: gen/firebird/plugins/libfbtrace.so
include $(CLEAR_VARS)

LOCAL_CFLAGS:= -DBOOT_BUILD -DNAMESPACE=Vulcan -DNDEBUG -DLINUX -DARM \
-fPIC -fsigned-char -fmessage-length=0 -ffunction-sections -fdata-sections
LOCAL_CPPFLAGS:= -DSUPERCLIENT 

LOCAL_C_INCLUDES:= \
	icu/androidbuild/icu_build/include\
	libtommath\
	ncurses/include\
	firebird/src/include/gen\
	firebird/src/include\
	firebird/src/vulcan
LOCAL_SRC_FILES:= \
	src/utilities/ntrace/TraceConfiguration.cpp\
	src/utilities/ntrace/traceplugin.cpp\
	src/utilities/ntrace/TracePluginImpl.cpp\
	src/utilities/ntrace/TraceUnicodeUtils.cpp\
	src/utilities/ntrace/PluginLogWriter.cpp\
	src/utilities/ntrace/os/posix/platform.cpp\
	src/jrd/os/posix/config_root.cpp\
	src/jrd/os/posix/path_utils.cpp\
	src/jrd/os/posix/mod_loader.cpp\
	src/jrd/os/posix/fbsyslog.cpp\
	src/jrd/os/posix/guid.cpp\
	src/jrd/os/posix/os_utils.cpp\
	src/common/dllinst.cpp\
	extern/binreloc/binreloc.c\
	src/jrd/isc.cpp\
	src/jrd/isc_file.cpp\
	src/jrd/isc_sync.cpp\
	src/jrd/CharSet.cpp\
	src/jrd/TextType.cpp\
	src/jrd/IntlUtil.cpp\
	src/jrd/unicode_util.cpp\
	src/common/classes/ClumpletReader.cpp\
	src/common/utils.cpp\
	src/config/AdminException.cpp\
	src/config/Args.cpp\
	src/config/ArgsException.cpp\
	src/config/ConfObj.cpp\
	src/config/ConfObject.cpp\
	src/config/ConfigFile.cpp\
	src/config/Configuration.cpp\
	src/config/Element.cpp\
	src/config/FileName.cpp\
	src/config/InputFile.cpp\
	src/config/InputStream.cpp\
	src/config/Lex.cpp\
	src/config/ScanDir.cpp\
	src/config/Stream.cpp\
	src/config/StreamSegment.cpp\
	src/vulcan/PathName.cpp\
	src/vulcan/RefObject.cpp\
	src/common/fb_exception.cpp\
	src/common/thd.cpp\
	src/common/classes/MetaName.cpp\
	src/common/StatusHolder.cpp\
	src/common/classes/init.cpp\
	src/common/StatusArg.cpp\
	src/common/classes/SafeArg.cpp\
	src/common/classes/MsgPrint.cpp\
	src/common/classes/BaseStream.cpp\
	src/common/classes/alloc.cpp\
	src/common/classes/locks.cpp\
	src/common/classes/semaphore.cpp\
	src/common/classes/fb_string.cpp\
	src/common/classes/timestamp.cpp\
	src/common/classes/PublicHandle.cpp\
	src/common/classes/TempFile.cpp\
	src/common/config/config.cpp\
	src/common/config/config_file.cpp\
	src/common/config/dir_list.cpp
LOCAL_STATIC_LIBRARIES:= \
	libncurses\
	libfbembed
LOCAL_MODULE := libfbtrace

include $(BUILD_SHARED_LIBRARY)

