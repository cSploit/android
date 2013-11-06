LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES:= 	passwd/apr_getpass.c \
					strings/apr_cpystrn.c \
					strings/apr_fnmatch.c \
					strings/apr_snprintf.c \
					strings/apr_strings.c \
					strings/apr_strnatcmp.c \
					strings/apr_strtok.c \
					tables/apr_hash.c \
					tables/apr_tables.c \
					atomic/unix/builtins.c \
					atomic/unix/ia32.c \
					atomic/unix/mutex.c \
					atomic/unix/ppc.c \
					atomic/unix/s390.c \
					atomic/unix/solaris.c \
					dso/unix/dso.c \
					file_io/unix/buffer.c \
					file_io/unix/copy.c \
					file_io/unix/dir.c \
					file_io/unix/fileacc.c \
					file_io/unix/filedup.c \
					file_io/unix/filepath.c \
					file_io/unix/filepath_util.c \
					file_io/unix/filestat.c \
					file_io/unix/flock.c \
					file_io/unix/fullrw.c \
					file_io/unix/mktemp.c \
					file_io/unix/open.c \
					file_io/unix/pipe.c \
					file_io/unix/readwrite.c \
					file_io/unix/seek.c \
					file_io/unix/tempdir.c \
					locks/unix/global_mutex.c \
					locks/unix/proc_mutex.c \
					locks/unix/thread_cond.c \
					locks/unix/thread_mutex.c \
					locks/unix/thread_rwlock.c \
					memory/unix/apr_pools.c \
					misc/unix/charset.c \
					misc/unix/env.c \
					misc/unix/errorcodes.c \
					misc/unix/getopt.c \
					misc/unix/otherchild.c \
					misc/unix/rand.c \
					misc/unix/start.c \
					misc/unix/version.c \
					mmap/unix/common.c \
					mmap/unix/mmap.c \
					network_io/unix/inet_ntop.c \
					network_io/unix/inet_pton.c \
					network_io/unix/multicast.c \
					network_io/unix/sendrecv.c \
					network_io/unix/sockaddr.c \
					network_io/unix/socket_util.c \
					network_io/unix/sockets.c \
					network_io/unix/sockopt.c \
					poll/unix/epoll.c \
					poll/unix/kqueue.c \
					poll/unix/poll.c \
					poll/unix/pollcb.c \
					poll/unix/pollset.c \
					poll/unix/port.c \
					poll/unix/select.c \
					random/unix/apr_random.c \
					random/unix/sha2.c \
					random/unix/sha2_glue.c \
					shmem/unix/shm.c \
					support/unix/waitio.c \
					threadproc/unix/proc.c \
					threadproc/unix/procsup.c \
					threadproc/unix/signals.c \
					threadproc/unix/thread.c \
					threadproc/unix/threadpriv.c \
					time/unix/time.c \
					time/unix/timestr.c \
					user/unix/groupinfo.c \
					user/unix/userinfo.c

LOCAL_C_INCLUDES:=	apr\
					apr/include/arch/unix/\
					apr/include/

LOCAL_CFLAGS:=-g -O2 -pthread -DHAVE_CONFIG_H -DLINUX -D_REENTRANT -D_GNU_SOURCE \
-ffunction-sections -fdata-sections

LOCAL_MODULE:= libapr-1

include $(BUILD_STATIC_LIBRARY)
