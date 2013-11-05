/*
 * marshal.c :  Marshalling routines for Subversion protocol
 *
 * ====================================================================
 *    Licensed to the Apache Software Foundation (ASF) under one
 *    or more contributor license agreements.  See the NOTICE file
 *    distributed with this work for additional information
 *    regarding copyright ownership.  The ASF licenses this file
 *    to you under the Apache License, Version 2.0 (the
 *    "License"); you may not use this file except in compliance
 *    with the License.  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing,
 *    software distributed under the License is distributed on an
 *    "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 *    KIND, either express or implied.  See the License for the
 *    specific language governing permissions and limitations
 *    under the License.
 * ====================================================================
 */



#include <assert.h>
#include <stdlib.h>

#define APR_WANT_STRFUNC
#include <apr_want.h>
#include <apr_general.h>
#include <apr_lib.h>
#include <apr_strings.h>

#include "svn_types.h"
#include "svn_string.h"
#include "svn_error.h"
#include "svn_pools.h"
#include "svn_ra_svn.h"
#include "svn_private_config.h"
#include "svn_ctype.h"

#include "ra_svn.h"

#include "private/svn_string_private.h"
#include "private/svn_dep_compat.h"

#define svn_iswhitespace(c) ((c) == ' ' || (c) == '\n')

/* If we receive data that *claims* to be followed by a very long string,
 * we should not trust that claim right away. But everything up to 1 MB
 * should be too small to be instrumental for a DOS attack. */

#define SUSPICIOUSLY_HUGE_STRING_SIZE_THRESHOLD (0x100000)

/* --- CONNECTION INITIALIZATION --- */

svn_ra_svn_conn_t *svn_ra_svn_create_conn2(apr_socket_t *sock,
                                           apr_file_t *in_file,
                                           apr_file_t *out_file,
                                           int compression_level,
                                           apr_pool_t *pool)
{
  svn_ra_svn_conn_t *conn = apr_palloc(pool, sizeof(*conn));

  assert((sock && !in_file && !out_file) || (!sock && in_file && out_file));
#ifdef SVN_HAVE_SASL
  conn->sock = sock;
  conn->encrypted = FALSE;
#endif
  conn->session = NULL;
  conn->read_ptr = conn->read_buf;
  conn->read_end = conn->read_buf;
  conn->write_pos = 0;
  conn->block_handler = NULL;
  conn->block_baton = NULL;
  conn->capabilities = apr_hash_make(pool);
  conn->compression_level = compression_level;
  conn->pool = pool;

  if (sock != NULL)
    {
      apr_sockaddr_t *sa;
      conn->stream = svn_ra_svn__stream_from_sock(sock, pool);
      if (!(apr_socket_addr_get(&sa, APR_REMOTE, sock) == APR_SUCCESS
            && apr_sockaddr_ip_get(&conn->remote_ip, sa) == APR_SUCCESS))
        conn->remote_ip = NULL;
    }
  else
    {
      conn->stream = svn_ra_svn__stream_from_files(in_file, out_file, pool);
      conn->remote_ip = NULL;
    }

  return conn;
}

/* backward-compatible implementation using the default compression level */
svn_ra_svn_conn_t *svn_ra_svn_create_conn(apr_socket_t *sock,
                                          apr_file_t *in_file,
                                          apr_file_t *out_file,
                                          apr_pool_t *pool)
{
  return svn_ra_svn_create_conn2(sock, in_file, out_file,
                                 SVN_DELTA_COMPRESSION_LEVEL_DEFAULT, pool);
}

svn_error_t *svn_ra_svn_set_capabilities(svn_ra_svn_conn_t *conn,
                                         const apr_array_header_t *list)
{
  int i;
  svn_ra_svn_item_t *item;
  const char *word;

  for (i = 0; i < list->nelts; i++)
    {
      item = &APR_ARRAY_IDX(list, i, svn_ra_svn_item_t);
      if (item->kind != SVN_RA_SVN_WORD)
        return svn_error_create(SVN_ERR_RA_SVN_MALFORMED_DATA, NULL,
                                _("Capability entry is not a word"));
      word = apr_pstrdup(conn->pool, item->u.word);
      apr_hash_set(conn->capabilities, word, APR_HASH_KEY_STRING, word);
    }
  return SVN_NO_ERROR;
}

svn_boolean_t svn_ra_svn_has_capability(svn_ra_svn_conn_t *conn,
                                        const char *capability)
{
  return (apr_hash_get(conn->capabilities, capability,
                       APR_HASH_KEY_STRING) != NULL);
}

int
svn_ra_svn_compression_level(svn_ra_svn_conn_t *conn)
{
  return conn->compression_level;
}

const char *svn_ra_svn_conn_remote_host(svn_ra_svn_conn_t *conn)
{
  return conn->remote_ip;
}

void
svn_ra_svn__set_block_handler(svn_ra_svn_conn_t *conn,
                              ra_svn_block_handler_t handler,
                              void *baton)
{
  apr_interval_time_t interval = (handler) ? 0 : -1;

  conn->block_handler = handler;
  conn->block_baton = baton;
  svn_ra_svn__stream_timeout(conn->stream, interval);
}

