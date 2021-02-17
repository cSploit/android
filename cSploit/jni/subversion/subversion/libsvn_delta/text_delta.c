/*
 * text-delta.c -- Internal text delta representation
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
#include <string.h>

#include <apr_general.h>        /* for APR_INLINE */
#include <apr_md5.h>            /* for, um...MD5 stuff */

#include "svn_delta.h"
#include "svn_io.h"
#include "svn_pools.h"
#include "svn_checksum.h"

#include "delta.h"


/* Text delta stream descriptor. */

struct svn_txdelta_stream_t {
  /* Copied from parameters to svn_txdelta_stream_create. */
  void *baton;
  svn_txdelta_next_window_fn_t next_window;
  svn_txdelta_md5_digest_fn_t md5_digest;
};

/* Delta stream baton. */
struct txdelta_baton {
  /* These are copied from parameters passed to svn_txdelta. */
  svn_stream_t *source;
  svn_stream_t *target;

  /* Private data */
  svn_boolean_t more_source;    /* FALSE if source stream hit EOF. */
  svn_boolean_t more;           /* TRUE if there are more data in the pool. */
  svn_filesize_t pos;           /* Offset of next read in source file. */
  char *buf;                    /* Buffer for input data. */

  svn_checksum_ctx_t *context;  /* Context for computing the checksum. */
  svn_checksum_t *checksum;     /* If non-NULL, the checksum of TARGET. */

  apr_pool_t *result_pool;      /* For results (e.g. checksum) */
};


/* Target-push stream descriptor. */

struct tpush_baton {
  /* These are copied from parameters passed to svn_txdelta_target_push. */
  svn_stream_t *source;
  svn_txdelta_window_handler_t wh;
  void *whb;
  apr_pool_t *pool;

  /* Private data */
  char *buf;
  svn_filesize_t source_offset;
  apr_size_t source_len;
  svn_boolean_t source_done;
  apr_size_t target_len;
};


/* Text delta applicator.  */

struct apply_baton {
  /* These are copied from parameters passed to svn_txdelta_apply.  */
  svn_stream_t *source;
  svn_stream_t *target;

  /* Private data.  Between calls, SBUF contains the data from the
   * last window's source view, as specified by SBUF_OFFSET and
   * SBUF_LEN.  The contents of TBUF are not interesting between
   * calls.  */
  apr_pool_t *pool;             /* Pool to allocate data from */
  char *sbuf;                   /* Source buffer */
  apr_size_t sbuf_size;         /* Allocated source buffer space */
  svn_filesize_t sbuf_offset;   /* Offset of SBUF data in source stream */
  apr_size_t sbuf_len;          /* Length of SBUF data */
  char *tbuf;                   /* Target buffer */
  apr_size_t tbuf_size;         /* Allocated target buffer space */

  apr_md5_ctx_t md5_context;    /* Leads to result_digest below. */
  unsigned char *result_digest; /* MD5 digest of resultant fulltext;
                                   must point to at least APR_MD5_DIGESTSIZE
                                   bytes of storage. */

  const char *error_info;       /* Optional extra info for error returns. */
};



svn_txdelta_window_t *
svn_txdelta__make_window(const svn_txdelta__ops_baton_t *build_baton,
                         apr_pool_t *pool)
{
  svn_txdelta_window_t *window;
  svn_string_t *new_data = apr_palloc(pool, sizeof(*new_data));

  window = apr_palloc(pool, sizeof(*window));
  window->sview_offset = 0;
  window->sview_len = 0;
  window->tview_len = 0;

  window->num_ops = build_baton->num_ops;
  window->src_ops = build_baton->src_ops;
  window->ops = build_baton->ops;

  /* just copy the fields over, rather than alloc/copying into a whole new
     svn_string_t structure. */
  /* ### would be much nicer if window->new_data were not a ptr... */
  new_data->data = build_baton->new_data->data;
  new_data->len = build_baton->new_data->len;
  window->new_data = new_data;

  return window;
}


/* Compute and return a delta window using the xdelta algorithm on
   DATA, which contains SOURCE_LEN bytes of source data and TARGET_LEN
   bytes of target data.  SOURCE_OFFSET gives the offset of the source
   data, and is simply copied into the window's sview_offset field. */
