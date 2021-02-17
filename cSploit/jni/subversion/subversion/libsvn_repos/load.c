/* load.c --- parsing a 'dumpfile'-formatted stream.
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


#include "svn_private_config.h"
#include "svn_pools.h"
#include "svn_error.h"
#include "svn_fs.h"
#include "svn_repos.h"
#include "svn_string.h"
#include "svn_path.h"
#include "svn_props.h"
#include "repos.h"
#include "svn_private_config.h"
#include "svn_mergeinfo.h"
#include "svn_checksum.h"
#include "svn_subst.h"
#include "svn_ctype.h"

#include <apr_lib.h>

#include "private/svn_dep_compat.h"
#include "private/svn_mergeinfo_private.h"

/*----------------------------------------------------------------------*/

/** The parser and related helper funcs **/


static svn_error_t *
stream_ran_dry(void)
{
  return svn_error_create(SVN_ERR_INCOMPLETE_DATA, NULL,
                          _("Premature end of content data in dumpstream"));
}

static svn_error_t *
stream_malformed(void)
{
  return svn_error_create(SVN_ERR_STREAM_MALFORMED_DATA, NULL,
                          _("Dumpstream data appears to be malformed"));
}

/* Allocate a new hash *HEADERS in POOL, and read a series of
   RFC822-style headers from STREAM.  Duplicate each header's name and
   value into POOL and store in hash as a const char * ==> const char *.

   The headers are assumed to be terminated by a single blank line,
   which will be permanently sucked from the stream and tossed.

   If the caller has already read in the first header line, it should
   be passed in as FIRST_HEADER.  If not, pass NULL instead.
 */
static svn_error_t *
read_header_block(svn_stream_t *stream,
                  svn_stringbuf_t *first_header,
                  apr_hash_t **headers,
                  apr_pool_t *pool)
{
  *headers = apr_hash_make(pool);

  while (1)
    {
      svn_stringbuf_t *header_str;
      const char *name, *value;
      svn_boolean_t eof;
      apr_size_t i = 0;

      if (first_header != NULL)
        {
          header_str = first_header;
          first_header = NULL;  /* so we never visit this block again. */
          eof = FALSE;
        }

      else
        /* Read the next line into a stringbuf. */
        SVN_ERR(svn_stream_readline(stream, &header_str, "\n", &eof, pool));

      if (svn_stringbuf_isempty(header_str))
        break;    /* end of header block */
      else if (eof)
        return stream_ran_dry();

      /* Find the next colon in the stringbuf. */
      while (header_str->data[i] != ':')
        {
          if (header_str->data[i] == '\0')
            return svn_error_createf(SVN_ERR_STREAM_MALFORMED_DATA, NULL,
                                     _("Dump stream contains a malformed "
                                       "header (with no ':') at '%.20s'"),
                                     header_str->data);
          i++;
        }
      /* Create a 'name' string and point to it. */
      header_str->data[i] = '\0';
      name = header_str->data;

      /* Skip over the NULL byte and the space following it.  */
      i += 2;
      if (i > header_str->len)
        return svn_error_createf(SVN_ERR_STREAM_MALFORMED_DATA, NULL,
                                 _("Dump stream contains a malformed "
                                   "header (with no value) at '%.20s'"),
                                 header_str->data);

      /* Point to the 'value' string. */
      value = header_str->data + i;

      /* Store name/value in hash. */
      apr_hash_set(*headers, name, APR_HASH_KEY_STRING, value);
    }

  return SVN_NO_ERROR;
}


/* Set *PBUF to a string of length LEN, allocated in POOL, read from STREAM.
   Also read a newline from STREAM and increase *ACTUAL_LEN by the total
   number of bytes read from STREAM.  */
static svn_error_t *
read_key_or_val(char **pbuf,
                svn_filesize_t *actual_length,
                svn_stream_t *stream,
                apr_size_t len,
                apr_pool_t *pool)
{
  char *buf = apr_pcalloc(pool, len + 1);
  apr_size_t numread;
  char c;

  numread = len;
  SVN_ERR(svn_stream_read(stream, buf, &numread));
  *actual_length += numread;
  if (numread != len)
    return svn_error_trace(stream_ran_dry());
  buf[len] = '\0';

  /* Suck up extra newline after key data */
  numread = 1;
  SVN_ERR(svn_stream_read(stream, &c, &numread));
  *actual_length += numread;
  if (numread != 1)
    return svn_error_trace(stream_ran_dry());
  if (c != '\n')
    return svn_error_trace(stream_malformed());

  *pbuf = buf;
  return SVN_NO_ERROR;
}


