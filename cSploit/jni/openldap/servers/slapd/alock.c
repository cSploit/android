/* alock.c - access lock library */
/* $OpenLDAP$ */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
 * Copyright 2005-2014 The OpenLDAP Foundation.
 * Portions Copyright 2004-2005 Symas Corporation.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted only as authorized by the OpenLDAP
 * Public License.
 *
 * A copy of this license is available in the file LICENSE in the
 * top-level directory of the distribution or, alternatively, at
 * <http://www.OpenLDAP.org/license.html>.
 */
/* ACKNOWLEDGEMENTS:
 * This work was initially developed by Emily Backes at Symas
 * Corporation for inclusion in OpenLDAP Software.
 */

#include "portable.h"

#if SLAPD_BDB || SLAPD_HDB

#include <lber.h>
#include "alock.h"
#include "lutil.h"

#include <ac/stdlib.h>
#include <ac/string.h>
#include <ac/unistd.h>
#include <ac/errno.h>
#include <ac/assert.h>
#include <sys/types.h>
#include <sys/stat.h>
#ifdef HAVE_SYS_FILE_H
#include <sys/file.h>
#endif
#include <fcntl.h>

#ifdef _WIN32
#include <windows.h>
#include <stdio.h>
#include <io.h>
#endif


static int
alock_grab_lock ( int fd, int slot )
{
	int res;
	
#if defined( HAVE_LOCKF )
	res = lseek (fd, (off_t) (ALOCK_SLOT_SIZE * slot), SEEK_SET);
	if (res == -1) return -1;
	res = lockf (fd, F_LOCK, (off_t) ALOCK_SLOT_SIZE);
#elif defined( HAVE_FCNTL )
	struct flock lock_info;
	(void) memset ((void *) &lock_info, 0, sizeof (struct flock));

	lock_info.l_type = F_WRLCK;
	lock_info.l_whence = SEEK_SET;
	lock_info.l_start = (off_t) (ALOCK_SLOT_SIZE * slot);
	lock_info.l_len = (off_t) ALOCK_SLOT_SIZE;

	res = fcntl (fd, F_SETLKW, &lock_info);
#elif defined( _WIN32 )
	OVERLAPPED ov;
	HANDLE hh = _get_osfhandle ( fd );
	ov.hEvent = 0;
	ov.Offset = ALOCK_SLOT_SIZE*slot;
	ov.OffsetHigh = 0;
	res = LockFileEx( hh, LOCKFILE_EXCLUSIVE_LOCK, 0,
		ALOCK_SLOT_SIZE, 0, &ov ) ? 0 : -1;
#else
#   error alock needs lockf, fcntl, or LockFile[Ex]
#endif
	if (res == -1) {
		assert (errno != EDEADLK);
		return -1;
	}
	return 0;
}

static int
alock_release_lock ( int fd, int slot )
{
	int res;
	
#if defined( HAVE_LOCKF )
	res = lseek (fd, (off_t) (ALOCK_SLOT_SIZE * slot), SEEK_SET);
	if (res == -1) return -1;
	res = lockf (fd, F_ULOCK, (off_t) ALOCK_SLOT_SIZE);
	if (res == -1) return -1;
#elif defined ( HAVE_FCNTL )
	struct flock lock_info;
	(void) memset ((void *) &lock_info, 0, sizeof (struct flock));

	lock_info.l_type = F_UNLCK;
	lock_info.l_whence = SEEK_SET;
	lock_info.l_start = (off_t) (ALOCK_SLOT_SIZE * slot);
	lock_info.l_len = (off_t) ALOCK_SLOT_SIZE;

	res = fcntl (fd, F_SETLKW, &lock_info);
	if (res == -1) return -1;
#elif defined( _WIN32 )
	HANDLE hh = _get_osfhandle ( fd );
	if ( !UnlockFile ( hh, ALOCK_SLOT_SIZE*slot, 0,
		ALOCK_SLOT_SIZE, 0 ))
		return -1;
#else
#   error alock needs lockf, fcntl, or LockFile[Ex]
#endif

	return 0;
}