svn_boolean_t svn_ra_svn__input_waiting(svn_ra_svn_conn_t *conn,
                                        apr_pool_t *pool)
{
  return svn_ra_svn__stream_pending(conn->stream);
}

/* --- WRITE BUFFER MANAGEMENT --- */

/* Write bytes into the write buffer until either the write buffer is
 * full or we reach END. */
static const char *writebuf_push(svn_ra_svn_conn_t *conn, const char *data,
                                 const char *end)
{
  apr_ssize_t buflen, copylen;

  buflen = sizeof(conn->write_buf) - conn->write_pos;
  copylen = (buflen < end - data) ? buflen : end - data;
  memcpy(conn->write_buf + conn->write_pos, data, copylen);
  conn->write_pos += copylen;
  return data + copylen;
}

/* Write data to socket or output file as appropriate. */
static svn_error_t *writebuf_output(svn_ra_svn_conn_t *conn, apr_pool_t *pool,
                                    const char *data, apr_size_t len)
{
  const char *end = data + len;
  apr_size_t count;
  apr_pool_t *subpool = NULL;
  svn_ra_svn__session_baton_t *session = conn->session;

  while (data < end)
    {
      count = end - data;

      if (session && session->callbacks && session->callbacks->cancel_func)
        SVN_ERR((session->callbacks->cancel_func)(session->callbacks_baton));

      SVN_ERR(svn_ra_svn__stream_write(conn->stream, data, &count));
      if (count == 0)
        {
          if (!subpool)
            subpool = svn_pool_create(pool);
          else
            svn_pool_clear(subpool);
          SVN_ERR(conn->block_handler(conn, subpool, conn->block_baton));
        }
      data += count;

      if (session)
        {
          const svn_ra_callbacks2_t *cb = session->callbacks;
          session->bytes_written += count;

          if (cb && cb->progress_func)
            (cb->progress_func)(session->bytes_written + session->bytes_read,
                                -1, cb->progress_baton, subpool);
        }
    }

  if (subpool)
    svn_pool_destroy(subpool);
  return SVN_NO_ERROR;
}

/* Write data from the write buffer out to the socket. */
static svn_error_t *writebuf_flush(svn_ra_svn_conn_t *conn, apr_pool_t *pool)
{
  int write_pos = conn->write_pos;

  /* Clear conn->write_pos first in case the block handler does a read. */
  conn->write_pos = 0;
  SVN_ERR(writebuf_output(conn, pool, conn->write_buf, write_pos));
  return SVN_NO_ERROR;
}

static svn_error_t *writebuf_write(svn_ra_svn_conn_t *conn, apr_pool_t *pool,
                                   const char *data, apr_size_t len)
{
  const char *end = data + len;

  if (conn->write_pos > 0 && conn->write_pos + len > sizeof(conn->write_buf))
    {
      /* Fill and then empty the write buffer. */
      data = writebuf_push(conn, data, end);
      SVN_ERR(writebuf_flush(conn, pool));
    }

  if (end - data > (apr_ssize_t)sizeof(conn->write_buf))
    SVN_ERR(writebuf_output(conn, pool, data, end - data));
  else
    writebuf_push(conn, data, end);
  return SVN_NO_ERROR;
}

static svn_error_t *writebuf_printf(svn_ra_svn_conn_t *conn, apr_pool_t *pool,
                                    const char *fmt, ...)
  __attribute__ ((format(printf, 3, 4)));
static svn_error_t *writebuf_printf(svn_ra_svn_conn_t *conn, apr_pool_t *pool,
                                    const char *fmt, ...)
{
  va_list ap;
  char *str;

  va_start(ap, fmt);
  str = apr_pvsprintf(pool, fmt, ap);
  va_end(ap);
  return writebuf_write(conn, pool, str, strlen(str));
}

/* --- READ BUFFER MANAGEMENT --- */

/* Read bytes into DATA until either the read buffer is empty or
 * we reach END. */
static char *readbuf_drain(svn_ra_svn_conn_t *conn, char *data, char *end)
{
  apr_ssize_t buflen, copylen;

  buflen = conn->read_end - conn->read_ptr;
  copylen = (buflen < end - data) ? buflen : end - data;
  memcpy(data, conn->read_ptr, copylen);
  conn->read_ptr += copylen;
  return data + copylen;
}

/* Read data from socket or input file as appropriate. */
static svn_error_t *readbuf_input(svn_ra_svn_conn_t *conn, char *data,
                                  apr_size_t *len, apr_pool_t *pool)
{
  svn_ra_svn__session_baton_t *session = conn->session;

  if (session && session->callbacks && session->callbacks->cancel_func)
    SVN_ERR((session->callbacks->cancel_func)(session->callbacks_baton));

  SVN_ERR(svn_ra_svn__stream_read(conn->stream, data, len));
  if (*len == 0)
    return svn_error_create(SVN_ERR_RA_SVN_CONNECTION_CLOSED, NULL, NULL);

  if (session)
    {
      const svn_ra_callbacks2_t *cb = session->callbacks;
      session->bytes_read += *len;

      if (cb && cb->progress_func)
        (cb->progress_func)(session->bytes_read + session->bytes_written,
                            -1, cb->progress_baton, pool);
    }

  return SVN_NO_ERROR;
}

