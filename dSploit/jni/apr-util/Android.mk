LOCAL_PATH := $(call my-dir)

#original path: libaprutil-1.la
include $(CLEAR_VARS)

LOCAL_CFLAGS:= -DHAVE_CONFIG_H -DLINUX -D_REENTRANT -D_GNU_SOURCE -D_LIBC\
-ffunction-sections -fdata-sections

LOCAL_C_INCLUDES:= \
	apr-util/include\
	apr-util/include/private\
	apr-util/crypto/glibc_crypt\
	postgresql/include\
	mysql/include\
	sqlite3/include\
	apr/include\
	bdb/include\
	expat/lib\
	libiconv/include\
	unixODBC/include
LOCAL_SRC_FILES:= \
	buckets/apr_brigade.c\
	buckets/apr_buckets.c\
	buckets/apr_buckets_alloc.c\
	buckets/apr_buckets_eos.c\
	buckets/apr_buckets_file.c\
	buckets/apr_buckets_flush.c\
	buckets/apr_buckets_heap.c\
	buckets/apr_buckets_mmap.c\
	buckets/apr_buckets_pipe.c\
	buckets/apr_buckets_pool.c\
	buckets/apr_buckets_refcount.c\
	buckets/apr_buckets_simple.c\
	buckets/apr_buckets_socket.c\
	crypto/glibc_crypt/crypt-entry.c\
	crypto/glibc_crypt/md5-crypt.c\
	crypto/glibc_crypt/sha256-crypt.c\
	crypto/glibc_crypt/sha512-crypt.c\
	crypto/libc_crypt/crypt.c\
	crypto/apr_crypto.c\
	crypto/apr_md4.c\
	crypto/apr_md5.c\
	crypto/apr_passwd.c\
	crypto/apr_sha1.c\
	crypto/crypt_blowfish.c\
	crypto/getuuid.c\
	crypto/uuid.c\
	dbd/apr_dbd.c\
	dbm/apr_dbm.c\
	dbm/apr_dbm_sdbm.c\
	dbm/sdbm/sdbm.c\
	dbm/sdbm/sdbm_hash.c\
	dbm/sdbm/sdbm_lock.c\
	dbm/sdbm/sdbm_pair.c\
	encoding/apr_base64.c\
	hooks/apr_hooks.c\
	ldap/apr_ldap_stub.c\
	ldap/apr_ldap_url.c\
	memcache/apr_memcache.c\
	misc/apr_date.c\
	misc/apr_queue.c\
	misc/apr_reslist.c\
	misc/apr_rmm.c\
	misc/apr_thread_pool.c\
	misc/apu_dso.c\
	misc/apu_version.c\
	strmatch/apr_strmatch.c\
	uri/apr_uri.c\
	xlate/xlate.c\
	xml/apr_xml.c
LOCAL_STATIC_LIBRARIES:= libiconv
LOCAL_MODULE := libaprutil-1

include $(BUILD_STATIC_LIBRARY)


#original path: dbm/apr_dbm_db.la
include $(CLEAR_VARS)

LOCAL_CFLAGS:= -DHAVE_CONFIG_H -DLINUX -D_REENTRANT -D_GNU_SOURCE \
-ffunction-sections -fdata-sections

LOCAL_C_INCLUDES:= \
	apr-util/include\
	apr-util/include/private\
	postgresql/include\
	mysql/include\
	sqlite3/include\
	apr/include\
	bdb/include\
	expat/lib\
	libiconv/include\
	unixODBC/include
LOCAL_SRC_FILES:= \
	dbm/apr_dbm_berkeleydb.c
LOCAL_MODULE := apr_dbm_db

include $(BUILD_STATIC_LIBRARY)


#original path: dbd/apr_dbd_pgsql.la
include $(CLEAR_VARS)

LOCAL_CFLAGS:= -DHAVE_CONFIG_H -DLINUX -D_REENTRANT -D_GNU_SOURCE \
-ffunction-sections -fdata-sections

LOCAL_C_INCLUDES:= \
	apr-util/include\
	apr-util/include/private\
	postgresql/include\
	mysql/include\
	sqlite3/include\
	apr/include\
	bdb/include\
	expat/lib\
	libiconv/include\
	unixODBC/include
LOCAL_SRC_FILES:= \
	dbd/apr_dbd_pgsql.c
LOCAL_MODULE := apr_dbd_pgsql

include $(BUILD_STATIC_LIBRARY)


#original path: dbd/apr_dbd_sqlite3.la
include $(CLEAR_VARS)

LOCAL_CFLAGS:= -DHAVE_CONFIG_H -DLINUX -D_REENTRANT -D_GNU_SOURCE \
-ffunction-sections -fdata-sections

LOCAL_C_INCLUDES:= \
	apr-util/include\
	apr-util/include/private\
	postgresql/include\
	mysql/include\
	sqlite3/include\
	apr/include\
	bdb/include\
	expat/lib\
	libiconv/include\
	unixODBC/include
LOCAL_SRC_FILES:= \
	dbd/apr_dbd_sqlite3.c
LOCAL_MODULE := apr_dbd_sqlite3

include $(BUILD_STATIC_LIBRARY)


#original path: dbd/apr_dbd_mysql.la
include $(CLEAR_VARS)

LOCAL_CFLAGS:= -DHAVE_CONFIG_H -DLINUX -D_REENTRANT -D_GNU_SOURCE \
-ffunction-sections -fdata-sections

LOCAL_C_INCLUDES:= \
	apr-util/include\
	apr-util/include/private\
	postgresql/include\
	mysql/include\
	sqlite3/include\
	apr/include\
	bdb/include\
	expat/lib\
	libiconv/include\
	unixODBC/include
LOCAL_SRC_FILES:= \
	dbd/apr_dbd_mysql.c
LOCAL_MODULE := apr_dbd_mysql

include $(BUILD_STATIC_LIBRARY)


#original path: dbd/apr_dbd_odbc.la
include $(CLEAR_VARS)

LOCAL_CFLAGS:= -DHAVE_CONFIG_H -DLINUX -D_REENTRANT -D_GNU_SOURCE \
-ffunction-sections -fdata-sections

LOCAL_C_INCLUDES:= \
	apr-util/include\
	apr-util/include/private\
	postgresql/include\
	mysql/include\
	sqlite3/include\
	apr/include\
	bdb/include\
	expat/lib\
	libiconv/include\
	unixODBC/include
LOCAL_SRC_FILES:= \
	dbd/apr_dbd_odbc.c
LOCAL_MODULE := apr_dbd_odbc

include $(BUILD_STATIC_LIBRARY)