static int
alock_share_lock ( int fd, int slot )
{
	int res;

#if defined( HAVE_LOCKF )
	res = 0;	/* lockf has no shared locks */
#elif defined ( HAVE_FCNTL )
	struct flock lock_info;
	(void) memset ((void *) &lock_info, 0, sizeof (struct flock));

	/* The shared lock replaces the existing lock */
	lock_info.l_type = F_RDLCK;
	lock_info.l_whence = SEEK_SET;
	lock_info.l_start = (off_t) (ALOCK_SLOT_SIZE * slot);
	lock_info.l_len = (off_t) ALOCK_SLOT_SIZE;

	res = fcntl (fd, F_SETLK, &lock_info);
	if (res == -1) return -1;
#elif defined( _WIN32 )
	OVERLAPPED ov;
	HANDLE hh = _get_osfhandle ( fd );

	/* Windows locks are mandatory, not advisory.
	 * We must downgrade the lock to allow future
	 * callers to read the slot data.
	 *
	 * First acquire a shared lock. Unlock will
	 * release the existing exclusive lock.
	 */
	ov.hEvent = 0;
	ov.Offset = ALOCK_SLOT_SIZE*slot;
	ov.OffsetHigh = 0;
	LockFileEx (hh, 0, 0, ALOCK_SLOT_SIZE, 0, &ov);
	UnlockFile (hh, ALOCK_SLOT_SIZE*slot, 0, ALOCK_SLOT_SIZE, 0);
#else
#   error alock needs lockf, fcntl, or LockFile[Ex]
#endif

	return 0;
}

static int
alock_test_lock ( int fd, int slot )
{
	int res;

#if defined( HAVE_LOCKF )
	res = lseek (fd, (off_t) (ALOCK_SLOT_SIZE * slot), SEEK_SET);
	if (res == -1) return -1;

	res = lockf (fd, F_TEST, (off_t) ALOCK_SLOT_SIZE);
	if (res == -1) {
		if (errno == EACCES || errno == EAGAIN) { 
			return ALOCK_LOCKED;
		} else {
			return -1;
		}
	}
#elif defined( HAVE_FCNTL )
	struct flock lock_info;
	(void) memset ((void *) &lock_info, 0, sizeof (struct flock));

	lock_info.l_type = F_WRLCK;
	lock_info.l_whence = SEEK_SET;
	lock_info.l_start = (off_t) (ALOCK_SLOT_SIZE * slot);
	lock_info.l_len = (off_t) ALOCK_SLOT_SIZE;

	res = fcntl (fd, F_GETLK, &lock_info);
	if (res == -1) return -1;

	if (lock_info.l_type != F_UNLCK) return ALOCK_LOCKED;
#elif defined( _WIN32 )
	OVERLAPPED ov;
	HANDLE hh = _get_osfhandle ( fd );
	ov.hEvent = 0;
	ov.Offset = ALOCK_SLOT_SIZE*slot;
	ov.OffsetHigh = 0;
	if( !LockFileEx( hh,
		LOCKFILE_EXCLUSIVE_LOCK|LOCKFILE_FAIL_IMMEDIATELY, 0,
		ALOCK_SLOT_SIZE, 0, &ov )) {
		int err = GetLastError();
		if ( err == ERROR_LOCK_VIOLATION )
			return ALOCK_LOCKED;
		else
			return -1;
	}
#else
#   error alock needs lockf, fcntl, or LockFile
#endif
	
	return 0;
}

/* Read a 64bit LE value */
static unsigned long int
alock_read_iattr ( unsigned char * bufptr )
{
	unsigned long int val = 0;
	int count;

	assert (bufptr != NULL);

	bufptr += sizeof (unsigned long int);
	for (count=0; count <= (int) sizeof (unsigned long int); ++count) {
		val <<= 8;
		val += (unsigned long int) *bufptr--;
	}

	return val;
}

