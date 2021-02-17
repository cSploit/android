/*
   nslcd-prot.h - helper macros for reading and writing in protocol streams

   Copyright (C) 2006 West Consulting
   Copyright (C) 2006, 2007, 2009 Arthur de Jong

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

#ifndef COMMON__NSLCD_PROT_H
#define COMMON__NSLCD_PROT_H 1

#include "tio.h"

/* If you use these macros you should define the following macros to
   handle error conditions (these marcos should clean up and return from the
   function):
     ERROR_OUT_WRITEERROR(fp)
     ERROR_OUT_READERROR(fp)
     ERROR_OUT_BUFERROR(fp)
     ERROR_OUT_NOSUCCESS(fp) */


/* Debugging marcos that can be used to enable detailed protocol logging,
   pass -DDEBUG_PROT to do overall protocol debugging, and -DDEBUG_PROT_DUMP
   to dump the actual bytestream. */

#ifdef DEBUG_PROT
/* define a debugging macro to output logging */
#include <string.h>
#include <errno.h>
#define DEBUG_PRINT(fmt,arg) \
  fprintf(stderr,"%s:%d:%s: " fmt "\n",__FILE__,__LINE__,__PRETTY_FUNCTION__,arg);
#else /* DEBUG_PROT */
/* define an empty debug macro to disable logging */
#define DEBUG_PRINT(fmt,arg)
#endif /* not DEBUG_PROT */

#ifdef DEBUG_PROT_DUMP
/* define a debugging macro to output detailed logging */
#ifdef HAVE_STDINT_H
#include <stdint.h>
#endif /* HAVE_STDINT_H */
static void debug_dump(const void *ptr,size_t size)
{
  int i;
  for (i=0;i<size;i++)
    fprintf(stderr," %02x",((const uint8_t *)ptr)[i]);
  fprintf(stderr,"\n");
}
#define DEBUG_DUMP(ptr,size) \
  fprintf(stderr,"%s:%d:%s:",__FILE__,__LINE__,__PRETTY_FUNCTION__); \
  debug_dump(ptr,size);
#else /* DEBUG_PROT_DUMP */
/* define an empty debug macro to disable logging */
#define DEBUG_DUMP(ptr,size)
#endif /* not DEBUG_PROT_DUMP */


/* WRITE marcos, used for writing data, on write error they will
   call the ERROR_OUT_WRITEERROR macro
   these macros may require the availability of the following
   variables:
   int32_t tmpint32; - temporary variable
   */

#define WRITE(fp,ptr,size) \
  DEBUG_PRINT("WRITE       : var="__STRING(ptr)" size=%d",(int)size); \
  DEBUG_DUMP(ptr,size); \
  if (tio_write(fp,ptr,(size_t)size)) \
  { \
    DEBUG_PRINT("WRITE       : var="__STRING(ptr)" error: %s",strerror(errno)); \
    ERROR_OUT_WRITEERROR(fp); \
  }

#define WRITE_TYPE(fp,field,type) \
  WRITE(fp,&(field),sizeof(type))

#define WRITE_INT32(fp,i) \
  DEBUG_PRINT("WRITE_INT32 : var="__STRING(i)" int32=%d",(int)i); \
  tmpint32=(int32_t)(i); \
  WRITE_TYPE(fp,tmpint32,int32_t)

#define WRITE_STRING(fp,str) \
  DEBUG_PRINT("WRITE_STRING: var="__STRING(str)" string=\"%s\"",(str)); \
  if ((str)==NULL) \
  { \
    WRITE_INT32(fp,0); \
  } \
  else \
  { \
    WRITE_INT32(fp,strlen(str)); \
    if (tmpint32>0) \
      { WRITE(fp,(str),tmpint32); } \
  }

#define WRITE_STRINGLIST(fp,arr) \
  if ((arr)==NULL) \
  { \
    DEBUG_PRINT("WRITE_STRLST: var="__STRING(arr)" num=%d",0); \
    WRITE_INT32(fp,0); \
  } \
  else \
  { \
    /* first determin length of array */ \
    for (tmp3int32=0;(arr)[tmp3int32]!=NULL;tmp3int32++) \
      /*noting*/ ; \
    /* write number of strings */ \
    DEBUG_PRINT("WRITE_STRLST: var="__STRING(arr)" num=%d",(int)tmp3int32); \
    WRITE_TYPE(fp,tmp3int32,int32_t); \
    /* write strings */ \
    for (tmp2int32=0;tmp2int32<tmp3int32;tmp2int32++) \
    { \
      WRITE_STRING(fp,(arr)[tmp2int32]); \
    } \
  }