static svn_txdelta_window_t *
compute_window(const char *data, apr_size_t source_len, apr_size_t target_len,
               svn_filesize_t source_offset, apr_pool_t *pool)
{
  svn_txdelta__ops_baton_t build_baton = { 0 };
  svn_txdelta_window_t *window;

  /* Compute the delta operations. */
  build_baton.new_data = svn_stringbuf_create("", pool);

  if (source_len == 0)
    svn_txdelta__insert_op(&build_baton, svn_txdelta_new, 0, target_len, data,
                           pool);
  else
    svn_txdelta__xdelta(&build_baton, data, source_len, target_len, pool);

  /* Create and return the delta window. */
  window = svn_txdelta__make_window(&build_baton, pool);
  window->sview_offset = source_offset;
  window->sview_len = source_len;
  window->tview_len = target_len;
  return window;
}



svn_txdelta_window_t *
svn_txdelta_window_dup(const svn_txdelta_window_t *window,
                       apr_pool_t *pool)
{
  svn_txdelta__ops_baton_t build_baton = { 0 };
  svn_txdelta_window_t *new_window;
  const apr_size_t ops_size = (window->num_ops * sizeof(*build_baton.ops));

  build_baton.num_ops = window->num_ops;
  build_baton.src_ops = window->src_ops;
  build_baton.ops_size = window->num_ops;
  build_baton.ops = apr_palloc(pool, ops_size);
  memcpy(build_baton.ops, window->ops, ops_size);
  build_baton.new_data =
    svn_stringbuf_create_from_string(window->new_data, pool);

  new_window = svn_txdelta__make_window(&build_baton, pool);
  new_window->sview_offset = window->sview_offset;
  new_window->sview_len = window->sview_len;
  new_window->tview_len = window->tview_len;
  return new_window;
}

/* This is a private interlibrary compatibility wrapper. */
svn_txdelta_window_t *
svn_txdelta__copy_window(const svn_txdelta_window_t *window,
                         apr_pool_t *pool);
svn_txdelta_window_t *
svn_txdelta__copy_window(const svn_txdelta_window_t *window,
                         apr_pool_t *pool)
{
  return svn_txdelta_window_dup(window, pool);
}


/* Insert a delta op into a delta window. */

void
svn_txdelta__insert_op(svn_txdelta__ops_baton_t *build_baton,
                       enum svn_delta_action opcode,
                       apr_size_t offset,
                       apr_size_t length,
                       const char *new_data,
                       apr_pool_t *pool)
{
  svn_txdelta_op_t *op;

  /* Check if this op can be merged with the previous op. The delta
     combiner sometimes generates such ops, and this is the obvious
     place to make the check. */
  if (build_baton->num_ops > 0)
    {
      op = &build_baton->ops[build_baton->num_ops - 1];
      if (op->action_code == opcode
          && (opcode == svn_txdelta_new
              || op->offset + op->length == offset))
        {
          op->length += length;
          if (opcode == svn_txdelta_new)
            svn_stringbuf_appendbytes(build_baton->new_data,
                                      new_data, length);
          return;
        }
    }

  /* Create space for the new op. */
  if (build_baton->num_ops == build_baton->ops_size)
    {
      svn_txdelta_op_t *const old_ops = build_baton->ops;
      int const new_ops_size = (build_baton->ops_size == 0
                                ? 16 : 2 * build_baton->ops_size);
      build_baton->ops =
        apr_palloc(pool, new_ops_size * sizeof(*build_baton->ops));

      /* Copy any existing ops into the new array */
      if (old_ops)
        memcpy(build_baton->ops, old_ops,
               build_baton->ops_size * sizeof(*build_baton->ops));
      build_baton->ops_size = new_ops_size;
    }

  /* Insert the op. svn_delta_source and svn_delta_target are
     just inserted. For svn_delta_new, the new data must be
     copied into the window. */
  op = &build_baton->ops[build_baton->num_ops];
  switch (opcode)
    {
    case svn_txdelta_source:
      ++build_baton->src_ops;
      /*** FALLTHRU ***/
    case svn_txdelta_target:
      op->action_code = opcode;
      op->offset = offset;
      op->length = length;
      break;
    case svn_txdelta_new:
      op->action_code = opcode;
      op->offset = build_baton->new_data->len;
      op->length = length;
      svn_stringbuf_appendbytes(build_baton->new_data, new_data, length);
      break;
    default:
      assert(!"unknown delta op.");
    }

  ++build_baton->num_ops;
}