/* Write a 64bit LE value */
static void
alock_write_iattr ( unsigned char * bufptr,
		    unsigned long int val )
{
	int count;

	assert (bufptr != NULL);

	for (count=0; count < 8; ++count) {
		*bufptr++ = (unsigned char) (val & 0xff);
		val >>= 8;
	}
}

static int
alock_read_slot ( alock_info_t * info,
		  alock_slot_t * slot_data )
{
	unsigned char slotbuf [ALOCK_SLOT_SIZE];
	int res, size, size_total, err;

	assert (info != NULL);
	assert (slot_data != NULL);
	assert (info->al_slot > 0);

	res = lseek (info->al_fd, 
		     (off_t) (ALOCK_SLOT_SIZE * info->al_slot), 
		     SEEK_SET);
	if (res == -1) return -1;

	size_total = 0;
	while (size_total < ALOCK_SLOT_SIZE) {
		size = read (info->al_fd, 
			     slotbuf + size_total, 
			     ALOCK_SLOT_SIZE - size_total);
		if (size == 0) return -1;
		if (size < 0) {
			err = errno;
			if (err != EINTR && err != EAGAIN) return -1;
		} else {
			size_total += size;
		}
	}
	
	if (alock_read_iattr (slotbuf) != ALOCK_MAGIC) {
		return -1;
	}
	slot_data->al_lock  = alock_read_iattr (slotbuf+8);
	slot_data->al_stamp = alock_read_iattr (slotbuf+16);
	slot_data->al_pid   = alock_read_iattr (slotbuf+24);

	if (slot_data->al_appname) ber_memfree (slot_data->al_appname);
	slot_data->al_appname = ber_memcalloc (1, ALOCK_MAX_APPNAME);
	if (slot_data->al_appname == NULL) {
		return -1;
	}
	strncpy (slot_data->al_appname, (char *)slotbuf+32, ALOCK_MAX_APPNAME-1);
	(slot_data->al_appname) [ALOCK_MAX_APPNAME-1] = '\0';

	return 0;
}

static int
alock_write_slot ( alock_info_t * info,
		   alock_slot_t * slot_data )
{
	unsigned char slotbuf [ALOCK_SLOT_SIZE];
	int res, size, size_total, err;

	assert (info != NULL);
	assert (slot_data != NULL);
	assert (info->al_slot > 0);

	(void) memset ((void *) slotbuf, 0, ALOCK_SLOT_SIZE);
	
	alock_write_iattr (slotbuf,    ALOCK_MAGIC);
	assert (alock_read_iattr (slotbuf) == ALOCK_MAGIC);
	alock_write_iattr (slotbuf+8,  slot_data->al_lock);
	alock_write_iattr (slotbuf+16, slot_data->al_stamp);
	alock_write_iattr (slotbuf+24, slot_data->al_pid);

	if (slot_data->al_appname)
		strncpy ((char *)slotbuf+32, slot_data->al_appname, ALOCK_MAX_APPNAME-1);
	slotbuf[ALOCK_SLOT_SIZE-1] = '\0';

	res = lseek (info->al_fd, 
		     (off_t) (ALOCK_SLOT_SIZE * info->al_slot),
		     SEEK_SET);
	if (res == -1) return -1;

	size_total = 0;
	while (size_total < ALOCK_SLOT_SIZE) {
		size = write (info->al_fd, 
			      slotbuf + size_total, 
			      ALOCK_SLOT_SIZE - size_total);
		if (size == 0) return -1;
		if (size < 0) {
			err = errno;
			if (err != EINTR && err != EAGAIN) return -1;
		} else {
			size_total += size;
		}
	}
	
	return 0;
}