/* Read data from the socket into the read buffer, which must be empty. */
static svn_error_t *readbuf_fill(svn_ra_svn_conn_t *conn, apr_pool_t *pool)
{
  apr_size_t len;

  SVN_ERR_ASSERT(conn->read_ptr == conn->read_end);
  SVN_ERR(writebuf_flush(conn, pool));
  len = sizeof(conn->read_buf);
  SVN_ERR(readbuf_input(conn, conn->read_buf, &len, pool));
  conn->read_ptr = conn->read_buf;
  conn->read_end = conn->read_buf + len;
  return SVN_NO_ERROR;
}

static APR_INLINE svn_error_t *
readbuf_getchar(svn_ra_svn_conn_t *conn, apr_pool_t *pool, char *result)
{
  if (conn->read_ptr == conn->read_end)
    SVN_ERR(readbuf_fill(conn, pool));
  *result = *conn->read_ptr++;
  return SVN_NO_ERROR;
}

static svn_error_t *readbuf_getchar_skip_whitespace(svn_ra_svn_conn_t *conn,
                                                    apr_pool_t *pool,
                                                    char *result)
{
  do
    SVN_ERR(readbuf_getchar(conn, pool, result));
  while (svn_iswhitespace(*result));
  return SVN_NO_ERROR;
}

/* Read the next LEN bytes from CONN and copy them to *DATA. */
static svn_error_t *readbuf_read(svn_ra_svn_conn_t *conn, apr_pool_t *pool,
                                 char *data, apr_size_t len)
{
  char *end = data + len;
  apr_size_t count;

  /* Copy in an appropriate amount of data from the buffer. */
  data = readbuf_drain(conn, data, end);

  /* Read large chunks directly into buffer. */
  while (end - data > (apr_ssize_t)sizeof(conn->read_buf))
    {
      SVN_ERR(writebuf_flush(conn, pool));
      count = end - data;
      SVN_ERR(readbuf_input(conn, data, &count, pool));
      data += count;
    }

  while (end > data)
    {
      /* The remaining amount to read is small; fill the buffer and
       * copy from that. */
      SVN_ERR(readbuf_fill(conn, pool));
      data = readbuf_drain(conn, data, end);
    }

  return SVN_NO_ERROR;
}

static svn_error_t *readbuf_skip_leading_garbage(svn_ra_svn_conn_t *conn,
                                                 apr_pool_t *pool)
{
  char buf[256];  /* Must be smaller than sizeof(conn->read_buf) - 1. */
  const char *p, *end;
  apr_size_t len;
  svn_boolean_t lparen = FALSE;

  SVN_ERR_ASSERT(conn->read_ptr == conn->read_end);
  while (1)
    {
      /* Read some data directly from the connection input source. */
      len = sizeof(buf);
      SVN_ERR(readbuf_input(conn, buf, &len, pool));
      end = buf + len;

      /* Scan the data for '(' WS with a very simple state machine. */
      for (p = buf; p < end; p++)
        {
          if (lparen && svn_iswhitespace(*p))
            break;
          else
            lparen = (*p == '(');
        }
      if (p < end)
        break;
    }

  /* p now points to the whitespace just after the left paren.  Fake
   * up the left paren and then copy what we have into the read
   * buffer. */
  conn->read_buf[0] = '(';
  memcpy(conn->read_buf + 1, p, end - p);
  conn->read_ptr = conn->read_buf;
  conn->read_end = conn->read_buf + 1 + (end - p);
  return SVN_NO_ERROR;
}

/* --- WRITING DATA ITEMS --- */

svn_error_t *svn_ra_svn_write_number(svn_ra_svn_conn_t *conn, apr_pool_t *pool,
                                     apr_uint64_t number)
{
  return writebuf_printf(conn, pool, "%" APR_UINT64_T_FMT " ", number);
}

svn_error_t *svn_ra_svn_write_string(svn_ra_svn_conn_t *conn, apr_pool_t *pool,
                                     const svn_string_t *str)
{
  SVN_ERR(writebuf_printf(conn, pool, "%" APR_SIZE_T_FMT ":", str->len));
  SVN_ERR(writebuf_write(conn, pool, str->data, str->len));
  SVN_ERR(writebuf_write(conn, pool, " ", 1));
  return SVN_NO_ERROR;
}

svn_error_t *svn_ra_svn_write_cstring(svn_ra_svn_conn_t *conn,
                                      apr_pool_t *pool, const char *s)
{
  return writebuf_printf(conn, pool, "%" APR_SIZE_T_FMT ":%s ", strlen(s), s);
}

svn_error_t *svn_ra_svn_write_word(svn_ra_svn_conn_t *conn, apr_pool_t *pool,
                                   const char *word)
{
  return writebuf_printf(conn, pool, "%s ", word);
}

