LOCAL_PATH := $(call my-dir)

#original path: libs/armeabi-v7a/libzlib.a
include $(CLEAR_VARS)

LOCAL_CFLAGS:= -DANDROID -DHAVE_CONFIG_H -DDBUG_OFF 

LOCAL_C_INCLUDES:= \
	mysql/androidbuild/include\
	mysql/include\
	mysql/zlib
LOCAL_SRC_FILES:= \
	zlib/adler32.c\
	zlib/compress.c\
	zlib/crc32.c\
	zlib/deflate.c\
	zlib/gzio.c\
	zlib/infback.c\
	zlib/inffast.c\
	zlib/inflate.c\
	zlib/inftrees.c\
	zlib/trees.c\
	zlib/uncompr.c\
	zlib/zutil.c
LOCAL_MODULE := libzlib

include $(BUILD_STATIC_LIBRARY)


#original path: libs/armeabi-v7a/libyassl.a
include $(CLEAR_VARS)

LOCAL_CFLAGS:= -DANDROID -DHAVE_CONFIG_H -Dget_tty_password=yassl_mysql_get_tty_password \
-Dget_tty_password_ext=yassl_mysql_get_tty_password_ext -DDBUG_OFF -DHAVE_YASSL \
-DYASSL_PREFIX -DHAVE_OPENSSL -DMULTI_THREADED 

LOCAL_C_INCLUDES:= \
	mysql/androidbuild/include\
	mysql/include\
	mysql/extra/yassl/include\
	mysql/extra/yassl/taocrypt/include\
	mysql/extra/yassl/taocrypt/mySTL
LOCAL_SRC_FILES:= \
	extra/yassl/src/buffer.cpp\
	extra/yassl/src/cert_wrapper.cpp\
	extra/yassl/src/crypto_wrapper.cpp\
	extra/yassl/src/handshake.cpp\
	extra/yassl/src/lock.cpp\
	extra/yassl/src/log.cpp\
	extra/yassl/src/socket_wrapper.cpp\
	extra/yassl/src/ssl.cpp\
	extra/yassl/src/timer.cpp\
	extra/yassl/src/yassl_error.cpp\
	extra/yassl/src/yassl_imp.cpp\
	extra/yassl/src/yassl_int.cpp\
	client/get_password.c
LOCAL_MODULE := libyassl

include $(BUILD_STATIC_LIBRARY)


#original path: libs/armeabi-v7a/libtaocrypt.a
include $(CLEAR_VARS)

LOCAL_CFLAGS:= -DANDROID -DHAVE_CONFIG_H -DDBUG_OFF -DHAVE_YASSL -DYASSL_PREFIX -DHAVE_OPENSSL \
-DMULTI_THREADED 

LOCAL_C_INCLUDES:= \
	mysql/androidbuild/include\
	mysql/extra/yassl/taocrypt/mySTL\
	mysql/extra/yassl/taocrypt/include\
	mysql/include
LOCAL_SRC_FILES:= \
	extra/yassl/taocrypt/src/aes.cpp\
	extra/yassl/taocrypt/src/aestables.cpp\
	extra/yassl/taocrypt/src/algebra.cpp\
	extra/yassl/taocrypt/src/arc4.cpp\
	extra/yassl/taocrypt/src/asn.cpp\
	extra/yassl/taocrypt/src/coding.cpp\
	extra/yassl/taocrypt/src/des.cpp\
	extra/yassl/taocrypt/src/dh.cpp\
	extra/yassl/taocrypt/src/dsa.cpp\
	extra/yassl/taocrypt/src/file.cpp\
	extra/yassl/taocrypt/src/hash.cpp\
	extra/yassl/taocrypt/src/integer.cpp\
	extra/yassl/taocrypt/src/md2.cpp\
	extra/yassl/taocrypt/src/md4.cpp\
	extra/yassl/taocrypt/src/md5.cpp\
	extra/yassl/taocrypt/src/misc.cpp\
	extra/yassl/taocrypt/src/random.cpp\
	extra/yassl/taocrypt/src/ripemd.cpp\
	extra/yassl/taocrypt/src/rsa.cpp\
	extra/yassl/taocrypt/src/sha.cpp\
	extra/yassl/taocrypt/src/rabbit.cpp\
	extra/yassl/taocrypt/src/hc128.cpp