/* Generic delta stream functions. */

svn_txdelta_stream_t *
svn_txdelta_stream_create(void *baton,
                          svn_txdelta_next_window_fn_t next_window,
                          svn_txdelta_md5_digest_fn_t md5_digest,
                          apr_pool_t *pool)
{
  svn_txdelta_stream_t *stream = apr_palloc(pool, sizeof(*stream));

  stream->baton = baton;
  stream->next_window = next_window;
  stream->md5_digest = md5_digest;

  return stream;
}

svn_error_t *
svn_txdelta_next_window(svn_txdelta_window_t **window,
                        svn_txdelta_stream_t *stream,
                        apr_pool_t *pool)
{
  return stream->next_window(window, stream->baton, pool);
}

const unsigned char *
svn_txdelta_md5_digest(svn_txdelta_stream_t *stream)
{
  return stream->md5_digest(stream->baton);
}



static svn_error_t *
txdelta_next_window(svn_txdelta_window_t **window,
                    void *baton,
                    apr_pool_t *pool)
{
  struct txdelta_baton *b = baton;
  apr_size_t source_len = SVN_DELTA_WINDOW_SIZE;
  apr_size_t target_len = SVN_DELTA_WINDOW_SIZE;

  /* Read the source stream. */
  if (b->more_source)
    {
      SVN_ERR(svn_stream_read(b->source, b->buf, &source_len));
      b->more_source = (source_len == SVN_DELTA_WINDOW_SIZE);
    }
  else
    source_len = 0;

  /* Read the target stream. */
  SVN_ERR(svn_stream_read(b->target, b->buf + source_len, &target_len));
  b->pos += source_len;

  if (target_len == 0)
    {
      /* No target data?  We're done; return the final window. */
      if (b->context != NULL)
        SVN_ERR(svn_checksum_final(&b->checksum, b->context, b->result_pool));

      *window = NULL;
      b->more = FALSE;
      return SVN_NO_ERROR;
    }
  else if (b->context != NULL)
    SVN_ERR(svn_checksum_update(b->context, b->buf + source_len, target_len));

  *window = compute_window(b->buf, source_len, target_len,
                           b->pos - source_len, pool);

  /* That's it. */
  return SVN_NO_ERROR;
}


static const unsigned char *
txdelta_md5_digest(void *baton)
{
  struct txdelta_baton *b = baton;
  /* If there are more windows for this stream, the digest has not yet
     been calculated.  */
  if (b->more)
    return NULL;

  /* The checksum should be there. */
  return b->checksum->digest;
}


svn_error_t *
svn_txdelta_run(svn_stream_t *source,
                svn_stream_t *target,
                svn_txdelta_window_handler_t handler,
                void *handler_baton,
                svn_checksum_kind_t checksum_kind,
                svn_checksum_t **checksum,
                svn_cancel_func_t cancel_func,
                void *cancel_baton,
                apr_pool_t *result_pool,
                apr_pool_t *scratch_pool)
{
  apr_pool_t *iterpool = svn_pool_create(scratch_pool);
  struct txdelta_baton tb = { 0 };
  svn_txdelta_window_t *window;

  tb.source = source;
  tb.target = target;
  tb.more_source = TRUE;
  tb.more = TRUE;
  tb.pos = 0;
  tb.buf = apr_palloc(scratch_pool, 2 * SVN_DELTA_WINDOW_SIZE);
  tb.result_pool = result_pool;

  if (checksum != NULL)
    tb.context = svn_checksum_ctx_create(checksum_kind, scratch_pool);

  do
    {
      /* free the window (if any) */
      svn_pool_clear(iterpool);

      /* read in a single delta window */
      SVN_ERR(txdelta_next_window(&window, &tb, iterpool));

      /* shove it at the handler */
      SVN_ERR((*handler)(window, handler_baton));

      if (cancel_func)
        SVN_ERR(cancel_func(cancel_baton));
    }
  while (window != NULL);

  svn_pool_destroy(iterpool);

  if (checksum != NULL)
    *checksum = tb.checksum;  /* should be there! */

  return SVN_NO_ERROR;
}


