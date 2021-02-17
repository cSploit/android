LOCAL_PATH := $(call my-dir)

#original path: libraries/liblutil/liblutil.a
include $(CLEAR_VARS)

LOCAL_CFLAGS:= -DOPENSSL_NO_DEPRECATED

# fixed flags
LOCAL_CFLAGS+= -ffunction-sections -fdata-sections

LOCAL_C_INCLUDES:= \
	$(LOCAL_PATH)/include\
	$(LOCAL_PATH)/generated/bionic/include\
	external/openssl/include\
	external/icu4c/common
LOCAL_SRC_FILES:= \
	libraries/liblutil/base64.c\
	libraries/liblutil/entropy.c\
	libraries/liblutil/sasl.c\
	libraries/liblutil/signal.c\
	libraries/liblutil/hash.c\
	libraries/liblutil/passfile.c\
	libraries/liblutil/md5.c\
	libraries/liblutil/passwd.c\
	libraries/liblutil/sha1.c\
	libraries/liblutil/getpass.c\
	libraries/liblutil/lockf.c\
	libraries/liblutil/utils.c\
	libraries/liblutil/uuid.c\
	libraries/liblutil/sockpair.c\
	libraries/liblutil/avl.c\
	libraries/liblutil/tavl.c\
	libraries/liblutil/meter.c\
	libraries/liblutil/setproctitle.c\
	libraries/liblutil/memcmp.c\
	libraries/liblutil/getpeereid.c\
	libraries/liblutil/detach.c\
	generated/bionic/libraries/liblutil/version.c
LOCAL_MODULE := liblutil

include $(BUILD_STATIC_LIBRARY)


#original path: libraries/liblber/.libs/liblber.a
include $(CLEAR_VARS)

LOCAL_CFLAGS:= -DLBER_LIBRARY -DOPENSSL_NO_DEPRECATED

# fixed flags
LOCAL_CFLAGS+= -ffunction-sections -fdata-sections

LOCAL_C_INCLUDES:= \
	$(LOCAL_PATH)/include\
	$(LOCAL_PATH)/generated/bionic/include\
	external/openssl/include\
	external/icu4c/common
LOCAL_SRC_FILES:= \
	libraries/liblber/assert.c\
	libraries/liblber/decode.c\
	libraries/liblber/encode.c\
	libraries/liblber/io.c\
	libraries/liblber/bprint.c\
	libraries/liblber/debug.c\
	libraries/liblber/memory.c\
	libraries/liblber/options.c\
	libraries/liblber/sockbuf.c\
	libraries/liblber/stdio.c\
	generated/bionic/libraries/liblber/version.c
LOCAL_MODULE := liblber

include $(BUILD_STATIC_LIBRARY)


#original path: libraries/liblunicode/liblunicode.a
include $(CLEAR_VARS)

LOCAL_CFLAGS:= -DOPENSSL_NO_DEPRECATED

# fixed flags
LOCAL_CFLAGS+= -ffunction-sections -fdata-sections

LOCAL_C_INCLUDES:= \
	$(LOCAL_PATH)/include\
	$(LOCAL_PATH)/generated/bionic/include\
	$(LOCAL_PATH)/libraries/liblunicode/ucdata\
	external/openssl/include\
	external/icu4c/common
LOCAL_SRC_FILES:= \
	libraries/liblunicode/ucdata/ucdata.c\
	libraries/liblunicode/ure/ure.c\
	libraries/liblunicode/ure/urestubs.c\
	libraries/liblunicode/ucstr.c\
	generated/bionic/libraries/liblunicode/version.c
LOCAL_MODULE := liblunicode

include $(BUILD_STATIC_LIBRARY)


#original path: libraries/libldap/.libs/libldap.a
include $(CLEAR_VARS)

LOCAL_CFLAGS:= -DLDAP_LIBRARY -DOPENSSL_NO_DEPRECATED

# fixed flags
LOCAL_CFLAGS+= -ffunction-sections -fdata-sections

