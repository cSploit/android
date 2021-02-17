# DO NOT EDIT: automatically built by dist/s_android.
# Makefile for building a drop-in replacement of SQLite using
# Berkeley DB 12c Release 1, library version 12.1.6.0.30: (January 23, 2014)
###################################################################
LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE := libdb-6.0

BDB_TOP := $(realpath $(LOCAL_PATH)/../)
BDB_PATH := $(BDB_TOP)/src

# This directive results in arm (vs thumb) code.  It's necessary to
# allow some BDB assembler code (for mutexes) to compile.
LOCAL_ARM_MODE := arm

# basic includes for BDB 11gR2
LOCAL_C_INCLUDES := \
	$(BDB_PATH)\
	$(LOCAL_PATH)\
	$(BDB_TOP)/src

# sSource files
LOCAL_SRC_FILES := \
	$(BDB_TOP)/src/blob/blob_fileops.c \
	$(BDB_TOP)/src/blob/blob_page.c \
	$(BDB_TOP)/src/blob/blob_stream.c \
	$(BDB_TOP)/src/blob/blob_util.c \
	$(BDB_TOP)/src/btree/bt_compact.c \
	$(BDB_TOP)/src/btree/bt_compare.c \
	$(BDB_TOP)/src/btree/bt_compress.c \
	$(BDB_TOP)/src/btree/bt_conv.c \
	$(BDB_TOP)/src/btree/bt_curadj.c \
	$(BDB_TOP)/src/btree/bt_cursor.c \
	$(BDB_TOP)/src/btree/bt_delete.c \
	$(BDB_TOP)/src/btree/bt_method.c \
	$(BDB_TOP)/src/btree/bt_open.c \
	$(BDB_TOP)/src/btree/bt_put.c \
	$(BDB_TOP)/src/btree/bt_rec.c \
	$(BDB_TOP)/src/btree/bt_reclaim.c \
	$(BDB_TOP)/src/btree/bt_recno.c \
	$(BDB_TOP)/src/btree/bt_rsearch.c \
	$(BDB_TOP)/src/btree/bt_search.c \
	$(BDB_TOP)/src/btree/bt_split.c \
	$(BDB_TOP)/src/btree/bt_stat.c \
	$(BDB_TOP)/src/btree/bt_upgrade.c \
	$(BDB_TOP)/src/btree/btree_auto.c \
	$(BDB_TOP)/src/clib/rand.c \
	$(BDB_TOP)/src/clib/snprintf.c \
	$(BDB_TOP)/src/common/clock.c \
	$(BDB_TOP)/src/common/db_byteorder.c \
	$(BDB_TOP)/src/common/db_compint.c \
	$(BDB_TOP)/src/common/db_err.c \
	$(BDB_TOP)/src/common/db_getlong.c \
	$(BDB_TOP)/src/common/db_idspace.c \
	$(BDB_TOP)/src/common/db_log2.c \
	$(BDB_TOP)/src/common/db_shash.c \
	$(BDB_TOP)/src/common/dbt.c \
	$(BDB_TOP)/src/common/mkpath.c \
	$(BDB_TOP)/src/common/os_method.c \
	$(BDB_TOP)/src/common/zerofill.c \
	$(BDB_TOP)/src/db/crdel_auto.c \
	$(BDB_TOP)/src/db/crdel_rec.c \
	$(BDB_TOP)/src/db/db.c \
	$(BDB_TOP)/src/db/db_am.c \
	$(BDB_TOP)/src/db/db_auto.c \
	$(BDB_TOP)/src/db/db_backup.c \
	$(BDB_TOP)/src/db/db_cam.c \
	$(BDB_TOP)/src/db/db_cds.c \
	$(BDB_TOP)/src/db/db_compact.c \
	$(BDB_TOP)/src/db/db_conv.c \
	$(BDB_TOP)/src/db/db_copy.c \
	$(BDB_TOP)/src/db/db_dispatch.c \
	$(BDB_TOP)/src/db/db_dup.c \
	$(BDB_TOP)/src/db/db_iface.c \
	$(BDB_TOP)/src/db/db_join.c \
	$(BDB_TOP)/src/db/db_meta.c \
	$(BDB_TOP)/src/db/db_method.c \
	$(BDB_TOP)/src/db/db_open.c \
	$(BDB_TOP)/src/db/db_overflow.c \
	$(BDB_TOP)/src/db/db_pr.c \
	$(BDB_TOP)/src/db/db_rec.c \
	$(BDB_TOP)/src/db/db_reclaim.c \
	$(BDB_TOP)/src/db/db_remove.c \
	$(BDB_TOP)/src/db/db_rename.c \
	$(BDB_TOP)/src/db/db_ret.c \
	$(BDB_TOP)/src/db/db_setid.c \
	$(BDB_TOP)/src/db/db_setlsn.c \
	$(BDB_TOP)/src/db/db_sort_multiple.c \
	$(BDB_TOP)/src/db/db_stati.c \
	$(BDB_TOP)/src/db/db_truncate.c \
	$(BDB_TOP)/src/db/db_upg.c \
	$(BDB_TOP)/src/db/db_upg_opd.c \
	$(BDB_TOP)/src/db/db_vrfy_stub.c \
	$(BDB_TOP)/src/db/partition.c \
	$(BDB_TOP)/src/dbreg/dbreg.c \
	$(BDB_TOP)/src/dbreg/dbreg_auto.c \
	$(BDB_TOP)/src/dbreg/dbreg_rec.c \
	$(BDB_TOP)/src/dbreg/dbreg_stat.c \
	$(BDB_TOP)/src/dbreg/dbreg_util.c \
	$(BDB_TOP)/src/env/env_alloc.c \
	$(BDB_TOP)/src/env/env_backup.c \
	$(BDB_TOP)/src/env/env_config.c \
	$(BDB_TOP)/src/env/env_failchk.c \
	$(BDB_TOP)/src/env/env_file.c \
	$(BDB_TOP)/src/env/env_globals.c \
	$(BDB_TOP)/src/env/env_method.c \
	$(BDB_TOP)/src/env/env_name.c \
	$(BDB_TOP)/src/env/env_open.c \
	$(BDB_TOP)/src/env/env_recover.c \
	$(BDB_TOP)/src/env/env_region.c \
	$(BDB_TOP)/src/env/env_register.c \
	$(BDB_TOP)/src/env/env_sig.c \
	$(BDB_TOP)/src/env/env_stat.c \
	$(BDB_TOP)/src/fileops/fileops_auto.c \
	$(BDB_TOP)/src/fileops/fop_basic.c \
	$(BDB_TOP)/src/fileops/fop_rec.c \
	$(BDB_TOP)/src/fileops/fop_util.c \
	$(BDB_TOP)/src/hash/hash_func.c \
	$(BDB_TOP)/src/hash/hash_stub.c \
	$(BDB_TOP)/src/heap/heap_stub.c \
	$(BDB_TOP)/src/hmac/hmac.c \
	$(BDB_TOP)/src/hmac/sha1.c \
	$(BDB_TOP)/src/lock/lock.c \
	$(BDB_TOP)/src/lock/lock_deadlock.c \
	$(BDB_TOP)/src/lock/lock_failchk.c \
	$(BDB_TOP)/src/lock/lock_id.c \
	$(BDB_TOP)/src/lock/lock_list.c \
	$(BDB_TOP)/src/lock/lock_method.c \
	$(BDB_TOP)/src/lock/lock_region.c \
	$(BDB_TOP)/src/lock/lock_stat.c \
	$(BDB_TOP)/src/lock/lock_timer.c \
	$(BDB_TOP)/src/lock/lock_util.c \
	$(BDB_TOP)/src/log/log.c \
	$(BDB_TOP)/src/log/log_archive.c \
	$(BDB_TOP)/src/log/log_compare.c \
	$(BDB_TOP)/src/log/log_debug.c \
	$(BDB_TOP)/src/log/log_get.c \
	$(BDB_TOP)/src/log/log_method.c \
	$(BDB_TOP)/src/log/log_print.c \
	$(BDB_TOP)/src/log/log_put.c \
	$(BDB_TOP)/src/log/log_stat.c \
	$(BDB_TOP)/src/log/log_verify_stub.c \
	$(BDB_TOP)/src/mp/mp_alloc.c \
	$(BDB_TOP)/src/mp/mp_backup.c \
	$(BDB_TOP)/src/mp/mp_bh.c \
	$(BDB_TOP)/src/mp/mp_fget.c \
	$(BDB_TOP)/src/mp/mp_fmethod.c \
	$(BDB_TOP)/src/mp/mp_fopen.c \
	$(BDB_TOP)/src/mp/mp_fput.c \
	$(BDB_TOP)/src/mp/mp_fset.c \
	$(BDB_TOP)/src/mp/mp_method.c \
	$(BDB_TOP)/src/mp/mp_mvcc.c \
	$(BDB_TOP)/src/mp/mp_region.c \
	$(BDB_TOP)/src/mp/mp_register.c \
	$(BDB_TOP)/src/mp/mp_resize.c \
	$(BDB_TOP)/src/mp/mp_stat.c \
	$(BDB_TOP)/src/mp/mp_sync.c \
	$(BDB_TOP)/src/mp/mp_trickle.c \
	$(BDB_TOP)/src/mutex/mut_alloc.c \
	$(BDB_TOP)/src/mutex/mut_failchk.c \
	$(BDB_TOP)/src/mutex/mut_method.c \
	$(BDB_TOP)/src/mutex/mut_region.c \
	$(BDB_TOP)/src/mutex/mut_stat.c \
	$(BDB_TOP)/src/mutex/mut_tas.c \
	$(BDB_TOP)/src/os/os_abort.c \
	$(BDB_TOP)/src/os/os_abs.c \
	$(BDB_TOP)/src/os/os_alloc.c \
	$(BDB_TOP)/src/os/os_clock.c \
	$(BDB_TOP)/src/os/os_config.c \
	$(BDB_TOP)/src/os/os_cpu.c \
	$(BDB_TOP)/src/os/os_ctime.c \
	$(BDB_TOP)/src/os/os_dir.c \
	$(BDB_TOP)/src/os/os_errno.c \
	$(BDB_TOP)/src/os/os_fid.c \
	$(BDB_TOP)/src/os/os_flock.c \
	$(BDB_TOP)/src/os/os_fsync.c \
	$(BDB_TOP)/src/os/os_getenv.c \
	$(BDB_TOP)/src/os/os_handle.c \
	$(BDB_TOP)/src/os/os_map.c \
	$(BDB_TOP)/src/os/os_mkdir.c \
	$(BDB_TOP)/src/os/os_open.c \
	$(BDB_TOP)/src/os/os_path.c \
	$(BDB_TOP)/src/os/os_pid.c \
	$(BDB_TOP)/src/os/os_rename.c \
	$(BDB_TOP)/src/os/os_rmdir.c \
	$(BDB_TOP)/src/os/os_root.c \
	$(BDB_TOP)/src/os/os_rpath.c \
	$(BDB_TOP)/src/os/os_rw.c \
	$(BDB_TOP)/src/os/os_seek.c \
	$(BDB_TOP)/src/os/os_stack.c \
	$(BDB_TOP)/src/os/os_stat.c \
	$(BDB_TOP)/src/os/os_tmpdir.c \
	$(BDB_TOP)/src/os/os_truncate.c \
	$(BDB_TOP)/src/os/os_uid.c \
	$(BDB_TOP)/src/os/os_unlink.c \
	$(BDB_TOP)/src/os/os_yield.c \
	$(BDB_TOP)/src/qam/qam_stub.c \
	$(BDB_TOP)/src/rep/rep_stub.c \
	$(BDB_TOP)/src/repmgr/repmgr_stub.c \
	$(BDB_TOP)/src/sequence/seq_stat.c \
	$(BDB_TOP)/src/sequence/sequence.c \
	$(BDB_TOP)/src/txn/txn.c \
	$(BDB_TOP)/src/txn/txn_auto.c \
	$(BDB_TOP)/src/txn/txn_chkpt.c \
	$(BDB_TOP)/src/txn/txn_failchk.c \
	$(BDB_TOP)/src/txn/txn_method.c \
	$(BDB_TOP)/src/txn/txn_rec.c \
	$(BDB_TOP)/src/txn/txn_recover.c \
	$(BDB_TOP)/src/txn/txn_region.c \
	$(BDB_TOP)/src/txn/txn_stat.c \
	$(BDB_TOP)/src/txn/txn_util.c \
	$(BDB_TOP)/src/common/crypto_stub.c