static int
alock_query_slot ( alock_info_t * info )
{
	int res, nosave;
	alock_slot_t slot_data;

	assert (info != NULL);
	assert (info->al_slot > 0);
	
	(void) memset ((void *) &slot_data, 0, sizeof (alock_slot_t));
	alock_read_slot (info, &slot_data);

	if (slot_data.al_appname != NULL) ber_memfree (slot_data.al_appname);
	slot_data.al_appname = NULL;

	nosave = slot_data.al_lock & ALOCK_NOSAVE;

	if ((slot_data.al_lock & ALOCK_SMASK) == ALOCK_UNLOCKED)
		return slot_data.al_lock;

	res = alock_test_lock (info->al_fd, info->al_slot);
	if (res < 0) return -1;
	if (res > 0) {
		if ((slot_data.al_lock & ALOCK_SMASK) == ALOCK_UNIQUE) {
			return slot_data.al_lock;
		} else {
			return ALOCK_LOCKED | nosave;
		}
	}
	
	return ALOCK_DIRTY | nosave;
}

int 
alock_open ( alock_info_t * info,
	     const char * appname,
	     const char * envdir,
	     int locktype )
{
	struct stat statbuf;
	alock_info_t scan_info;
	alock_slot_t slot_data;
	char * filename;
	int res, max_slot;
	int dirty_count, live_count, nosave;
	char *ptr;

	assert (info != NULL);
	assert (appname != NULL);
	assert (envdir != NULL);
	assert ((locktype & ALOCK_SMASK) >= 1 && (locktype & ALOCK_SMASK) <= 2);

	slot_data.al_lock = locktype;
	slot_data.al_stamp = time(NULL);
	slot_data.al_pid = getpid();
	slot_data.al_appname = ber_memcalloc (1, ALOCK_MAX_APPNAME);
	if (slot_data.al_appname == NULL) {
		return ALOCK_UNSTABLE;
	}
	strncpy (slot_data.al_appname, appname, ALOCK_MAX_APPNAME-1);
	slot_data.al_appname [ALOCK_MAX_APPNAME-1] = '\0';

	filename = ber_memcalloc (1, strlen (envdir) + strlen ("/alock") + 1);
	if (filename == NULL ) {
		ber_memfree (slot_data.al_appname);
		return ALOCK_UNSTABLE;
	}
	ptr = lutil_strcopy(filename, envdir);
	lutil_strcopy(ptr, "/alock");
#ifdef _WIN32
	{ HANDLE handle = CreateFile (filename, GENERIC_READ|GENERIC_WRITE,
		FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, OPEN_ALWAYS,
		FILE_ATTRIBUTE_NORMAL, NULL);
		info->al_fd = _open_osfhandle (handle, 0);
	}
#else
	info->al_fd = open (filename, O_CREAT|O_RDWR, 0666);
#endif
	ber_memfree (filename);
	if (info->al_fd < 0) {
		ber_memfree (slot_data.al_appname);
		return ALOCK_UNSTABLE;
	}
	info->al_slot = 0;

	res = alock_grab_lock (info->al_fd, 0);
	if (res == -1) { 
		close (info->al_fd);
		ber_memfree (slot_data.al_appname);
		return ALOCK_UNSTABLE;
	}

	res = fstat (info->al_fd, &statbuf);
	if (res == -1) { 
		close (info->al_fd);
		ber_memfree (slot_data.al_appname);
		return ALOCK_UNSTABLE;
	}

	max_slot = (statbuf.st_size + ALOCK_SLOT_SIZE - 1) / ALOCK_SLOT_SIZE;
	dirty_count = 0;
	live_count = 0;
	nosave = 0;
	scan_info.al_fd = info->al_fd;
	for (scan_info.al_slot = 1; 
	     scan_info.al_slot < max_slot;
	     ++ scan_info.al_slot) {
		if (scan_info.al_slot != info->al_slot) {
			res = alock_query_slot (&scan_info);

			if (res & ALOCK_NOSAVE) {
				nosave = ALOCK_NOSAVE;
				res ^= ALOCK_NOSAVE;
			}
			if (res == ALOCK_UNLOCKED
			    && info->al_slot == 0) {
				info->al_slot = scan_info.al_slot;

			} else if (res == ALOCK_LOCKED) {
				++live_count;

			} else if (res == ALOCK_UNIQUE
				&& (( locktype & ALOCK_SMASK ) == ALOCK_UNIQUE
				|| nosave )) {
				close (info->al_fd);
				ber_memfree (slot_data.al_appname);
				return ALOCK_BUSY;

			} else if (res == ALOCK_DIRTY) {
				++dirty_count;

			} else if (res == -1) {
				close (info->al_fd);
				ber_memfree (slot_data.al_appname);
				return ALOCK_UNSTABLE;

			}
		}
	}

	if (dirty_count && live_count) {
		close (info->al_fd);
		ber_memfree (slot_data.al_appname);
		return ALOCK_UNSTABLE;
	}
	
	if (info->al_slot == 0) info->al_slot = max_slot + 1;
	res = alock_grab_lock (info->al_fd,
			       info->al_slot);
	if (res == -1) { 
		close (info->al_fd);
		ber_memfree (slot_data.al_appname);
		return ALOCK_UNSTABLE;
	}
	res = alock_write_slot (info, &slot_data);
	ber_memfree (slot_data.al_appname);
	if (res == -1) { 
		close (info->al_fd);
		return ALOCK_UNSTABLE;
	}
	alock_share_lock (info->al_fd, info->al_slot);

	res = alock_release_lock (info->al_fd, 0);
	if (res == -1) { 
		close (info->al_fd);
		return ALOCK_UNSTABLE;
	}
	
	if (dirty_count) return ALOCK_RECOVER | nosave;
	return ALOCK_CLEAN | nosave;
}