LOCAL_C_INCLUDES:= \
	$(LOCAL_PATH)/include\
	$(LOCAL_PATH)/generated/bionic/include\
	external/openssl/include\
	external/icu4c/common
LOCAL_SRC_FILES:= \
	libraries/libldap/bind.c\
	libraries/libldap/open.c\
	libraries/libldap/result.c\
	libraries/libldap/error.c\
	libraries/libldap/compare.c\
	libraries/libldap/search.c\
	libraries/libldap/controls.c\
	libraries/libldap/messages.c\
	libraries/libldap/references.c\
	libraries/libldap/extended.c\
	libraries/libldap/cyrus.c\
	libraries/libldap/modify.c\
	libraries/libldap/add.c\
	libraries/libldap/modrdn.c\
	libraries/libldap/delete.c\
	libraries/libldap/abandon.c\
	libraries/libldap/sasl.c\
	libraries/libldap/gssapi.c\
	libraries/libldap/sbind.c\
	libraries/libldap/unbind.c\
	libraries/libldap/cancel.c\
	libraries/libldap/filter.c\
	libraries/libldap/free.c\
	libraries/libldap/sort.c\
	libraries/libldap/passwd.c\
	libraries/libldap/whoami.c\
	libraries/libldap/vc.c\
	libraries/libldap/getdn.c\
	libraries/libldap/getentry.c\
	libraries/libldap/getattr.c\
	libraries/libldap/getvalues.c\
	libraries/libldap/addentry.c\
	libraries/libldap/request.c\
	libraries/libldap/os-ip.c\
	libraries/libldap/url.c\
	libraries/libldap/pagectrl.c\
	libraries/libldap/sortctrl.c\
	libraries/libldap/vlvctrl.c\
	libraries/libldap/init.c\
	libraries/libldap/options.c\
	libraries/libldap/print.c\
	libraries/libldap/string.c\
	libraries/libldap/util-int.c\
	libraries/libldap/schema.c\
	libraries/libldap/charray.c\
	libraries/libldap/os-local.c\
	libraries/libldap/dnssrv.c\
	libraries/libldap/utf-8.c\
	libraries/libldap/utf-8-conv.c\
	libraries/libldap/tls2.c\
	libraries/libldap/tls_o.c\
	libraries/libldap/tls_g.c\
	libraries/libldap/tls_m.c\
	libraries/libldap/turn.c\
	libraries/libldap/ppolicy.c\
	libraries/libldap/dds.c\
	libraries/libldap/txn.c\
	libraries/libldap/ldap_sync.c\
	libraries/libldap/stctrl.c\
	libraries/libldap/assertion.c\
	libraries/libldap/deref.c\
	libraries/libldap/ldifutil.c\
	libraries/libldap/ldif.c\
	libraries/libldap/fetch.c\
	generated/bionic/libraries/libldap/version.c
LOCAL_MODULE := libldap

include $(BUILD_STATIC_LIBRARY)


#original path: libraries/libldap_r/.libs/libldap_r.a
include $(CLEAR_VARS)

LOCAL_CFLAGS:= -DLDAP_R_COMPILE -DLDAP_LIBRARY -DOPENSSL_NO_DEPRECATED

# fixed flags
LOCAL_CFLAGS+= -ffunction-sections -fdata-sections

LOCAL_C_INCLUDES:= \
	$(LOCAL_PATH)/include\
	$(LOCAL_PATH)/generated/bionic/include\
	$(LOCAL_PATH)/libraries/libldap\
	external/openssl/include\
	external/icu4c/common
