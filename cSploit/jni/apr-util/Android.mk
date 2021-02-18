LOCAL_PATH := $(call my-dir)

#original path: libaprutil-1.a
#WARNING: apr_password_validate and apr_bcrypt_encode is missing
include $(CLEAR_VARS)

LOCAL_CFLAGS:= -DLINUX -D_REENTRANT -D_GNU_SOURCE -D_FORTIFY_SOURCE=2 -DOPENSSL_NO_DEPRECATED

# fixed flags
LOCAL_CFLAGS+= -ffunction-sections -fdata-sections

LOCAL_C_INCLUDES:= \
	$(LOCAL_PATH)/include\
	$(LOCAL_PATH)/include/private\
	$(LOCAL_PATH)/crypto/glibc_crypt\
	external/postgresql/src/interfaces/libpq\
	external/postgresql/src/include\
	external/libmysqlclient/include\
	external/sqlite/dist\
	external/freetds/include\
	external/freetds/generated/android/include\
	external/apr/include\
	external/openssl/include\
	external/bdb/build_android\
	external/libexpat/lib\
	external/libiconv/include\
	external/unixODBC/include
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
	crypto/apr_crypto.c\
	crypto/apr_md4.c\
	crypto/apr_md5.c\
	crypto/apr_sha1.c\
	crypto/crypt_blowfish.c\
	crypto/getuuid.c\
	crypto/uuid.c\
	dbd/apr_dbd.c\
	dbd/apr_dbd_freetds.c\
	dbd/apr_dbd_mysql.c\
	dbd/apr_dbd_odbc.c\
	dbd/apr_dbd_pgsql.c\
	dbd/apr_dbd_sqlite3.c\
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
LOCAL_STATIC_LIBRARIES:= \
	libiconv\
	libapr-1\
	libpq\
	libodbc\
	libltdl\
	libmysqlclient\
	libsybdb
LOCAL_SHARED_LIBRARIES:= \
	libssl\
	libcrypto\
	libexpat\
	libsqlite
LOCAL_MODULE := libaprutil-1

include $(BUILD_STATIC_LIBRARY)