/* Read CONTENT_LENGTH bytes from STREAM, parsing the bytes as an
   encoded Subversion properties hash, and making multiple calls to
   PARSE_FNS->set_*_property on RECORD_BATON (depending on the value
   of IS_NODE.)

   Set *ACTUAL_LENGTH to the number of bytes consumed from STREAM.
   If an error is returned, the value of *ACTUAL_LENGTH is undefined.

   Use POOL for all allocations.  */
static svn_error_t *
parse_property_block(svn_stream_t *stream,
                     svn_filesize_t content_length,
                     const svn_repos_parse_fns2_t *parse_fns,
                     void *record_baton,
                     void *parse_baton,
                     svn_boolean_t is_node,
                     svn_filesize_t *actual_length,
                     apr_pool_t *pool)
{
  svn_stringbuf_t *strbuf;
  apr_pool_t *proppool = svn_pool_create(pool);

  *actual_length = 0;
  while (content_length != *actual_length)
    {
      char *buf;  /* a pointer into the stringbuf's data */
      svn_boolean_t eof;

      svn_pool_clear(proppool);

      /* Read a key length line.  (Actually, it might be PROPS_END). */
      SVN_ERR(svn_stream_readline(stream, &strbuf, "\n", &eof, proppool));

      if (eof)
        {
          /* We could just use stream_ran_dry() or stream_malformed(),
             but better to give a non-generic property block error. */
          return svn_error_create
            (SVN_ERR_STREAM_MALFORMED_DATA, NULL,
             _("Incomplete or unterminated property block"));
        }

      *actual_length += (strbuf->len + 1); /* +1 because we read a \n too. */
      buf = strbuf->data;

      if (! strcmp(buf, "PROPS-END"))
        break; /* no more properties. */

      else if ((buf[0] == 'K') && (buf[1] == ' '))
        {
          char *keybuf;
          apr_uint64_t len;

          SVN_ERR(svn_cstring_strtoui64(&len, buf + 2, 0, APR_SIZE_MAX, 10));
          SVN_ERR(read_key_or_val(&keybuf, actual_length,
                                  stream, (apr_size_t)len, proppool));

          /* Read a val length line */
          SVN_ERR(svn_stream_readline(stream, &strbuf, "\n", &eof, proppool));
          if (eof)
            return stream_ran_dry();

          *actual_length += (strbuf->len + 1); /* +1 because we read \n too */
          buf = strbuf->data;

          if ((buf[0] == 'V') && (buf[1] == ' '))
            {
              svn_string_t propstring;
              char *valbuf;
              apr_int64_t val;

              SVN_ERR(svn_cstring_atoi64(&val, buf + 2));
              propstring.len = (apr_size_t)val;
              SVN_ERR(read_key_or_val(&valbuf, actual_length,
                                      stream, propstring.len, proppool));
              propstring.data = valbuf;

              /* Now, send the property pair to the vtable! */
              if (is_node)
                {
                  SVN_ERR(parse_fns->set_node_property(record_baton,
                                                       keybuf,
                                                       &propstring));
                }
              else
                {
                  SVN_ERR(parse_fns->set_revision_property(record_baton,
                                                           keybuf,
                                                           &propstring));
                }
            }
          else
            return stream_malformed(); /* didn't find expected 'V' line */
        }
      else if ((buf[0] == 'D') && (buf[1] == ' '))
        {
          char *keybuf;
          apr_uint64_t len;

          SVN_ERR(svn_cstring_strtoui64(&len, buf + 2, 0, APR_SIZE_MAX, 10));
          SVN_ERR(read_key_or_val(&keybuf, actual_length,
                                  stream, (apr_size_t)len, proppool));

          /* We don't expect these in revision properties, and if we see
             one when we don't have a delete_node_property callback,
             then we're seeing a v3 feature in a v2 dump. */
          if (!is_node || !parse_fns->delete_node_property)
            return stream_malformed();

          SVN_ERR(parse_fns->delete_node_property(record_baton, keybuf));
        }
      else
        return stream_malformed(); /* didn't find expected 'K' line */

    } /* while (1) */

  svn_pool_destroy(proppool);
  return SVN_NO_ERROR;
}


/* Read CONTENT_LENGTH bytes from STREAM, and use
   PARSE_FNS->set_fulltext to push those bytes as replace fulltext for
   a node.  Use BUFFER/BUFLEN to push the fulltext in "chunks".

   Use POOL for all allocations.  */