svn_error_t *svn_ra_svn_write_proplist(svn_ra_svn_conn_t *conn,
                                       apr_pool_t *pool, apr_hash_t *props)
{
  apr_pool_t *iterpool;
  apr_hash_index_t *hi;
  const void *key;
  void *val;
  const char *propname;
  svn_string_t *propval;

  if (props)
    {
      iterpool = svn_pool_create(pool);
      for (hi = apr_hash_first(pool, props); hi; hi = apr_hash_next(hi))
        {
          svn_pool_clear(iterpool);
          apr_hash_this(hi, &key, NULL, &val);
          propname = key;
          propval = val;
          SVN_ERR(svn_ra_svn_write_tuple(conn, iterpool, "cs",
                                         propname, propval));
        }
      svn_pool_destroy(iterpool);
    }

  return SVN_NO_ERROR;
}

svn_error_t *svn_ra_svn_start_list(svn_ra_svn_conn_t *conn, apr_pool_t *pool)
{
  return writebuf_write(conn, pool, "( ", 2);
}

svn_error_t *svn_ra_svn_end_list(svn_ra_svn_conn_t *conn, apr_pool_t *pool)
{
  return writebuf_write(conn, pool, ") ", 2);
}

svn_error_t *svn_ra_svn_flush(svn_ra_svn_conn_t *conn, apr_pool_t *pool)
{
  return writebuf_flush(conn, pool);
}

/* --- WRITING TUPLES --- */

static svn_error_t *vwrite_tuple(svn_ra_svn_conn_t *conn, apr_pool_t *pool,
                                 const char *fmt, va_list ap)
{
  svn_boolean_t opt = FALSE;
  svn_revnum_t rev;
  const char *cstr;
  const svn_string_t *str;

  if (*fmt == '!')
    fmt++;
  else
    SVN_ERR(svn_ra_svn_start_list(conn, pool));
  for (; *fmt; fmt++)
    {
      if (*fmt == 'n' && !opt)
        SVN_ERR(svn_ra_svn_write_number(conn, pool, va_arg(ap, apr_uint64_t)));
      else if (*fmt == 'r')
        {
          rev = va_arg(ap, svn_revnum_t);
          SVN_ERR_ASSERT(opt || SVN_IS_VALID_REVNUM(rev));
          if (SVN_IS_VALID_REVNUM(rev))
            SVN_ERR(svn_ra_svn_write_number(conn, pool, rev));
        }
      else if (*fmt == 's')
        {
          str = va_arg(ap, const svn_string_t *);
          SVN_ERR_ASSERT(opt || str);
          if (str)
            SVN_ERR(svn_ra_svn_write_string(conn, pool, str));
        }
      else if (*fmt == 'c')
        {
          cstr = va_arg(ap, const char *);
          SVN_ERR_ASSERT(opt || cstr);
          if (cstr)
            SVN_ERR(svn_ra_svn_write_cstring(conn, pool, cstr));
        }
      else if (*fmt == 'w')
        {
          cstr = va_arg(ap, const char *);
          SVN_ERR_ASSERT(opt || cstr);
          if (cstr)
            SVN_ERR(svn_ra_svn_write_word(conn, pool, cstr));
        }
      else if (*fmt == 'b' && !opt)
        {
          cstr = va_arg(ap, svn_boolean_t) ? "true" : "false";
          SVN_ERR(svn_ra_svn_write_word(conn, pool, cstr));
        }
      else if (*fmt == '?')
        opt = TRUE;
      else if (*fmt == '(' && !opt)
        SVN_ERR(svn_ra_svn_start_list(conn, pool));
      else if (*fmt == ')')
        {
          SVN_ERR(svn_ra_svn_end_list(conn, pool));
          opt = FALSE;
        }
      else if (*fmt == '!' && !*(fmt + 1))
        return SVN_NO_ERROR;
      else
        SVN_ERR_MALFUNCTION();
    }
  SVN_ERR(svn_ra_svn_end_list(conn, pool));
  return SVN_NO_ERROR;
}

svn_error_t *svn_ra_svn_write_tuple(svn_ra_svn_conn_t *conn, apr_pool_t *pool,
                                    const char *fmt, ...)
{
  svn_error_t *err;
  va_list ap;

  va_start(ap, fmt);
  err = vwrite_tuple(conn, pool, fmt, ap);
  va_end(ap);
  return err;
}

/* --- READING DATA ITEMS --- */

/* Read LEN bytes from CONN into already-allocated structure ITEM.
 * Afterwards, *ITEM is of type 'SVN_RA_SVN_STRING', and its string
 * data is allocated in POOL. */
