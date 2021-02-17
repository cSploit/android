/*
   tio.h - timed io functions
   This file is part of the nss-pam-ldapd library.

   Copyright (C) 2007, 2008, 2010, 2012 Arthur de Jong

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with this library; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
   02110-1301 USA
*/

/*

   TODO: Add some documentation here.

   the SIGPIPE signal should be ignored (is ignored in this code)

   This library is not thread safe. You cannot share TFILE objects between
   threads and expect to be able to read and write from them in different
   threads. All the state is in the TFILE object so calls to this library on
   different objects can be done in parallel.

*/

#ifndef COMMON__TIO_H
#define COMMON__TIO_H

#include <sys/time.h>
#include <sys/types.h>

#include "attrs.h"

/* This is a generic file handle used for reading and writing
   (something like FILE from stdio.h). */
typedef struct tio_fileinfo TFILE;

/* Open a new TFILE based on the file descriptor. The timeout is set for any
   operation (value in milliseconds). */
TFILE *tio_fdopen(int fd,int readtimeout,int writetimeout,
                  size_t initreadsize,size_t maxreadsize,
                  size_t initwritesize,size_t maxwritesize)
  LIKE_MALLOC MUST_USE;

/* Read the specified number of bytes from the stream. */
int tio_read(TFILE *fp,void *buf,size_t count);

/* Read and discard the specified number of bytes from the stream. */
int tio_skip(TFILE *fp,size_t count);

/* Read all available data from the stream and empty the read buffer. */
int tio_skipall(TFILE *fp);

/* Write the specified buffer to the stream. */
int tio_write(TFILE *fp,const void *buf,size_t count);

/* Write out all buffered data to the stream. */
int tio_flush(TFILE *fp);

/* Flush the streams and closes the underlying file descriptor. */
int tio_close(TFILE *fp);

/* Store the current position in the stream so that we can jump back to it
   with the tio_reset() function. */
void tio_mark(TFILE *fp);

/* Rewinds the stream to the point set by tio_mark(). Note that this only
   resets the read stream and not the write stream. This function returns
   whether the reset was successful (this function may fail if the buffers
   were full). */
int tio_reset(TFILE *fp);

#endif /* COMMON__TIO_H */