static svn_error_t *
parse_text_block(svn_stream_t *stream,
                 svn_filesize_t content_length,
                 svn_boolean_t is_delta,
                 const svn_repos_parse_fns2_t *parse_fns,
                 void *record_baton,
                 char *buffer,
                 apr_size_t buflen,
                 apr_pool_t *pool)
{
  svn_stream_t *text_stream = NULL;
  apr_size_t num_to_read, rlen, wlen;

  if (is_delta)
    {
      svn_txdelta_window_handler_t wh;
      void *whb;

      SVN_ERR(parse_fns->apply_textdelta(&wh, &whb, record_baton));
      if (wh)
        text_stream = svn_txdelta_parse_svndiff(wh, whb, TRUE, pool);
    }
  else
    {
      /* Get a stream to which we can push the data. */
      SVN_ERR(parse_fns->set_fulltext(&text_stream, record_baton));
    }

  /* If there are no contents to read, just write an empty buffer
     through our callback. */
  if (content_length == 0)
    {
      wlen = 0;
      if (text_stream)
        SVN_ERR(svn_stream_write(text_stream, "", &wlen));
    }

  /* Regardless of whether or not we have a sink for our data, we
     need to read it. */
  while (content_length)
    {
      if (content_length >= buflen)
        rlen = buflen;
      else
        rlen = (apr_size_t) content_length;

      num_to_read = rlen;
      SVN_ERR(svn_stream_read(stream, buffer, &rlen));
      content_length -= rlen;
      if (rlen != num_to_read)
        return stream_ran_dry();

      if (text_stream)
        {
          /* write however many bytes you read. */
          wlen = rlen;
          SVN_ERR(svn_stream_write(text_stream, buffer, &wlen));
          if (wlen != rlen)
            {
              /* Uh oh, didn't write as many bytes as we read. */
              return svn_error_create(SVN_ERR_STREAM_UNEXPECTED_EOF, NULL,
                                      _("Unexpected EOF writing contents"));
            }
        }
    }

  /* If we opened a stream, we must close it. */
  if (text_stream)
    SVN_ERR(svn_stream_close(text_stream));

  return SVN_NO_ERROR;
}



/* Parse VERSIONSTRING and verify that we support the dumpfile format
   version number, setting *VERSION appropriately. */
static svn_error_t *
parse_format_version(const char *versionstring, int *version)
{
  static const int magic_len = sizeof(SVN_REPOS_DUMPFILE_MAGIC_HEADER) - 1;
  const char *p = strchr(versionstring, ':');
  int value;

  if (p == NULL
      || p != (versionstring + magic_len)
      || strncmp(versionstring,
                 SVN_REPOS_DUMPFILE_MAGIC_HEADER,
                 magic_len))
    return svn_error_createf(SVN_ERR_STREAM_MALFORMED_DATA, NULL,
                             _("Malformed dumpfile header '%s'"),
                             versionstring);

  SVN_ERR(svn_cstring_atoi(&value, p + 1));

  if (value > SVN_REPOS_DUMPFILE_FORMAT_VERSION)
    return svn_error_createf(SVN_ERR_STREAM_MALFORMED_DATA, NULL,
                             _("Unsupported dumpfile version: %d"),
                             value);

  *version = value;
  return SVN_NO_ERROR;
}



/*----------------------------------------------------------------------*/

/** The public routines **/