int 
alock_scan ( alock_info_t * info )
{
	struct stat statbuf;
	alock_info_t scan_info;
	int res, max_slot;
	int dirty_count, live_count, nosave;

	assert (info != NULL);

	scan_info.al_fd = info->al_fd;

	res = alock_grab_lock (info->al_fd, 0);
	if (res == -1) {
		close (info->al_fd);
		return ALOCK_UNSTABLE;
	}

	res = fstat (info->al_fd, &statbuf);
	if (res == -1) {
		close (info->al_fd);
		return ALOCK_UNSTABLE;
	}

	max_slot = (statbuf.st_size + ALOCK_SLOT_SIZE - 1) / ALOCK_SLOT_SIZE;
	dirty_count = 0;
	live_count = 0;
	nosave = 0;
	for (scan_info.al_slot = 1; 
	     scan_info.al_slot < max_slot;
	     ++ scan_info.al_slot) {
		if (scan_info.al_slot != info->al_slot) {
			res = alock_query_slot (&scan_info);

			if (res & ALOCK_NOSAVE) {
				nosave = ALOCK_NOSAVE;
				res ^= ALOCK_NOSAVE;
			}

			if (res == ALOCK_LOCKED) {
				++live_count;
				
			} else if (res == ALOCK_DIRTY) {
				++dirty_count;

			} else if (res == -1) {
				close (info->al_fd);
				return ALOCK_UNSTABLE;

			}
		}
	}

	res = alock_release_lock (info->al_fd, 0);
	if (res == -1) {
		close (info->al_fd);
		return ALOCK_UNSTABLE;
	}

	if (dirty_count) {
		if (live_count) {
			close (info->al_fd);
			return ALOCK_UNSTABLE;
		} else {
			return ALOCK_RECOVER | nosave;
		}
	}
	
	return ALOCK_CLEAN | nosave;
}

