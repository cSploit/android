LOCAL_PATH := $(call my-dir)

#original path: lib/libncp.a
include $(CLEAR_VARS)

LOCAL_CFLAGS:= -DN_PLAT_LINUX \
-DLOCALEDIR=\"/opt/android-ndk/platforms/android-18/arch-arm/usr/share/locale\" \
-DNCPFS_VERSION=\"2.2.6\" -DNCPFS_PACKAGE=\"ncpfs\" -D_REENTRANT \
-ffunction-sections -fdata-sections \
-DNWSFIND=\"/opt/android-ndk/platforms/android-18/arch-arm/usr/bin/nwsfind\" \
-DHAVE_CONFIG_H -DMAKE_NCPLIB -D_GNU_SOURCE \
-DGLOBALCFGFILE=\"/opt/android-ndk/platforms/android-18/arch-arm/usr/etc/ncpfs.conf\" \
-DPORTABLE -DUPTON  -Wno-attributes

LOCAL_C_INCLUDES:= \
	ncpfs/include\
	ncpfs/intl\
	ncpfs/lib
LOCAL_SRC_FILES:= \
	lib/ncplib.c\
	lib/filemgmt.c\
	lib/queue.c\
	lib/nwcalls.c\
	lib/nwtime.c\
	lib/cfgfile.c\
	lib/fs/eas.c\
	lib/strops.c\
	lib/ncpext.c\
	lib/nwclient.c\
	lib/resolve.c\
	lib/fs/filelock.c\
	lib/stats.c\
	lib/ndscrypt.c\
	lib/nwnet.c\
	lib/wcs.c\
	lib/rdn.c\
	lib/ds/filter.c\
	lib/ds/search.c\
	lib/ds/request.c\
	lib/ds/setkeys.c\
	lib/ds/dsgetstat.c\
	lib/ds/partops.c\
	lib/ds/iterhandle.c\
	lib/ds/effright.c\
	lib/ds/dsread.c\
	lib/ds/dslist.c\
	lib/ds/bindctx.c\
	lib/ds/classes.c\
	lib/ds/syntaxes.c\
	lib/ds/dsstream.c\
	lib/o_ndslib.c\
	lib/ncpsign.c\
	lib/ndslib.c\
	lib/mpilib.c\
	util/getpass.c
LOCAL_MODULE := libncp

include $(BUILD_STATIC_LIBRARY)