static svn_error_t *read_string(svn_ra_svn_conn_t *conn, apr_pool_t *pool,
                                svn_ra_svn_item_t *item, apr_uint64_t len64)
{
  svn_stringbuf_t *stringbuf;
  apr_size_t len = (apr_size_t)len64;
  apr_size_t readbuf_len;
  char *dest;

  /* We can't store strings longer than the maximum size of apr_size_t,
   * so check for wrapping */
  if (len64 > APR_SIZE_MAX)
    return svn_error_create(SVN_ERR_RA_SVN_MALFORMED_DATA, NULL,
                            _("String length larger than maximum"));

  /* Read the string in chunks.  The chunk size is large enough to avoid
   * re-allocation in typical cases, and small enough to ensure we do not
   * pre-allocate an unreasonable amount of memory if (perhaps due to
   * network data corruption or a DOS attack), we receive a bogus claim that
   * a very long string is going to follow.  In that case, we start small
   * and wait for all that data to actually show up.  This does not fully
   * prevent DOS attacks but makes them harder (you have to actually send
   * gigabytes of data). */
  readbuf_len = len < SUSPICIOUSLY_HUGE_STRING_SIZE_THRESHOLD
                    ? len
                    : SUSPICIOUSLY_HUGE_STRING_SIZE_THRESHOLD;
  stringbuf = svn_stringbuf_create_ensure(readbuf_len, pool);
  dest = stringbuf->data;

  /* Read remaining string data directly into the string structure.
   * Do it iteratively, if necessary.  */
  while (readbuf_len)
    {
      SVN_ERR(readbuf_read(conn, pool, dest, readbuf_len));

      stringbuf->len += readbuf_len;
      len -= readbuf_len;

      /* Early exit. In most cases, strings can be read in the first
       * iteration. */
      if (len == 0)
        break;

      /* Prepare next iteration: determine length of chunk to read
       * and re-alloc the string buffer. */
      readbuf_len
        = len < SUSPICIOUSLY_HUGE_STRING_SIZE_THRESHOLD
              ? len
              : SUSPICIOUSLY_HUGE_STRING_SIZE_THRESHOLD;

      svn_stringbuf_ensure(stringbuf, stringbuf->len + readbuf_len + 1);
      dest = stringbuf->data + stringbuf->len;
    }

  /* zero-terminate the string */
  stringbuf->data[stringbuf->len] = '\0';

  /* Return the string properly wrapped into an RA_SVN item. */
  item->kind = SVN_RA_SVN_STRING;
  item->u.string = svn_stringbuf__morph_into_string(stringbuf);

  return SVN_NO_ERROR;
}

/* Given the first non-whitespace character FIRST_CHAR, read an item
 * into the already allocated structure ITEM.  LEVEL should be set
 * to 0 for the first call and is used to enforce a recurssion limit
 * on the parser. */
static svn_error_t *read_item(svn_ra_svn_conn_t *conn, apr_pool_t *pool,
                              svn_ra_svn_item_t *item, char first_char,
                              int level)
{
  char c = first_char;
  apr_uint64_t val;
  svn_stringbuf_t *str;
  svn_ra_svn_item_t *listitem;

  if (++level >= 64)
    return svn_error_create(SVN_ERR_RA_SVN_MALFORMED_DATA, NULL,
                            _("Too many nested items"));


  /* Determine the item type and read it in.  Make sure that c is the
   * first character at the end of the item so we can test to make
   * sure it's whitespace. */
  if (svn_ctype_isdigit(c))
    {
      /* It's a number or a string.  Read the number part, either way. */
      val = c - '0';
      while (1)
        {
          apr_uint64_t prev_val = val;
          SVN_ERR(readbuf_getchar(conn, pool, &c));
          if (!svn_ctype_isdigit(c))
            break;
          val = val * 10 + (c - '0');
          if ((val / 10) != prev_val) /* val wrapped past maximum value */
            return svn_error_create(SVN_ERR_RA_SVN_MALFORMED_DATA, NULL,
                                    _("Number is larger than maximum"));
        }
      if (c == ':')
        {
          /* It's a string. */
          SVN_ERR(read_string(conn, pool, item, val));
          SVN_ERR(readbuf_getchar(conn, pool, &c));
        }
      else
        {
          /* It's a number. */
          item->kind = SVN_RA_SVN_NUMBER;
          item->u.number = val;
        }
    }
  else if (svn_ctype_isalpha(c))
    {
      /* It's a word. */
      str = svn_stringbuf_create_ensure(16, pool);
      svn_stringbuf_appendbyte(str, c);
      while (1)
        {
          SVN_ERR(readbuf_getchar(conn, pool, &c));
          if (!svn_ctype_isalnum(c) && c != '-')
            break;
          svn_stringbuf_appendbyte(str, c);
        }
      item->kind = SVN_RA_SVN_WORD;
      item->u.word = str->data;
    }
  else if (c == '(')
    {
      /* Read in the list items. */
      item->kind = SVN_RA_SVN_LIST;
      item->u.list = apr_array_make(pool, 4, sizeof(svn_ra_svn_item_t));
      while (1)
        {
          SVN_ERR(readbuf_getchar_skip_whitespace(conn, pool, &c));
          if (c == ')')
            break;
          listitem = apr_array_push(item->u.list);
          SVN_ERR(read_item(conn, pool, listitem, c, level));
        }
      SVN_ERR(readbuf_getchar(conn, pool, &c));
    }

  if (!svn_iswhitespace(c))
    return svn_error_create(SVN_ERR_RA_SVN_MALFORMED_DATA, NULL,
                            _("Malformed network data"));
  return SVN_NO_ERROR;
}