int
alock_close ( alock_info_t * info, int nosave )
{
	alock_slot_t slot_data;
	int res;

	if ( !info->al_slot )
		return ALOCK_CLEAN;

	(void) memset ((void *) &slot_data, 0, sizeof(alock_slot_t));

	res = alock_grab_lock (info->al_fd, 0);
	if (res == -1) {
fail:
		/* Windows doesn't clean up locks immediately when a process exits.
		 * Make sure we release our locks, to prevent stale locks from
		 * hanging around.
		 */
		alock_release_lock (info->al_fd, 0);
		close (info->al_fd);
		return ALOCK_UNSTABLE;
	}

	/* mark our slot as clean */
	res = alock_read_slot (info, &slot_data);
	if (res == -1) {
		if (slot_data.al_appname != NULL) 
			ber_memfree (slot_data.al_appname);
		goto fail;
	}
	slot_data.al_lock = ALOCK_UNLOCKED;
	if ( nosave )
		slot_data.al_lock |= ALOCK_NOSAVE;
	/* since we have slot 0 locked, we don't need our slot lock */
	res = alock_release_lock (info->al_fd, info->al_slot);
	if (res == -1) {
		goto fail;
	}
	res = alock_write_slot (info, &slot_data);
	if (res == -1) {
		if (slot_data.al_appname != NULL) 
			ber_memfree (slot_data.al_appname);
		goto fail;
	}
	if (slot_data.al_appname != NULL) {
		ber_memfree (slot_data.al_appname);
		slot_data.al_appname = NULL;
	}

	res = alock_release_lock (info->al_fd, 0);
	if (res == -1) {
		close (info->al_fd);
		return ALOCK_UNSTABLE;
	}

	res = close (info->al_fd);
	if (res == -1) return ALOCK_UNSTABLE;
	
	return ALOCK_CLEAN;
}

int 
alock_recover ( alock_info_t * info )
{
	struct stat statbuf;
	alock_slot_t slot_data;
	alock_info_t scan_info;
	int res, max_slot;

	assert (info != NULL);

	scan_info.al_fd = info->al_fd;

	(void) memset ((void *) &slot_data, 0, sizeof(alock_slot_t));

	res = alock_grab_lock (info->al_fd, 0);
	if (res == -1) {
		goto fail;
	}

	res = fstat (info->al_fd, &statbuf);
	if (res == -1) {
		goto fail;
	}

	max_slot = (statbuf.st_size + ALOCK_SLOT_SIZE - 1) / ALOCK_SLOT_SIZE;
	for (scan_info.al_slot = 1; 
	     scan_info.al_slot < max_slot;
	     ++ scan_info.al_slot) {
		if (scan_info.al_slot != info->al_slot) {
			res = alock_query_slot (&scan_info) & ~ALOCK_NOSAVE;

			if (res == ALOCK_LOCKED
			    || res == ALOCK_UNIQUE) {
				/* recovery attempt on an active db? */
				goto fail;
				
			} else if (res == ALOCK_DIRTY) {
				/* mark it clean */
				res = alock_read_slot (&scan_info, &slot_data);
				if (res == -1) {
					goto fail;
				}
				slot_data.al_lock = ALOCK_UNLOCKED;
				res = alock_write_slot (&scan_info, &slot_data);
				if (res == -1) {
					if (slot_data.al_appname != NULL) 
						ber_memfree (slot_data.al_appname);
					goto fail;
				}
				if (slot_data.al_appname != NULL) {
					ber_memfree (slot_data.al_appname);
					slot_data.al_appname = NULL;
				}
				
			} else if (res == -1) {
				goto fail;

			}
		}
	}

	res = alock_release_lock (info->al_fd, 0);
	if (res == -1) {
		close (info->al_fd);
		return ALOCK_UNSTABLE;
	}

	return ALOCK_CLEAN;

fail:
	alock_release_lock (info->al_fd, 0);
	close (info->al_fd);
	return ALOCK_UNSTABLE;
}

#endif /* SLAPD_BDB || SLAPD_HDB */