void
svn_txdelta(svn_txdelta_stream_t **stream,
            svn_stream_t *source,
            svn_stream_t *target,
            apr_pool_t *pool)
{
  struct txdelta_baton *b = apr_pcalloc(pool, sizeof(*b));

  b->source = source;
  b->target = target;
  b->more_source = TRUE;
  b->more = TRUE;
  b->buf = apr_palloc(pool, 2 * SVN_DELTA_WINDOW_SIZE);
  b->context = svn_checksum_ctx_create(svn_checksum_md5, pool);
  b->result_pool = pool;

  *stream = svn_txdelta_stream_create(b, txdelta_next_window,
                                      txdelta_md5_digest, pool);
}



/* Functions for implementing a "target push" delta. */

/* This is the write handler for a target-push delta stream.  It reads
 * source data, buffers target data, and fires off delta windows when
 * the target data buffer is full. */
static svn_error_t *
tpush_write_handler(void *baton, const char *data, apr_size_t *len)
{
  struct tpush_baton *tb = baton;
  apr_size_t chunk_len, data_len = *len;
  apr_pool_t *pool = svn_pool_create(tb->pool);
  svn_txdelta_window_t *window;

  while (data_len > 0)
    {
      svn_pool_clear(pool);

      /* Make sure we're all full up on source data, if possible. */
      if (tb->source_len == 0 && !tb->source_done)
        {
          tb->source_len = SVN_DELTA_WINDOW_SIZE;
          SVN_ERR(svn_stream_read(tb->source, tb->buf, &tb->source_len));
          if (tb->source_len < SVN_DELTA_WINDOW_SIZE)
            tb->source_done = TRUE;
        }

      /* Copy in the target data, up to SVN_DELTA_WINDOW_SIZE. */
      chunk_len = SVN_DELTA_WINDOW_SIZE - tb->target_len;
      if (chunk_len > data_len)
        chunk_len = data_len;
      memcpy(tb->buf + tb->source_len + tb->target_len, data, chunk_len);
      data += chunk_len;
      data_len -= chunk_len;
      tb->target_len += chunk_len;

      /* If we're full of target data, compute and fire off a window. */
      if (tb->target_len == SVN_DELTA_WINDOW_SIZE)
        {
          window = compute_window(tb->buf, tb->source_len, tb->target_len,
                                  tb->source_offset, pool);
          SVN_ERR(tb->wh(window, tb->whb));
          tb->source_offset += tb->source_len;
          tb->source_len = 0;
          tb->target_len = 0;
        }
    }

  svn_pool_destroy(pool);
  return SVN_NO_ERROR;
}


/* This is the close handler for a target-push delta stream.  It sends
 * a final window if there is any buffered target data, and then sends
 * a NULL window signifying the end of the window stream. */
static svn_error_t *
tpush_close_handler(void *baton)
{
  struct tpush_baton *tb = baton;
  svn_txdelta_window_t *window;

  /* Send a final window if we have any residual target data. */
  if (tb->target_len > 0)
    {
      window = compute_window(tb->buf, tb->source_len, tb->target_len,
                              tb->source_offset, tb->pool);
      SVN_ERR(tb->wh(window, tb->whb));
    }

  /* Send a final NULL window signifying the end. */
  return tb->wh(NULL, tb->whb);
}


svn_stream_t *
svn_txdelta_target_push(svn_txdelta_window_handler_t handler,
                        void *handler_baton, svn_stream_t *source,
                        apr_pool_t *pool)
{
  struct tpush_baton *tb;
  svn_stream_t *stream;

  /* Initialize baton. */
  tb = apr_palloc(pool, sizeof(*tb));
  tb->source = source;
  tb->wh = handler;
  tb->whb = handler_baton;
  tb->pool = pool;
  tb->buf = apr_palloc(pool, 2 * SVN_DELTA_WINDOW_SIZE);
  tb->source_offset = 0;
  tb->source_len = 0;
  tb->source_done = FALSE;
  tb->target_len = 0;

  /* Create and return writable stream. */
  stream = svn_stream_create(tb, pool);
  svn_stream_set_write(stream, tpush_write_handler);
  svn_stream_set_close(stream, tpush_close_handler);
  return stream;
}