svn_error_t *svn_ra_svn_read_item(svn_ra_svn_conn_t *conn, apr_pool_t *pool,
                                  svn_ra_svn_item_t **item)
{
  char c;

  /* Allocate space, read the first character, and then do the rest of
   * the work.  This makes sense because of the way lists are read. */
  *item = apr_palloc(pool, sizeof(**item));
  SVN_ERR(readbuf_getchar_skip_whitespace(conn, pool, &c));
  return read_item(conn, pool, *item, c, 0);
}

svn_error_t *svn_ra_svn_skip_leading_garbage(svn_ra_svn_conn_t *conn,
                                             apr_pool_t *pool)
{
  return readbuf_skip_leading_garbage(conn, pool);
}

/* --- READING AND PARSING TUPLES --- */

/* Parse a tuple of svn_ra_svn_item_t *'s.  Advance *FMT to the end of the
 * tuple specification and advance AP by the corresponding arguments. */
static svn_error_t *vparse_tuple(const apr_array_header_t *items, apr_pool_t *pool,
                                 const char **fmt, va_list *ap)
{
  int count, nesting_level;
  svn_ra_svn_item_t *elt;

  for (count = 0; **fmt && count < items->nelts; (*fmt)++, count++)
    {
      /* '?' just means the tuple may stop; skip past it. */
      if (**fmt == '?')
        (*fmt)++;
      elt = &APR_ARRAY_IDX(items, count, svn_ra_svn_item_t);
      if (**fmt == 'n' && elt->kind == SVN_RA_SVN_NUMBER)
        *va_arg(*ap, apr_uint64_t *) = elt->u.number;
      else if (**fmt == 'r' && elt->kind == SVN_RA_SVN_NUMBER)
        *va_arg(*ap, svn_revnum_t *) = (svn_revnum_t) elt->u.number;
      else if (**fmt == 's' && elt->kind == SVN_RA_SVN_STRING)
        *va_arg(*ap, svn_string_t **) = elt->u.string;
      else if (**fmt == 'c' && elt->kind == SVN_RA_SVN_STRING)
        *va_arg(*ap, const char **) = elt->u.string->data;
      else if (**fmt == 'w' && elt->kind == SVN_RA_SVN_WORD)
        *va_arg(*ap, const char **) = elt->u.word;
      else if (**fmt == 'b' && elt->kind == SVN_RA_SVN_WORD)
        {
          if (strcmp(elt->u.word, "true") == 0)
            *va_arg(*ap, svn_boolean_t *) = TRUE;
          else if (strcmp(elt->u.word, "false") == 0)
            *va_arg(*ap, svn_boolean_t *) = FALSE;
          else
            break;
        }
      else if (**fmt == 'B' && elt->kind == SVN_RA_SVN_WORD)
        {
          if (strcmp(elt->u.word, "true") == 0)
            *va_arg(*ap, apr_uint64_t *) = TRUE;
          else if (strcmp(elt->u.word, "false") == 0)
            *va_arg(*ap, apr_uint64_t *) = FALSE;
          else
            break;
        }
      else if (**fmt == 'l' && elt->kind == SVN_RA_SVN_LIST)
        *va_arg(*ap, apr_array_header_t **) = elt->u.list;
      else if (**fmt == '(' && elt->kind == SVN_RA_SVN_LIST)
        {
          (*fmt)++;
          SVN_ERR(vparse_tuple(elt->u.list, pool, fmt, ap));
        }
      else if (**fmt == ')')
        return SVN_NO_ERROR;
      else
        break;
    }
  if (**fmt == '?')
    {
      nesting_level = 0;
      for (; **fmt; (*fmt)++)
        {
          switch (**fmt)
            {
            case '?':
              break;
            case 'r':
              *va_arg(*ap, svn_revnum_t *) = SVN_INVALID_REVNUM;
              break;
            case 's':
              *va_arg(*ap, svn_string_t **) = NULL;
              break;
            case 'c':
            case 'w':
              *va_arg(*ap, const char **) = NULL;
              break;
            case 'l':
              *va_arg(*ap, apr_array_header_t **) = NULL;
              break;
            case 'B':
            case 'n':
              *va_arg(*ap, apr_uint64_t *) = SVN_RA_SVN_UNSPECIFIED_NUMBER;
              break;
            case '(':
              nesting_level++;
              break;
            case ')':
              if (--nesting_level < 0)
                return SVN_NO_ERROR;
              break;
            default:
              SVN_ERR_MALFUNCTION();
            }
        }
    }
  if (**fmt && **fmt != ')')
    return svn_error_create(SVN_ERR_RA_SVN_MALFORMED_DATA, NULL,
                            _("Malformed network data"));
  return SVN_NO_ERROR;
}