LOCAL_MODULE := libtaocrypt

include $(BUILD_STATIC_LIBRARY)


#original path: libs/armeabi-v7a/libstrings.a
include $(CLEAR_VARS)

LOCAL_CFLAGS:= -DANDROID -DDISABLE_MYSQL_THREAD_H -DHAVE_CONFIG_H -DDBUG_OFF 

LOCAL_C_INCLUDES:= \
	mysql/androidbuild/include\
	mysql/include
LOCAL_SRC_FILES:= \
	strings/bchange.c\
	strings/ctype-big5.c\
	strings/ctype-bin.c\
	strings/ctype-cp932.c\
	strings/ctype-czech.c\
	strings/ctype-euc_kr.c\
	strings/ctype-eucjpms.c\
	strings/ctype-extra.c\
	strings/ctype-gb2312.c\
	strings/ctype-gbk.c\
	strings/ctype-latin1.c\
	strings/ctype-mb.c\
	strings/ctype-simple.c\
	strings/ctype-sjis.c\
	strings/ctype-tis620.c\
	strings/ctype-uca.c\
	strings/ctype-ucs2.c\
	strings/ctype-ujis.c\
	strings/ctype-utf8.c\
	strings/ctype-win1250ch.c\
	strings/ctype.c\
	strings/decimal.c\
	strings/dtoa.c\
	strings/int2str.c\
	strings/is_prefix.c\
	strings/llstr.c\
	strings/longlong2str.c\
	strings/my_strtoll10.c\
	strings/my_vsnprintf.c\
	strings/str2int.c\
	strings/str_alloc.c\
	strings/strcend.c\
	strings/strend.c\
	strings/strfill.c\
	strings/strmake.c\
	strings/strmov.c\
	strings/strnmov.c\
	strings/strxmov.c\
	strings/strxnmov.c\
	strings/xml.c\
	strings/my_strchr.c\
	strings/strcont.c\
	strings/strappend.c
LOCAL_MODULE := libstrings

include $(BUILD_STATIC_LIBRARY)


#original path: libs/armeabi-v7a/libmysys.a
include $(CLEAR_VARS)

LOCAL_CFLAGS:= -DANDROID -DHAVE_CONFIG_H -DDBUG_OFF 

LOCAL_C_INCLUDES:= \
	mysql/androidbuild/include\
	mysql/zlib\
	mysql/include\
	mysql/mysys
LOCAL_SRC_FILES:= \
	mysys/array.c\
	mysys/charset-def.c\
	mysys/charset.c\
	mysys/checksum.c\
	mysys/errors.c\
	mysys/hash.c\
	mysys/list.c\
	mysys/mf_cache.c\
	mysys/mf_dirname.c\
	mysys/mf_fn_ext.c\
	mysys/mf_format.c\
	mysys/mf_getdate.c\
	mysys/mf_iocache.c\
	mysys/mf_iocache2.c\
	mysys/mf_keycache.c\
	mysys/mf_keycaches.c\
	mysys/mf_loadpath.c\
	mysys/mf_pack.c\
	mysys/mf_path.c\
	mysys/mf_qsort.c\
	mysys/mf_qsort2.c\
	mysys/mf_radix.c\
	mysys/mf_same.c\
	mysys/mf_sort.c\
	mysys/mf_soundex.c\
	mysys/mf_arr_appstr.c\
	mysys/mf_tempdir.c\
	mysys/mf_tempfile.c\
	mysys/mf_unixpath.c\
	mysys/mf_wcomp.c\
	mysys/mulalloc.c\
	mysys/my_access.c\
	mysys/my_alloc.c\
	mysys/my_bit.c\
	mysys/my_bitmap.c\
	mysys/my_chsize.c\
	mysys/my_compress.c\
	mysys/my_copy.c\
	mysys/my_create.c\
	mysys/my_delete.c\
	mysys/my_div.c\
	mysys/my_error.c\
	mysys/my_file.c\
	mysys/my_fopen.c\
	mysys/my_fstream.c\
	mysys/my_gethwaddr.c\
	mysys/my_getsystime.c\
	mysys/my_getwd.c\
	mysys/my_compare.c\
	mysys/my_init.c\
	mysys/my_isnan.c\
	mysys/my_lib.c\
	mysys/my_lock.c\
	mysys/my_malloc.c\
	mysys/my_mess.c\
	mysys/my_mkdir.c\
	mysys/my_mmap.c\
	mysys/my_once.c\
	mysys/my_open.c\
	mysys/my_pread.c\
	mysys/my_read.c\
	mysys/my_redel.c\
	mysys/my_rename.c\
	mysys/my_seek.c\
	mysys/my_sleep.c\
	mysys/my_static.c\
	mysys/my_symlink.c\
	mysys/my_symlink2.c\
	mysys/my_sync.c\
	mysys/my_thr_init.c\
	mysys/my_write.c\
	mysys/ptr_cmp.c\
	mysys/queues.c\
	mysys/stacktrace.c\
	mysys/string.c\
	mysys/thr_alarm.c\
	mysys/thr_lock.c\
	mysys/thr_mutex.c\
	mysys/thr_rwlock.c\
	mysys/tree.c\
	mysys/typelib.c\
	mysys/base64.c\
	mysys/my_memmem.c\
	mysys/my_getpagesize.c\
	mysys/lf_alloc-pin.c\
	mysys/lf_dynarray.c\
	mysys/lf_hash.c\
	mysys/my_getncpus.c\
	mysys/my_rdtsc.c\
	mysys/psi_noop.c\
	mysys/my_syslog.c\
	mysys/my_alarm.c