/* Functions for applying deltas.  */

/* Ensure that BUF has enough space for VIEW_LEN bytes.  */
static APR_INLINE svn_error_t *
size_buffer(char **buf, apr_size_t *buf_size,
            apr_size_t view_len, apr_pool_t *pool)
{
  if (view_len > *buf_size)
    {
      *buf_size *= 2;
      if (*buf_size < view_len)
        *buf_size = view_len;
      SVN_ERR_ASSERT(APR_ALIGN_DEFAULT(*buf_size) >= *buf_size);
      *buf = apr_palloc(pool, *buf_size);
    }

  return SVN_NO_ERROR;
}

/* Copy LEN bytes from SOURCE to TARGET, optimizing for the case where LEN
 * is often very small.  Return a pointer to the first byte after the copied
 * target range, unlike standard memcpy(), as a potential further
 * optimization for the caller.
 *
 * memcpy() is hard to tune for a wide range of buffer lengths.  Therefore,
 * it is often tuned for high throughput on large buffers and relatively
 * low latency for mid-sized buffers (tens of bytes).  However, the overhead
 * for very small buffers (<10 bytes) is still high.  Even passing the
 * parameters, for instance, may take as long as copying 3 bytes.
 *
 * Because short copy sequences seem to be a common case, at least in
 * "format 2" FSFS repositories, we copy them directly.  Larger buffer sizes
 * aren't hurt measurably by the exta 'if' clause.  */
static APR_INLINE char *
fast_memcpy(char *target, const char *source, apr_size_t len)
{
  if (len > 7)
    {
      memcpy(target, source, len);
      target += len;
    }
  else
    {
      /* memcpy is not exactly fast for small block sizes.
       * Since they are common, let's run optimized code for them. */
      const char *end = source + len;
      for (; source != end; source++)
        *(target++) = *source;
    }

  return target;
}

/* Copy LEN bytes from SOURCE to TARGET.  Unlike memmove() or memcpy(),
 * create repeating patterns if the source and target ranges overlap.
 * Return a pointer to the first byte after the copied target range.  */
static APR_INLINE char *
patterning_copy(char *target, const char *source, apr_size_t len)
{
  const char *end = source + len;

  /* On many machines, we can do "chunky" copies. */

#if SVN_UNALIGNED_ACCESS_IS_OK

  if (end + sizeof(apr_uint32_t) <= target)
    {
      /* Source and target are at least 4 bytes apart, so we can copy in
       * 4-byte chunks.  */
      for (; source + sizeof(apr_uint32_t) <= end;
           source += sizeof(apr_uint32_t),
           target += sizeof(apr_uint32_t))
      *(apr_uint32_t *)(target) = *(apr_uint32_t *)(source);
    }

#endif

  /* fall through to byte-wise copy (either for the below-chunk-size tail
   * or the whole copy) */
  for (; source != end; source++)
    *(target++) = *source;

  return target;
}

void
svn_txdelta_apply_instructions(svn_txdelta_window_t *window,
                               const char *sbuf, char *tbuf,
                               apr_size_t *tlen)
{
  const svn_txdelta_op_t *op;
  apr_size_t tpos = 0;

  for (op = window->ops; op < window->ops + window->num_ops; op++)
    {
      const apr_size_t buf_len = (op->length < *tlen - tpos
                                  ? op->length : *tlen - tpos);

      /* Check some invariants common to all instructions.  */
      assert(tpos + op->length <= window->tview_len);

      switch (op->action_code)
        {
        case svn_txdelta_source:
          /* Copy from source area.  */
          assert(op->offset + op->length <= window->sview_len);
          fast_memcpy(tbuf + tpos, sbuf + op->offset, buf_len);
          break;

        case svn_txdelta_target:
          /* Copy from target area.  We can't use memcpy() or the like
           * since we need a specific semantics for overlapping copies:
           * they must result in repeating patterns.
           * Note that most copies won't have overlapping source and
           * target ranges (they are just a result of self-compressed
           * data) but a small percentage will.  */
          assert(op->offset < tpos);
          patterning_copy(tbuf + tpos, tbuf + op->offset, buf_len);
          break;

        case svn_txdelta_new:
          /* Copy from window new area.  */
          assert(op->offset + op->length <= window->new_data->len);
          fast_memcpy(tbuf + tpos,
                      window->new_data->data + op->offset,
                      buf_len);
          break;

        default:
          assert(!"Invalid delta instruction code");
        }

      tpos += op->length;
      if (tpos >= *tlen)
        return;                 /* The buffer is full. */
    }

  /* Check that we produced the right amount of data.  */
  assert(tpos == window->tview_len);
  *tlen = tpos;
}