ifneq ($(TARGET_ARCH),arm)
LOCAL_LDLIBS += -lpthread -ldl
endif

#
# flags -- most of these are from the SQLite build, some are not.
# Here are some that may be changed for tuning or behavior:
# SQLITE_DEFAULT_JOURNAL_SIZE_LIMIT -- default size of BDB log file in bytes
# SQLITE_DEFAULT_PAGE_SIZE -- explicit control over page size for cache 
#  and databases
# SQLITE_DEFAULT_CACHE_SIZE -- sizes the BDB cache, in pages
# BDBSQL_SHARE_PRIVATE -- uses private environments but still shares databases
#  among processes using external synchronization.
# BDBSQL_CONVERT_SQLITE -- define this to convert SQLite databases to BDB SQL
#  format -- this has other requirements so do not do this without consulting
#  Oracle.
#
LOCAL_CFLAGS += -Wall -DHAVE_USLEEP=1 -DBDBSQL_SHARE_PRIVATE=1\
								-DNDEBUG=1 -Dfdatasync=fsync

# LOCAL_CFLAGS that are not used at this time
# -DSQLITE_ENABLE_POISON
# -DSQLITE_ENABLE_MEMORY_MANAGEMENT

ifneq ($(TARGET_SIMULATOR),true)
LOCAL_SHARED_LIBRARIES := libdl
endif

LOCAL_C_INCLUDES += $(call include-path-for, system-core)/cutils
LOCAL_SHARED_LIBRARIES += liblog libutils
LOCAL_STATIC_LIBRARIES := libicuuc libicui18n 

include $(BUILD_STATIC_LIBRARY)