#define WRITE_STRINGLIST_EXCEPT(fp,arr,not) \
  /* first determin length of array */ \
  tmp3int32=0; \
  for (tmp2int32=0;(arr)[tmp2int32]!=NULL;tmp2int32++) \
    if (strcmp((arr)[tmp2int32],(not))!=0) \
      tmp3int32++; \
  /* write number of strings (mius one because we intend to skip one) */ \
  DEBUG_PRINT("WRITE_STRLST: var="__STRING(arr)" num=%d",(int)tmp3int32); \
  WRITE_TYPE(fp,tmp3int32,int32_t); \
  /* write strings */ \
  for (tmp2int32=0;(arr)[tmp2int32]!=NULL;tmp2int32++) \
  { \
    if (strcmp((arr)[tmp2int32],(not))!=0) \
    { \
      WRITE_STRING(fp,(arr)[tmp2int32]); \
    } \
  }


/* READ macros, used for reading data, on read error they will
   call the ERROR_OUT_READERROR or ERROR_OUT_BUFERROR macro
   these macros may require the availability of the following
   variables:
   int32_t tmpint32; - temporary variable
   */

#define READ(fp,ptr,size) \
  if (tio_read(fp,ptr,(size_t)size)) \
  { \
    DEBUG_PRINT("READ       : var="__STRING(ptr)" error: %s",strerror(errno)); \
    ERROR_OUT_READERROR(fp); \
  } \
  DEBUG_PRINT("READ       : var="__STRING(ptr)" size=%d",(int)size); \
  DEBUG_DUMP(ptr,size);

#define READ_TYPE(fp,field,type) \
  READ(fp,&(field),sizeof(type))

#define READ_INT32(fp,i) \
  READ_TYPE(fp,tmpint32,int32_t); \
  i=tmpint32; \
  DEBUG_PRINT("READ_INT32 : var="__STRING(i)" int32=%d",(int)i);

/* read a string in a fixed-size "normal" buffer */
#define READ_STRING(fp,buffer) \
  /* read the size of the string */ \
  READ_TYPE(fp,tmpint32,int32_t); \
  DEBUG_PRINT("READ_STRING: var="__STRING(buffer)" strlen=%d",tmpint32); \
  /* check if read would fit */ \
  if (((size_t)tmpint32)>=sizeof(buffer)) \
  { \
    /* will not fit */ \
    tmpint32=(tmpint32-sizeof(buffer))+1; \
    DEBUG_PRINT("READ       : buffer %d bytes too small",tmpint32); \
    ERROR_OUT_BUFERROR(fp); \
  } \
  /* read string from the stream */ \
  if (tmpint32>0) \
    { READ(fp,buffer,(size_t)tmpint32); } \
  /* null-terminate string in buffer */ \
  buffer[tmpint32]='\0'; \
  DEBUG_PRINT("READ_STRING: var="__STRING(buffer)" string=\"%s\"",buffer);


/* READ BUF macros that read data into a pre-allocated buffer.
   these macros may require the availability of the following
   variables:
   int32_t tmpint32; - temporary variable
   char *buffer;     - pointer to a buffer for reading strings
   size_t buflen;    - the size of the buffer
   size_t bufptr;    - the current position in the buffer
   */

/* current position in the buffer */
#define BUF_CUR \
  (buffer+bufptr)

/* check that the buffer has sz bytes left in it */
#define BUF_CHECK(fp,sz) \
  if ((bufptr+(size_t)(sz))>buflen) \
  { \
    /* will not fit */ \
    tmpint32=bufptr+(sz)-(buflen); \
    DEBUG_PRINT("READ       : buffer %d bytes too small",tmpint32); \
    ERROR_OUT_BUFERROR(fp); \
  }

/* move the buffer pointer */
#define BUF_SKIP(sz) \
  bufptr+=(size_t)(sz);

/* move BUF_CUR foreward so that it is aligned to the specified
   type width */
#define BUF_ALIGN(fp,type) \
  /* figure out number of bytes to skip foreward */ \
  tmp2int32=(sizeof(type)-((BUF_CUR-(char *)NULL)%sizeof(type)))%sizeof(type); \
  /* check and skip */ \
  BUF_CHECK(fp,tmp2int32); \
  BUF_SKIP(tmp2int32);

/* allocate a piece of the buffer to store an array in */
#define BUF_ALLOC(fp,ptr,type,num) \
  /* align to the specified type width */ \
  BUF_ALIGN(fp,type); \
  /* check that we have enough room */ \
  BUF_CHECK(fp,(size_t)(num)*sizeof(type)); \
  /* store the pointer */ \
  (ptr)=(type *)BUF_CUR; \
  /* reserve the space */ \
  BUF_SKIP((size_t)(num)*sizeof(type));

/* read a binary blob into the buffer */
#define READ_BUF(fp,ptr,sz) \
  /* check that there is enough room and read */ \
  BUF_CHECK(fp,sz); \
  READ(fp,BUF_CUR,(size_t)sz); \
  /* store pointer and skip */ \
  (ptr)=BUF_CUR; \
  BUF_SKIP(sz);