/* This is a private interlibrary compatibility wrapper. */
void
svn_txdelta__apply_instructions(svn_txdelta_window_t *window,
                                const char *sbuf, char *tbuf,
                                apr_size_t *tlen);
void
svn_txdelta__apply_instructions(svn_txdelta_window_t *window,
                                const char *sbuf, char *tbuf,
                                apr_size_t *tlen)
{
  svn_txdelta_apply_instructions(window, sbuf, tbuf, tlen);
}


/* Apply WINDOW to the streams given by APPL.  */
static svn_error_t *
apply_window(svn_txdelta_window_t *window, void *baton)
{
  struct apply_baton *ab = (struct apply_baton *) baton;
  apr_size_t len;
  svn_error_t *err;

  if (window == NULL)
    {
      /* We're done; just clean up.  */
      if (ab->result_digest)
        apr_md5_final(ab->result_digest, &(ab->md5_context));

      err = svn_stream_close(ab->target);
      svn_pool_destroy(ab->pool);

      return err;
    }

  /* Make sure the source view didn't slide backwards.  */
  SVN_ERR_ASSERT(window->sview_len == 0
                 || (window->sview_offset >= ab->sbuf_offset
                     && (window->sview_offset + window->sview_len
                         >= ab->sbuf_offset + ab->sbuf_len)));

  /* Make sure there's enough room in the target buffer.  */
  SVN_ERR(size_buffer(&ab->tbuf, &ab->tbuf_size, window->tview_len, ab->pool));

  /* Prepare the source buffer for reading from the input stream.  */
  if (window->sview_offset != ab->sbuf_offset
      || window->sview_len > ab->sbuf_size)
    {
      char *old_sbuf = ab->sbuf;

      /* Make sure there's enough room.  */
      SVN_ERR(size_buffer(&ab->sbuf, &ab->sbuf_size, window->sview_len,
              ab->pool));

      /* If the existing view overlaps with the new view, copy the
       * overlap to the beginning of the new buffer.  */
      if (ab->sbuf_offset + ab->sbuf_len > window->sview_offset)
        {
          apr_size_t start =
            (apr_size_t)(window->sview_offset - ab->sbuf_offset);
          memmove(ab->sbuf, old_sbuf + start, ab->sbuf_len - start);
          ab->sbuf_len -= start;
        }
      else
        ab->sbuf_len = 0;
      ab->sbuf_offset = window->sview_offset;
    }

  /* Read the remainder of the source view into the buffer.  */
  if (ab->sbuf_len < window->sview_len)
    {
      len = window->sview_len - ab->sbuf_len;
      err = svn_stream_read(ab->source, ab->sbuf + ab->sbuf_len, &len);
      if (err == SVN_NO_ERROR && len != window->sview_len - ab->sbuf_len)
        err = svn_error_create(SVN_ERR_INCOMPLETE_DATA, NULL,
                               "Delta source ended unexpectedly");
      if (err != SVN_NO_ERROR)
        return err;
      ab->sbuf_len = window->sview_len;
    }

  /* Apply the window instructions to the source view to generate
     the target view.  */
  len = window->tview_len;
  svn_txdelta_apply_instructions(window, ab->sbuf, ab->tbuf, &len);
  SVN_ERR_ASSERT(len == window->tview_len);

  /* Write out the output. */

  /* ### We've also considered just adding two (optionally null)
     arguments to svn_stream_create(): read_checksum and
     write_checksum.  Then instead of every caller updating an md5
     context when it calls svn_stream_write() or svn_stream_read(),
     streams would do it automatically, and verify the checksum in
     svn_stream_closed().  But this might be overkill for issue #689;
     so for now we just update the context here. */
  if (ab->result_digest)
    apr_md5_update(&(ab->md5_context), ab->tbuf, len);

  return svn_stream_write(ab->target, ab->tbuf, &len);
}