svn_error_t *
svn_repos_parse_dumpstream2(svn_stream_t *stream,
                            const svn_repos_parse_fns2_t *parse_fns,
                            void *parse_baton,
                            svn_cancel_func_t cancel_func,
                            void *cancel_baton,
                            apr_pool_t *pool)
{
  svn_boolean_t eof;
  svn_stringbuf_t *linebuf;
  void *rev_baton = NULL;
  char *buffer = apr_palloc(pool, SVN__STREAM_CHUNK_SIZE);
  apr_size_t buflen = SVN__STREAM_CHUNK_SIZE;
  apr_pool_t *linepool = svn_pool_create(pool);
  apr_pool_t *revpool = svn_pool_create(pool);
  apr_pool_t *nodepool = svn_pool_create(pool);
  int version;

  SVN_ERR(svn_stream_readline(stream, &linebuf, "\n", &eof, linepool));
  if (eof)
    return stream_ran_dry();

  /* The first two lines of the stream are the dumpfile-format version
     number, and a blank line. */
  SVN_ERR(parse_format_version(linebuf->data, &version));

  /* If we were called from svn_repos_parse_dumpstream(), the
     callbacks to handle delta contents will be NULL, so we have to
     reject dumpfiles with the current version. */
  if (version == SVN_REPOS_DUMPFILE_FORMAT_VERSION
      && (!parse_fns->delete_node_property || !parse_fns->apply_textdelta))
    return svn_error_createf(SVN_ERR_STREAM_MALFORMED_DATA, NULL,
                             _("Unsupported dumpfile version: %d"), version);

  /* A dumpfile "record" is defined to be a header-block of
     rfc822-style headers, possibly followed by a content-block.

       - A header-block is always terminated by a single blank line (\n\n)

       - We know whether the record has a content-block by looking for
         a 'Content-length:' header.  The content-block will always be
         of a specific length, plus an extra newline.

     Once a record is fully sucked from the stream, an indeterminate
     number of blank lines (or lines that begin with whitespace) may
     follow before the next record (or the end of the stream.)
  */

  while (1)
    {
      apr_hash_t *headers;
      void *node_baton;
      svn_boolean_t found_node = FALSE;
      svn_boolean_t old_v1_with_cl = FALSE;
      const char *content_length;
      const char *prop_cl;
      const char *text_cl;
      const char *value;
      svn_filesize_t actual_prop_length;

      /* Clear our per-line pool. */
      svn_pool_clear(linepool);

      /* Check for cancellation. */
      if (cancel_func)
        SVN_ERR(cancel_func(cancel_baton));

      /* Keep reading blank lines until we discover a new record, or until
         the stream runs out. */
      SVN_ERR(svn_stream_readline(stream, &linebuf, "\n", &eof, linepool));

      if (eof)
        {
          if (svn_stringbuf_isempty(linebuf))
            break;   /* end of stream, go home. */
          else
            return stream_ran_dry();
        }

      if ((linebuf->len == 0) || (svn_ctype_isspace(linebuf->data[0])))
        continue; /* empty line ... loop */

      /*** Found the beginning of a new record. ***/

      /* The last line we read better be a header of some sort.
         Read the whole header-block into a hash. */
      SVN_ERR(read_header_block(stream, linebuf, &headers, linepool));

      /*** Handle the various header blocks. ***/

      /* Is this a revision record? */
      if (apr_hash_get(headers, SVN_REPOS_DUMPFILE_REVISION_NUMBER,
                       APR_HASH_KEY_STRING))
        {
          /* If we already have a rev_baton open, we need to close it
             and clear the per-revision subpool. */
          if (rev_baton != NULL)
            {
              SVN_ERR(parse_fns->close_revision(rev_baton));
              svn_pool_clear(revpool);
            }

          SVN_ERR(parse_fns->new_revision_record(&rev_baton,
                                                 headers, parse_baton,
                                                 revpool));
        }
      /* Or is this, perhaps, a node record? */
      else if (apr_hash_get(headers, SVN_REPOS_DUMPFILE_NODE_PATH,
                            APR_HASH_KEY_STRING))
        {
          SVN_ERR(parse_fns->new_node_record(&node_baton,
                                             headers,
                                             rev_baton,
                                             nodepool));
          found_node = TRUE;
        }
      /* Or is this the repos UUID? */
      else if ((value = apr_hash_get(headers, SVN_REPOS_DUMPFILE_UUID,
                                     APR_HASH_KEY_STRING)))
        {
          SVN_ERR(parse_fns->uuid_record(value, parse_baton, pool));
        }
      /* Or perhaps a dumpfile format? */
      else if ((value = apr_hash_get(headers,
                                     SVN_REPOS_DUMPFILE_MAGIC_HEADER,
                                     APR_HASH_KEY_STRING)))
        {
          /* ### someday, switch modes of operation here. */
          SVN_ERR(svn_cstring_atoi(&version, value));
        }
      /* Or is this bogosity?! */
      else
        {
          /* What the heck is this record?!? */
          return svn_error_create(SVN_ERR_STREAM_MALFORMED_DATA, NULL,
                                  _("Unrecognized record type in stream"));
        }

      /* Need 3 values below to determine v1 dump type

         Old (pre 0.14?) v1 dumps don't have Prop-content-length
         and Text-content-length fields, but always have a properties
         block in a block with Content-Length > 0 */

      content_length = apr_hash_get(headers,
                                    SVN_REPOS_DUMPFILE_CONTENT_LENGTH,
                                    APR_HASH_KEY_STRING);
      prop_cl = apr_hash_get(headers,
                             SVN_REPOS_DUMPFILE_PROP_CONTENT_LENGTH,
                             APR_HASH_KEY_STRING);
      text_cl = apr_hash_get(headers,
                             SVN_REPOS_DUMPFILE_TEXT_CONTENT_LENGTH,
                             APR_HASH_KEY_STRING);
      old_v1_with_cl =
        version == 1 && content_length && ! prop_cl && ! text_cl;

      /* Is there a props content-block to parse? */
      if (prop_cl || old_v1_with_cl)
        {
          const char *delta = apr_hash_get(headers,
                                           SVN_REPOS_DUMPFILE_PROP_DELTA,
                                           APR_HASH_KEY_STRING);
          svn_boolean_t is_delta = (delta && strcmp(delta, "true") == 0);

          /* First, remove all node properties, unless this is a delta
             property block. */
          if (found_node && !is_delta)
            SVN_ERR(parse_fns->remove_node_props(node_baton));

          SVN_ERR(parse_property_block
                  (stream,
                   svn__atoui64(prop_cl ? prop_cl : content_length),
                   parse_fns,
                   found_node ? node_baton : rev_baton,
                   parse_baton,
                   found_node,
                   &actual_prop_length,
                   found_node ? nodepool : revpool));
        }

      /* Is there a text content-block to parse? */
      if (text_cl)
        {
          const char *delta = apr_hash_get(headers,
                                           SVN_REPOS_DUMPFILE_TEXT_DELTA,
                                           APR_HASH_KEY_STRING);
          svn_boolean_t is_delta = (delta && strcmp(delta, "true") == 0);

          SVN_ERR(parse_text_block(stream,
                                   svn__atoui64(text_cl),
                                   is_delta,
                                   parse_fns,
                                   found_node ? node_baton : rev_baton,
                                   buffer,
                                   buflen,
                                   found_node ? nodepool : revpool));
        }
      else if (old_v1_with_cl)
        {
          /* An old-v1 block with a Content-length might have a text block.
             If the property block did not consume all the bytes of the
             Content-length, then it clearly does have a text block.
             If not, then we must deduce whether we have an *empty* text
             block or an *absent* text block.  The rules are:
             - "Node-kind: file" blocks have an empty (i.e. present, but
               zero-length) text block, since they represent a file
               modification.  Note that file-copied-text-unmodified blocks
               have no Content-length - even if they should have contained
               a modified property block, the pre-0.14 dumper forgets to
               dump the modified properties.
             - If it is not a file node, then it is a revision or directory,
               and so has an absent text block.
          */
          const char *node_kind;
          svn_filesize_t cl_value = svn__atoui64(content_length)
                                    - actual_prop_length;

          if (cl_value ||
              ((node_kind = apr_hash_get(headers,
                                         SVN_REPOS_DUMPFILE_NODE_KIND,
                                         APR_HASH_KEY_STRING))
               && strcmp(node_kind, "file") == 0)
             )
            SVN_ERR(parse_text_block(stream,
                                     cl_value,
                                     FALSE,
                                     parse_fns,
                                     found_node ? node_baton : rev_baton,
                                     buffer,
                                     buflen,
                                     found_node ? nodepool : revpool));
        }

      /* if we have a content-length header, did we read all of it?
         in case of an old v1, we *always* read all of it, because
         text-content-length == content-length - prop-content-length
      */
      if (content_length && ! old_v1_with_cl)
        {
          apr_size_t rlen, num_to_read;
          svn_filesize_t remaining =
            svn__atoui64(content_length) -
            (prop_cl ? svn__atoui64(prop_cl) : 0) -
            (text_cl ? svn__atoui64(text_cl) : 0);


          if (remaining < 0)
            return svn_error_create(SVN_ERR_STREAM_MALFORMED_DATA, NULL,
                                    _("Sum of subblock sizes larger than "
                                      "total block content length"));

          /* Consume remaining bytes in this content block */
          while (remaining > 0)
            {
              if (remaining >= buflen)
                rlen = buflen;
              else
                rlen = (apr_size_t) remaining;

              num_to_read = rlen;
              SVN_ERR(svn_stream_read(stream, buffer, &rlen));
              remaining -= rlen;
              if (rlen != num_to_read)
                return stream_ran_dry();
            }
        }

      /* If we just finished processing a node record, we need to
         close the node record and clear the per-node subpool. */
      if (found_node)
        {
          SVN_ERR(parse_fns->close_node(node_baton));
          svn_pool_clear(nodepool);
        }

      /*** End of processing for one record. ***/

    } /* end of stream */

  /* Close out whatever revision we're in. */
  if (rev_baton != NULL)
    SVN_ERR(parse_fns->close_revision(rev_baton));

  svn_pool_destroy(linepool);
  svn_pool_destroy(revpool);
  svn_pool_destroy(nodepool);
  return SVN_NO_ERROR;
}