LOCAL_STATIC_LIBRARIES:= libstrings\
libzlib\

LOCAL_MODULE := libmysys

include $(BUILD_STATIC_LIBRARY)


#original path: libs/armeabi-v7a/libdbug.a
include $(CLEAR_VARS)

LOCAL_CFLAGS:= -DANDROID -DHAVE_CONFIG_H -DDBUG_OFF 

LOCAL_C_INCLUDES:= \
	mysql/androidbuild/include\
	mysql/dbug\
	mysql/include
LOCAL_SRC_FILES:= \
	dbug/dbug.c
LOCAL_MODULE := libdbug

include $(BUILD_STATIC_LIBRARY)


#original path: libs/armeabi-v7a/libvio.a
include $(CLEAR_VARS)

LOCAL_CFLAGS:= -DANDROID -DHAVE_CONFIG_H -DDBUG_OFF -DHAVE_YASSL -DYASSL_PREFIX -DHAVE_OPENSSL \
-DMULTI_THREADED 

LOCAL_C_INCLUDES:= \
	mysql/androidbuild/include\
	mysql/include\
	mysql/extra/yassl/include\
	mysql/extra/yassl/taocrypt/include
LOCAL_SRC_FILES:= \
	vio/vio.c\
	vio/viosocket.c\
	vio/viossl.c\
	vio/viopipe.c\
	vio/vioshm.c\
	vio/viosslfactories.c
LOCAL_MODULE := libvio

include $(BUILD_STATIC_LIBRARY)


#original path: libs/armeabi-v7a/libregex.a
include $(CLEAR_VARS)

LOCAL_CFLAGS:= -DANDROID -DHAVE_CONFIG_H -DDBUG_OFF 

LOCAL_C_INCLUDES:= \
	mysql/androidbuild/include\
	mysql/include
LOCAL_SRC_FILES:= \
	regex/regcomp.c\
	regex/regerror.c\
	regex/regexec.c\
	regex/regfree.c\
	regex/reginit.c
LOCAL_MODULE := libregex

include $(BUILD_STATIC_LIBRARY)


#original path: libs/armeabi-v7a/libmysys_ssl.a
include $(CLEAR_VARS)

LOCAL_CFLAGS:= -DANDROID -DHAVE_CONFIG_H -DDBUG_OFF -DHAVE_YASSL -DYASSL_PREFIX -DHAVE_OPENSSL \
-DMULTI_THREADED 

LOCAL_C_INCLUDES:= \
	mysql/androidbuild/include\
	mysql/include\
	mysql/mysys_ssl\
	mysql/extra/yassl/include\
	mysql/extra/yassl/taocrypt/include
LOCAL_SRC_FILES:= \
	mysys_ssl/crypt_genhash_impl.cc\
	mysys_ssl/my_default.cc\
	mysys_ssl/my_getopt.cc\
	mysys_ssl/my_aes.cc\
	mysys_ssl/my_sha1.cc\
	mysys_ssl/my_sha2.cc\
	mysys_ssl/my_md5.cc\
	mysys_ssl/my_rnd.cc\
	mysys_ssl/my_murmur3.cc