svn_error_t *svn_ra_svn_parse_tuple(const apr_array_header_t *list,
                                    apr_pool_t *pool,
                                    const char *fmt, ...)
{
  svn_error_t *err;
  va_list ap;

  va_start(ap, fmt);
  err = vparse_tuple(list, pool, &fmt, &ap);
  va_end(ap);
  return err;
}

svn_error_t *svn_ra_svn_read_tuple(svn_ra_svn_conn_t *conn, apr_pool_t *pool,
                                   const char *fmt, ...)
{
  va_list ap;
  svn_ra_svn_item_t *item;
  svn_error_t *err;

  SVN_ERR(svn_ra_svn_read_item(conn, pool, &item));
  if (item->kind != SVN_RA_SVN_LIST)
    return svn_error_create(SVN_ERR_RA_SVN_MALFORMED_DATA, NULL,
                            _("Malformed network data"));
  va_start(ap, fmt);
  err = vparse_tuple(item->u.list, pool, &fmt, &ap);
  va_end(ap);
  return err;
}

svn_error_t *svn_ra_svn_parse_proplist(const apr_array_header_t *list,
                                       apr_pool_t *pool,
                                       apr_hash_t **props)
{
  char *name;
  svn_string_t *value;
  svn_ra_svn_item_t *elt;
  int i;

  *props = apr_hash_make(pool);
  for (i = 0; i < list->nelts; i++)
    {
      elt = &APR_ARRAY_IDX(list, i, svn_ra_svn_item_t);
      if (elt->kind != SVN_RA_SVN_LIST)
        return svn_error_create(SVN_ERR_RA_SVN_MALFORMED_DATA, NULL,
                                _("Proplist element not a list"));
      SVN_ERR(svn_ra_svn_parse_tuple(elt->u.list, pool, "cs", &name, &value));
      apr_hash_set(*props, name, APR_HASH_KEY_STRING, value);
    }

  return SVN_NO_ERROR;
}


/* --- READING AND WRITING COMMANDS AND RESPONSES --- */

svn_error_t *svn_ra_svn__locate_real_error_child(svn_error_t *err)
{
  svn_error_t *this_link;

  SVN_ERR_ASSERT(err);

  for (this_link = err;
       this_link && (this_link->apr_err == SVN_ERR_RA_SVN_CMD_ERR);
       this_link = this_link->child)
    ;

  SVN_ERR_ASSERT(this_link);
  return this_link;
}

svn_error_t *svn_ra_svn__handle_failure_status(const apr_array_header_t *params,
                                               apr_pool_t *pool)
{
  const char *message, *file;
  svn_error_t *err = NULL;
  svn_ra_svn_item_t *elt;
  int i;
  apr_uint64_t apr_err, line;
  apr_pool_t *subpool = svn_pool_create(pool);

  if (params->nelts == 0)
    return svn_error_create(SVN_ERR_RA_SVN_MALFORMED_DATA, NULL,
                            _("Empty error list"));

  /* Rebuild the error list from the end, to avoid reversing the order. */
  for (i = params->nelts - 1; i >= 0; i--)
    {
      svn_pool_clear(subpool);
      elt = &APR_ARRAY_IDX(params, i, svn_ra_svn_item_t);
      if (elt->kind != SVN_RA_SVN_LIST)
        return svn_error_create(SVN_ERR_RA_SVN_MALFORMED_DATA, NULL,
                                _("Malformed error list"));
      SVN_ERR(svn_ra_svn_parse_tuple(elt->u.list, subpool, "nccn", &apr_err,
                                      &message, &file, &line));
      /* The message field should have been optional, but we can't
         easily change that, so "" means a nonexistent message. */
      if (!*message)
        message = NULL;

      /* Skip over links in the error chain that were intended only to
         exist on the server (to wrap real errors intended for the
         client) but accidentally got included in the server's actual
         response. */
      if ((apr_status_t)apr_err != SVN_ERR_RA_SVN_CMD_ERR)
        {
          err = svn_error_create((apr_status_t)apr_err, err, message);
          err->file = apr_pstrdup(err->pool, file);
          err->line = (long)line;
        }
    }

  svn_pool_destroy(subpool);

  /* If we get here, then we failed to find a real error in the error
     chain that the server proported to be sending us.  That's bad. */
  if (! err)
    err = svn_error_create(SVN_ERR_RA_SVN_MALFORMED_DATA, NULL,
                           _("Malformed error list"));

  return err;
}

svn_error_t *svn_ra_svn_read_cmd_response(svn_ra_svn_conn_t *conn,
                                          apr_pool_t *pool,
                                          const char *fmt, ...)
{
  va_list ap;
  const char *status;
  apr_array_header_t *params;
  svn_error_t *err;

  SVN_ERR(svn_ra_svn_read_tuple(conn, pool, "wl", &status, &params));
  if (strcmp(status, "success") == 0)
    {
      va_start(ap, fmt);
      err = vparse_tuple(params, pool, &fmt, &ap);
      va_end(ap);
      return err;
    }
  else if (strcmp(status, "failure") == 0)
    {
      return svn_ra_svn__handle_failure_status(params, pool);
    }

  return svn_error_createf(SVN_ERR_RA_SVN_MALFORMED_DATA, NULL,
                           _("Unknown status '%s' in command response"),
                           status);
}