LOCAL_SRC_FILES:= \
	generated/bionic/libraries/libldap_r/version.c\
	libraries/libldap_r/rdwr.c\
	libraries/libldap_r/rmutex.c\
	libraries/libldap_r/rq.c\
	libraries/libldap_r/thr_cthreads.c\
	libraries/libldap_r/thr_debug.c\
	libraries/libldap_r/threads.c\
	libraries/libldap_r/thr_nt.c\
	libraries/libldap_r/thr_posix.c\
	libraries/libldap_r/thr_pth.c\
	libraries/libldap_r/thr_stub.c\
	libraries/libldap_r/thr_thr.c\
	libraries/libldap_r/tpool.c\
	libraries/libldap/bind.c\
	libraries/libldap/open.c\
	libraries/libldap/result.c\
	libraries/libldap/error.c\
	libraries/libldap/compare.c\
	libraries/libldap/search.c\
	libraries/libldap/controls.c\
	libraries/libldap/messages.c\
	libraries/libldap/references.c\
	libraries/libldap/extended.c\
	libraries/libldap/cyrus.c\
	libraries/libldap/modify.c\
	libraries/libldap/add.c\
	libraries/libldap/modrdn.c\
	libraries/libldap/delete.c\
	libraries/libldap/abandon.c\
	libraries/libldap/sasl.c\
	libraries/libldap/gssapi.c\
	libraries/libldap/sbind.c\
	libraries/libldap/unbind.c\
	libraries/libldap/cancel.c\
	libraries/libldap/filter.c\
	libraries/libldap/free.c\
	libraries/libldap/sort.c\
	libraries/libldap/passwd.c\
	libraries/libldap/whoami.c\
	libraries/libldap/vc.c\
	libraries/libldap/getdn.c\
	libraries/libldap/getentry.c\
	libraries/libldap/getattr.c\
	libraries/libldap/getvalues.c\
	libraries/libldap/addentry.c\
	libraries/libldap/request.c\
	libraries/libldap/os-ip.c\
	libraries/libldap/url.c\
	libraries/libldap/pagectrl.c\
	libraries/libldap/sortctrl.c\
	libraries/libldap/vlvctrl.c\
	libraries/libldap/init.c\
	libraries/libldap/options.c\
	libraries/libldap/print.c\
	libraries/libldap/string.c\
	libraries/libldap/util-int.c\
	libraries/libldap/schema.c\
	libraries/libldap/charray.c\
	libraries/libldap/os-local.c\
	libraries/libldap/dnssrv.c\
	libraries/libldap/utf-8.c\
	libraries/libldap/utf-8-conv.c\
	libraries/libldap/tls2.c\
	libraries/libldap/tls_o.c\
	libraries/libldap/tls_g.c\
	libraries/libldap/tls_m.c\
	libraries/libldap/turn.c\
	libraries/libldap/ppolicy.c\
	libraries/libldap/dds.c\
	libraries/libldap/txn.c\
	libraries/libldap/ldap_sync.c\
	libraries/libldap/stctrl.c\
	libraries/libldap/assertion.c\
	libraries/libldap/deref.c\
	libraries/libldap/ldifutil.c\
	libraries/libldap/ldif.c\
	libraries/libldap/fetch.c
LOCAL_MODULE := libldap_r

include $(BUILD_STATIC_LIBRARY)


#original path: libraries/librewrite/librewrite.a
include $(CLEAR_VARS)

LOCAL_CFLAGS:= -DOPENSSL_NO_DEPRECATED

# fixed flags
LOCAL_CFLAGS+= -ffunction-sections -fdata-sections

LOCAL_C_INCLUDES:= \
	$(LOCAL_PATH)/include\
	$(LOCAL_PATH)/generated/bionic/include\
	external/openssl/include\
	external/icu4c/common
LOCAL_SRC_FILES:= \
	libraries/librewrite/config.c\
	libraries/librewrite/context.c\
	libraries/librewrite/info.c\
	libraries/librewrite/ldapmap.c\
	libraries/librewrite/map.c\
	libraries/librewrite/params.c\
	libraries/librewrite/rule.c\
	libraries/librewrite/session.c\
	libraries/librewrite/subst.c\
	libraries/librewrite/var.c\
	libraries/librewrite/xmap.c\
	generated/bionic/libraries/librewrite/version.c
LOCAL_MODULE := librewrite

include $(BUILD_STATIC_LIBRARY)