LOCAL_STATIC_LIBRARIES:= libmysys\

LOCAL_MODULE := libmysys_ssl

include $(BUILD_STATIC_LIBRARY)


#original path: libs/armeabi-v7a/libclientlib.a
include $(CLEAR_VARS)

LOCAL_CPP_EXTENSION := .cc
LOCAL_CFLAGS:= -DANDROID -DHAVE_CONFIG_H -DDBUG_OFF -DHAVE_YASSL -DYASSL_PREFIX -DHAVE_OPENSSL \
-DMULTI_THREADED -D CLIENT_PROTOCOL_TRACING 
LOCAL_C_INCLUDES:= \
	mysql/androidbuild/include\
	mysql/include\
	mysql/libmysql\
	mysql/regex\
	mysql/sql\
	mysql/strings\
	mysql/extra/yassl/include\
	mysql/extra/yassl/taocrypt/include\
	mysql/extra/yassl/taocrypt/mySTL\
	mysql/zlib
LOCAL_SRC_FILES:= \
	libmysql/get_password.c\
	libmysql/libmysql.c\
	libmysql/errmsg.c\
	sql-common/client.c\
	sql-common/my_time.c\
	sql-common/client_plugin.c\
	sql-common/client_authentication.cc\
	sql/net_serv.cc\
	sql-common/pack.c\
	sql/auth/password.c\
	libmysql/mysql_trace.c
LOCAL_MODULE := libclientlib

include $(BUILD_STATIC_LIBRARY)


# #original path: libs/armeabi-v7a/libmysqlclient.so.18.1.0
# include $(CLEAR_VARS)
# 
# LOCAL_CFLAGS:= -DANDROID -DHAVE_CONFIG_H -Dlibmysql_EXPORTS -DDBUG_OFF -DHAVE_YASSL \
# -DYASSL_PREFIX -DHAVE_OPENSSL -DMULTI_THREADED -D CLIENT_PROTOCOL_TRACING 
# 
# LOCAL_C_INCLUDES:= \
# 	mysql/androidbuild/include\
# 	mysql/include\
# 	mysql/libmysql\
# 	mysql/regex\
# 	mysql/sql\
# 	mysql/strings\
# 	mysql/extra/yassl/include\
# 	mysql/extra/yassl/taocrypt/include\
# 	mysql/extra/yassl/taocrypt/mySTL\
# 	mysql/zlib
# LOCAL_SRC_FILES:= \
# 	androidbuild/libmysql/libmysql_exports_file.cc
# LOCAL_STATIC_LIBRARIES:= \
# 	libclientlib\
# 	libdbug\
# 	libstrings\
# 	libvio\
# 	libmysys\
# 	libmysys_ssl\
# 	libzlib\
# 	libyassl\
# 	libtaocrypt
# LOCAL_MODULE := libmysqlclient
# 
# include $(BUILD_SHARED_LIBRARY)


#original path: libs/armeabi-v7a/libmysqlclient.a
include $(CLEAR_VARS)

LOCAL_CFLAGS:= -DANDROID -DHAVE_CONFIG_H -DDBUG_OFF -DHAVE_YASSL -DYASSL_PREFIX -DHAVE_OPENSSL \
-DMULTI_THREADED -D CLIENT_PROTOCOL_TRACING 

LOCAL_C_INCLUDES:= \
	mysql/androidbuild/include\
	mysql/include\
	mysql/libmysql\
	mysql/regex\
	mysql/sql\
	mysql/strings\
	mysql/extra/yassl/include\
	mysql/extra/yassl/taocrypt/include\
	mysql/extra/yassl/taocrypt/mySTL\
	mysql/zlib
LOCAL_SRC_FILES:= \
	androidbuild/libmysql/mysqlclient_depends.c
LOCAL_STATIC_LIBRARIES:= \
	libclientlib\
	libdbug\
	libstrings\
	libvio\
	libmysys\
	libmysys_ssl\
	libzlib\
	libyassl\
	libtaocrypt
LOCAL_MODULE := libmysqlclient

include $(BUILD_STATIC_LIBRARY)