svn_error_t *svn_ra_svn_handle_commands2(svn_ra_svn_conn_t *conn,
                                         apr_pool_t *pool,
                                         const svn_ra_svn_cmd_entry_t *commands,
                                         void *baton,
                                         svn_boolean_t error_on_disconnect)
{
  apr_pool_t *subpool = svn_pool_create(pool);
  apr_pool_t *iterpool = svn_pool_create(subpool);
  const char *cmdname;
  const svn_ra_svn_cmd_entry_t *command;
  svn_error_t *err, *write_err;
  apr_array_header_t *params;
  apr_hash_t *cmd_hash = apr_hash_make(subpool);

  for (command = commands; command->cmdname; command++)
    apr_hash_set(cmd_hash, command->cmdname, APR_HASH_KEY_STRING, command);

  while (1)
    {
      svn_pool_clear(iterpool);
      err = svn_ra_svn_read_tuple(conn, iterpool, "wl", &cmdname, &params);
      if (err)
        {
          if (!error_on_disconnect
              && err->apr_err == SVN_ERR_RA_SVN_CONNECTION_CLOSED)
            {
              svn_error_clear(err);
              svn_pool_destroy(subpool);
              return SVN_NO_ERROR;
            }
          return err;
        }
      command = apr_hash_get(cmd_hash, cmdname, APR_HASH_KEY_STRING);

      if (command)
        err = (*command->handler)(conn, iterpool, params, baton);
      else
        {
          err = svn_error_createf(SVN_ERR_RA_SVN_UNKNOWN_CMD, NULL,
                                  _("Unknown command '%s'"), cmdname);
          err = svn_error_create(SVN_ERR_RA_SVN_CMD_ERR, err, NULL);
        }

      if (err && err->apr_err == SVN_ERR_RA_SVN_CMD_ERR)
        {
          write_err = svn_ra_svn_write_cmd_failure(
                          conn, iterpool,
                          svn_ra_svn__locate_real_error_child(err));
          svn_error_clear(err);
          if (write_err)
            return write_err;
        }
      else if (err)
        return err;

      if (command && command->terminate)
        break;
    }
  svn_pool_destroy(iterpool);
  svn_pool_destroy(subpool);
  return SVN_NO_ERROR;
}

svn_error_t *svn_ra_svn_handle_commands(svn_ra_svn_conn_t *conn,
                                        apr_pool_t *pool,
                                        const svn_ra_svn_cmd_entry_t *commands,
                                        void *baton)
{
  return svn_ra_svn_handle_commands2(conn, pool, commands, baton, TRUE);
}

svn_error_t *svn_ra_svn_write_cmd(svn_ra_svn_conn_t *conn, apr_pool_t *pool,
                                  const char *cmdname, const char *fmt, ...)
{
  va_list ap;
  svn_error_t *err;

  SVN_ERR(svn_ra_svn_start_list(conn, pool));
  SVN_ERR(svn_ra_svn_write_word(conn, pool, cmdname));
  va_start(ap, fmt);
  err = vwrite_tuple(conn, pool, fmt, ap);
  va_end(ap);
  if (err)
    return err;
  SVN_ERR(svn_ra_svn_end_list(conn, pool));
  return SVN_NO_ERROR;
}

svn_error_t *svn_ra_svn_write_cmd_response(svn_ra_svn_conn_t *conn,
                                           apr_pool_t *pool,
                                           const char *fmt, ...)
{
  va_list ap;
  svn_error_t *err;

  SVN_ERR(svn_ra_svn_start_list(conn, pool));
  SVN_ERR(svn_ra_svn_write_word(conn, pool, "success"));
  va_start(ap, fmt);
  err = vwrite_tuple(conn, pool, fmt, ap);
  va_end(ap);
  if (err)
    return err;
  SVN_ERR(svn_ra_svn_end_list(conn, pool));
  return SVN_NO_ERROR;
}

svn_error_t *svn_ra_svn_write_cmd_failure(svn_ra_svn_conn_t *conn,
                                          apr_pool_t *pool, svn_error_t *err)
{
  char buffer[128];
  SVN_ERR(svn_ra_svn_start_list(conn, pool));
  SVN_ERR(svn_ra_svn_write_word(conn, pool, "failure"));
  SVN_ERR(svn_ra_svn_start_list(conn, pool));
  for (; err; err = err->child)
    {
      const char *msg = svn_err_best_message(err, buffer, sizeof(buffer));

      /* The message string should have been optional, but we can't
         easily change that, so marshal nonexistent messages as "". */
      SVN_ERR(svn_ra_svn_write_tuple(conn, pool, "nccn",
                                     (apr_uint64_t) err->apr_err,
                                     msg ? msg : "",
                                     err->file ? err->file : "",
                                     (apr_uint64_t) err->line));
    }
  SVN_ERR(svn_ra_svn_end_list(conn, pool));
  SVN_ERR(svn_ra_svn_end_list(conn, pool));
  return SVN_NO_ERROR;
}