void
svn_txdelta_apply(svn_stream_t *source,
                  svn_stream_t *target,
                  unsigned char *result_digest,
                  const char *error_info,
                  apr_pool_t *pool,
                  svn_txdelta_window_handler_t *handler,
                  void **handler_baton)
{
  apr_pool_t *subpool = svn_pool_create(pool);
  struct apply_baton *ab;

  ab = apr_palloc(subpool, sizeof(*ab));
  ab->source = source;
  ab->target = target;
  ab->pool = subpool;
  ab->sbuf = NULL;
  ab->sbuf_size = 0;
  ab->sbuf_offset = 0;
  ab->sbuf_len = 0;
  ab->tbuf = NULL;
  ab->tbuf_size = 0;
  ab->result_digest = result_digest;

  if (result_digest)
    apr_md5_init(&(ab->md5_context));

  if (error_info)
    ab->error_info = apr_pstrdup(subpool, error_info);
  else
    ab->error_info = NULL;

  *handler = apply_window;
  *handler_baton = ab;
}



/* Convenience routines */

svn_error_t *
svn_txdelta_send_string(const svn_string_t *string,
                        svn_txdelta_window_handler_t handler,
                        void *handler_baton,
                        apr_pool_t *pool)
{
  svn_txdelta_window_t window = { 0 };
  svn_txdelta_op_t op;

  /* Build a single `new' op */
  op.action_code = svn_txdelta_new;
  op.offset = 0;
  op.length = string->len;

  /* Build a single window containing a ptr to the string. */
  window.tview_len = string->len;
  window.num_ops = 1;
  window.ops = &op;
  window.new_data = string;

  /* Push the one window at the handler. */
  SVN_ERR((*handler)(&window, handler_baton));

  /* Push a NULL at the handler, because we're done. */
  return (*handler)(NULL, handler_baton);
}

svn_error_t *svn_txdelta_send_stream(svn_stream_t *stream,
                                     svn_txdelta_window_handler_t handler,
                                     void *handler_baton,
                                     unsigned char *digest,
                                     apr_pool_t *pool)
{
  svn_txdelta_stream_t *txstream;
  svn_error_t *err;

  /* ### this is a hack. we should simply read from the stream, construct
     ### some windows, and pass those to the handler. there isn't any reason
     ### to crank up a full "diff" algorithm just to copy a stream.
     ###
     ### will fix RSN. */

  /* Create a delta stream which converts an *empty* bytestream into the
     target bytestream. */
  svn_txdelta(&txstream, svn_stream_empty(pool), stream, pool);
  err = svn_txdelta_send_txstream(txstream, handler, handler_baton, pool);

  if (digest && (! err))
    {
      const unsigned char *result_md5;
      result_md5 = svn_txdelta_md5_digest(txstream);
      /* Since err is null, result_md5 "cannot" be null. */
      memcpy(digest, result_md5, APR_MD5_DIGESTSIZE);
    }

  return err;
}

svn_error_t *svn_txdelta_send_txstream(svn_txdelta_stream_t *txstream,
                                       svn_txdelta_window_handler_t handler,
                                       void *handler_baton,
                                       apr_pool_t *pool)
{
  svn_txdelta_window_t *window;

  /* create a pool just for the windows */
  apr_pool_t *wpool = svn_pool_create(pool);

  do
    {
      /* free the window (if any) */
      svn_pool_clear(wpool);

      /* read in a single delta window */
      SVN_ERR(svn_txdelta_next_window(&window, txstream, wpool));

      /* shove it at the handler */
      SVN_ERR((*handler)(window, handler_baton));
    }
  while (window != NULL);

  svn_pool_destroy(wpool);

  return SVN_NO_ERROR;
}