/* read string in the buffer (using buffer, buflen and bufptr)
   and store the actual location of the string in field */
#define READ_BUF_STRING(fp,field) \
  /* read the size of the string */ \
  READ_TYPE(fp,tmpint32,int32_t); \
  DEBUG_PRINT("READ_BUF_STRING: var="__STRING(field)" strlen=%d",tmpint32); \
  /* check if read would fit */ \
  BUF_CHECK(fp,tmpint32+1); \
  /* read string from the stream */ \
  if (tmpint32>0) \
    { READ(fp,BUF_CUR,(size_t)tmpint32); } \
  /* null-terminate string in buffer */ \
  BUF_CUR[tmpint32]='\0'; \
  DEBUG_PRINT("READ_BUF_STRING: var="__STRING(field)" string=\"%s\"",BUF_CUR); \
  /* prepare result */ \
  (field)=BUF_CUR; \
  BUF_SKIP(tmpint32+1);

/* read an array from a stram and store it as a null-terminated
   array list (size for the array is allocated) */
#define READ_BUF_STRINGLIST(fp,arr) \
  /* read the number of entries */ \
  READ_TYPE(fp,tmp3int32,int32_t); \
  DEBUG_PRINT("READ_STRLST: var="__STRING(arr)" num=%d",(int)tmp3int32); \
  /* allocate room for *char[num+1] */ \
  BUF_ALLOC(fp,arr,char *,tmp3int32+1); \
  /* read all entries */ \
  for (tmp2int32=0;tmp2int32<tmp3int32;tmp2int32++) \
  { \
    READ_BUF_STRING(fp,(arr)[tmp2int32]); \
  } \
  /* set last entry to NULL */ \
  (arr)[tmp2int32]=NULL;


/* SKIP macros for skipping over certain parts of the protocol stream. */

/* skip a number of bytes foreward */
#define SKIP(fp,sz) \
  DEBUG_PRINT("READ       : skip %d bytes",(int)(sz)); \
  /* read (skip) the specified number of bytes */ \
  if (tio_skip(fp,sz)) \
  { \
    DEBUG_PRINT("READ       : skip error: %s",strerror(errno)); \
    ERROR_OUT_READERROR(fp); \
  }

/* read a string from the stream but don't do anything with the result */
#define SKIP_STRING(fp) \
  /* read the size of the string */ \
  READ_TYPE(fp,tmpint32,int32_t); \
  DEBUG_PRINT("READ_STRING: skip %d bytes",(int)tmpint32); \
  /* read (skip) the specified number of bytes */ \
  SKIP(fp,tmpint32);

/* skip a list of strings */
#define SKIP_STRINGLIST(fp) \
  /* read the number of entries */ \
  READ_TYPE(fp,tmp3int32,int32_t); \
  DEBUG_PRINT("READ_STRLST: skip %d strings",(int)tmp3int32); \
  /* read all entries */ \
  for (tmp2int32=0;tmp2int32<tmp3int32;tmp2int32++) \
  { \
    SKIP_STRING(fp); \
  }


/* These are functions and macors for performing common operations in
   the nslcd request/response protocol. */

/* returns a socket to the server or NULL on error (see errno),
   socket should be closed with tio_close() */
TFILE *nslcd_client_open(void)
  MUST_USE;

/* generic request code */
#define NSLCD_REQUEST(fp,action,writefn) \
  /* open a client socket */ \
  if ((fp=nslcd_client_open())==NULL) \
    { ERROR_OUT_OPENERROR } \
  /* write a request header with a request code */ \
  WRITE_INT32(fp,(int32_t)NSLCD_VERSION) \
  WRITE_INT32(fp,(int32_t)action) \
  /* write the request parameters (if any) */ \
  writefn; \
  /* flush the stream */ \
  if (tio_flush(fp)<0) \
  { \
    DEBUG_PRINT("WRITE_FLUSH : error: %s",strerror(errno)); \
    ERROR_OUT_WRITEERROR(fp); \
  } \
  /* read and check response version number */ \
  READ_TYPE(fp,tmpint32,int32_t); \
  if (tmpint32!=(int32_t)NSLCD_VERSION) \
    { ERROR_OUT_READERROR(fp) } \
  /* read and check response request number */ \
  READ_TYPE(fp,tmpint32,int32_t); \
  if (tmpint32!=(int32_t)(action)) \
    { ERROR_OUT_READERROR(fp) }

/* Read the response code (the result code of the query) from
   the stream. */
#define READ_RESPONSE_CODE(fp) \
  READ_TYPE(fp,tmpint32,int32_t); \
  if (tmpint32!=(int32_t)NSLCD_RESULT_BEGIN) \
    { ERROR_OUT_NOSUCCESS(fp) }

#endif /* not COMMON__NSLCD_PROT_H */
