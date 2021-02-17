/*
 * random-test.c:  Test delta generation and application using random data.
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

#define APR_WANT_STDIO
#define APR_WANT_STRFUNC
#include <apr_want.h>
#include <apr_general.h>
#include <apr_getopt.h>
#include <apr_file_io.h>

#include "../svn_test.h"

#include "svn_delta.h"
#include "svn_pools.h"
#include "svn_error.h"

#include "../../libsvn_delta/delta.h"
#include "delta-window-test.h"


#define DEFAULT_ITERATIONS 30
#define DEFAULT_MAXLEN (100 * 1024)
#define DEFAULT_DUMP_FILES 0
#define DEFAULT_PRINT_WINDOWS 0
#define SEEDS 50
#define MAXSEQ 100


/* Initialize parameters for the random tests. */
extern int test_argc;
extern const char **test_argv;

static void init_params(apr_uint32_t *seed,
                        apr_uint32_t *maxlen, int *iterations,
                        int *dump_files, int *print_windows,
                        const char **random_bytes,
                        apr_uint32_t *bytes_range,
                        apr_pool_t *pool)
{
  apr_getopt_t *opt;
  char optch;
  const char *opt_arg;
  apr_status_t status;

  *seed = (apr_uint32_t) apr_time_now();
  *maxlen = DEFAULT_MAXLEN;
  *iterations = DEFAULT_ITERATIONS;
  *dump_files = DEFAULT_DUMP_FILES;
  *print_windows = DEFAULT_PRINT_WINDOWS;
  *random_bytes = NULL;
  *bytes_range = 256;

  apr_getopt_init(&opt, pool, test_argc, test_argv);
  while (APR_SUCCESS
         == (status = apr_getopt(opt, "s:l:n:r:FW", &optch, &opt_arg)))
    {
      switch (optch)
        {
        case 's':
          *seed = atol(opt_arg);
          break;
        case 'l':
          *maxlen = atoi(opt_arg);
          break;
        case 'n':
          *iterations = atoi(opt_arg);
          break;
        case 'r':
          *random_bytes = opt_arg + 1;
          *bytes_range = strlen(*random_bytes);
          break;
        case 'F':
          *dump_files = !*dump_files;
          break;
        case 'W':
          *print_windows = !*print_windows;
          break;
        }
    }
}


/* Open a temporary file. */
static apr_file_t *
open_tempfile(const char *name_template, apr_pool_t *pool)
{
  apr_status_t apr_err;
  apr_file_t *fp = NULL;
  char *templ;

  if (!name_template)
    templ = apr_pstrdup(pool, "tempfile_XXXXXX");
  else
    templ = apr_pstrdup(pool, name_template);

  apr_err = apr_file_mktemp(&fp, templ, 0, pool);
  assert(apr_err == 0);
  assert(fp != NULL);
  return fp;
}

/* Rewind the file pointer */
static void rewind_file(apr_file_t *fp)
{
  apr_off_t offset = 0;
#ifndef NDEBUG
  apr_status_t apr_err =
#endif
    apr_file_seek(fp, APR_SET, &offset);
  assert(apr_err == 0);
  assert(offset == 0);
}


static void
dump_file_contents(apr_file_t *fp)
{
  static char file_buffer[10240];
  apr_size_t length = sizeof file_buffer;
  fputs("--------\n", stdout);
  do
    {
      apr_file_read_full(fp, file_buffer, sizeof file_buffer, &length);
      fwrite(file_buffer, 1, length, stdout);
    }
  while (length == sizeof file_buffer);
  putc('\n', stdout);
  rewind_file(fp);
}

/* Generate a temporary file containing sort-of random data.  Diffs
   between files of random data tend to be pretty boring, so we try to
   make sure there are a bunch of common substrings between two runs
   of this function with the same seedbase.  */
static apr_file_t *
generate_random_file(apr_uint32_t maxlen,
                     apr_uint32_t subseed_base,
                     apr_uint32_t *seed,
                     const char *random_bytes,
                     apr_uint32_t bytes_range,
                     int dump_files,
                     apr_pool_t *pool)
{
  static char file_buffer[10240];
  char *buf = file_buffer;
  char *const end = buf + sizeof file_buffer;

  apr_uint32_t len, seqlen;
  apr_file_t *fp;
  unsigned long r;

  fp = open_tempfile("random_XXXXXX", pool);
  len = svn_test_rand(seed) % maxlen; /* We might go over this by a bit.  */
  while (len > 0)
    {
      /* Generate a pseudo-random sequence of up to MAXSEQ bytes,
         where the seed is in the range [seedbase..seedbase+MAXSEQ-1].
         (Use our own pseudo-random number generator here to avoid
         clobbering the seed of the libc random number generator.)  */

      seqlen = svn_test_rand(seed) % MAXSEQ;
      if (seqlen > len) seqlen = len;
      len -= seqlen;
      r = subseed_base + svn_test_rand(seed) % SEEDS;
      while (seqlen-- > 0)
        {
          const int ch = (random_bytes
                          ? (unsigned)random_bytes[r % bytes_range]
                          : r % bytes_range);
          if (buf == end)
            {
              apr_size_t ignore_length;
              apr_file_write_full(fp, file_buffer, sizeof file_buffer,
                                  &ignore_length);
              buf = file_buffer;
            }

          *buf++ = ch;
          r = r * 1103515245 + 12345;
        }
    }

  if (buf > file_buffer)
    {
      apr_size_t ignore_length;
      apr_file_write_full(fp, file_buffer, buf - file_buffer, &ignore_length);
    }
  rewind_file(fp);

  if (dump_files)
    dump_file_contents(fp);

  return fp;
}

/* Compare two open files. The file positions may change. */
static svn_error_t *
compare_files(apr_file_t *f1, apr_file_t *f2, int dump_files)
{
  static char file_buffer_1[10240];
  static char file_buffer_2[10240];

  char *c1, *c2;
  apr_off_t pos = 0;
  apr_size_t len1, len2;

  rewind_file(f1);
  rewind_file(f2);

  if (dump_files)
    dump_file_contents(f2);

  do
    {
      apr_file_read_full(f1, file_buffer_1, sizeof file_buffer_1, &len1);
      apr_file_read_full(f2, file_buffer_2, sizeof file_buffer_2, &len2);

      for (c1 = file_buffer_1, c2 = file_buffer_2;
           c1 < file_buffer_1 + len1 && c2 < file_buffer_2 + len2;
           ++c1, ++c2, ++pos)
        {
          if (*c1 != *c2)
            return svn_error_createf(SVN_ERR_TEST_FAILED, NULL,
                                     "mismatch at position %"APR_OFF_T_FMT,
                                     pos);
        }

      if (len1 != len2)
        return svn_error_createf(SVN_ERR_TEST_FAILED, NULL,
                                 "unequal file sizes at position"
                                 " %"APR_OFF_T_FMT, pos);
    }
  while (len1 == sizeof file_buffer_1);
  return SVN_NO_ERROR;
}


static apr_file_t *
copy_tempfile(apr_file_t *fp, apr_pool_t *pool)
{
  static char file_buffer[10240];
  apr_file_t *newfp;
  apr_size_t length1, length2;

  newfp = open_tempfile("copy_XXXXXX", pool);

  rewind_file(fp);
  do
    {
      apr_file_read_full(fp, file_buffer, sizeof file_buffer, &length1);
      apr_file_write_full(newfp, file_buffer, length1, &length2);
      assert(length1 == length2);
    }
  while (length1 == sizeof file_buffer);

  rewind_file(fp);
  rewind_file(newfp);
  return newfp;
}



/* Implements svn_test_driver_t. */
static svn_error_t *
random_test(apr_pool_t *pool)
{
  apr_uint32_t seed, bytes_range, maxlen;
  int i, iterations, dump_files, print_windows;
  const char *random_bytes;

  /* Initialize parameters and print out the seed in case we dump core
     or something. */
  init_params(&seed, &maxlen, &iterations, &dump_files, &print_windows,
              &random_bytes, &bytes_range, pool);

  for (i = 0; i < iterations; i++)
    {
      /* Generate source and target for the delta and its application.  */
      apr_uint32_t subseed_base = svn_test_rand(&seed);
      apr_file_t *source = generate_random_file(maxlen, subseed_base, &seed,
                                                random_bytes, bytes_range,
                                                dump_files, pool);
      apr_file_t *target = generate_random_file(maxlen, subseed_base, &seed,
                                                random_bytes, bytes_range,
                                                dump_files, pool);
      apr_file_t *source_copy = copy_tempfile(source, pool);
      apr_file_t *target_regen = open_tempfile(NULL, pool);

      svn_txdelta_stream_t *txdelta_stream;
      svn_txdelta_window_handler_t handler;
      svn_stream_t *stream;
      void *handler_baton;

      /* Set up a four-stage pipeline: create a delta, convert it to
         svndiff format, parse it back into delta format, and apply it
         to a copy of the source file to see if we get the same target
         back.  */
      apr_pool_t *delta_pool = svn_pool_create(pool);

      /* Make stage 4: apply the text delta.  */
      svn_txdelta_apply(svn_stream_from_aprfile(source_copy, delta_pool),
                        svn_stream_from_aprfile(target_regen, delta_pool),
                        NULL, NULL, delta_pool, &handler, &handler_baton);

      /* Make stage 3: reparse the text delta.  */
      stream = svn_txdelta_parse_svndiff(handler, handler_baton, TRUE,
                                         delta_pool);

      /* Make stage 2: encode the text delta in svndiff format using
                       varying compression levels. */
      svn_txdelta_to_svndiff3(&handler, &handler_baton, stream, 1, i % 10,
                              delta_pool);

      /* Make stage 1: create the text delta.  */
      svn_txdelta(&txdelta_stream,
                  svn_stream_from_aprfile(source, delta_pool),
                  svn_stream_from_aprfile(target, delta_pool),
                  delta_pool);

      SVN_ERR(svn_txdelta_send_txstream(txdelta_stream,
                                        handler,
                                        handler_baton,
                                        delta_pool));

      svn_pool_destroy(delta_pool);

      SVN_ERR(compare_files(target, target_regen, dump_files));

      apr_file_close(source);
      apr_file_close(target);
      apr_file_close(source_copy);
      apr_file_close(target_regen);
    }

  return SVN_NO_ERROR;
}



/* (Note: *LAST_SEED is an output parameter.) */
static svn_error_t *
do_random_combine_test(apr_pool_t *pool,
                       apr_uint32_t *last_seed)
{
  apr_uint32_t seed, bytes_range, maxlen;
  int i, iterations, dump_files, print_windows;
  const char *random_bytes;

  /* Initialize parameters and print out the seed in case we dump core
     or something. */
  init_params(&seed, &maxlen, &iterations, &dump_files, &print_windows,
              &random_bytes, &bytes_range, pool);

  for (i = 0; i < iterations; i++)
    {
      /* Generate source and target for the delta and its application.  */
      apr_uint32_t subseed_base = svn_test_rand((*last_seed = seed, &seed));
      apr_file_t *source = generate_random_file(maxlen, subseed_base, &seed,
                                                random_bytes, bytes_range,
                                                dump_files, pool);
      apr_file_t *middle = generate_random_file(maxlen, subseed_base, &seed,
                                                random_bytes, bytes_range,
                                                dump_files, pool);
      apr_file_t *target = generate_random_file(maxlen, subseed_base, &seed,
                                                random_bytes, bytes_range,
                                                dump_files, pool);
      apr_file_t *source_copy = copy_tempfile(source, pool);
      apr_file_t *middle_copy = copy_tempfile(middle, pool);
      apr_file_t *target_regen = open_tempfile(NULL, pool);

      svn_txdelta_stream_t *txdelta_stream_A;
      svn_txdelta_stream_t *txdelta_stream_B;
      svn_txdelta_window_handler_t handler;
      svn_stream_t *stream;
      void *handler_baton;

      /* Set up a four-stage pipeline: create two deltas, combine them
         and convert the result to svndiff format, parse that back
         into delta format, and apply it to a copy of the source file
         to see if we get the same target back.  */
      apr_pool_t *delta_pool = svn_pool_create(pool);

      /* Make stage 4: apply the text delta.  */
      svn_txdelta_apply(svn_stream_from_aprfile(source_copy, delta_pool),
                        svn_stream_from_aprfile(target_regen, delta_pool),
                        NULL, NULL, delta_pool, &handler, &handler_baton);

      /* Make stage 3: reparse the text delta.  */
      stream = svn_txdelta_parse_svndiff(handler, handler_baton, TRUE,
                                         delta_pool);

      /* Make stage 2: encode the text delta in svndiff format using
                       varying compression levels. */
      svn_txdelta_to_svndiff3(&handler, &handler_baton, stream, 1, i % 10,
                              delta_pool);

      /* Make stage 1: create the text deltas.  */

      svn_txdelta(&txdelta_stream_A,
                  svn_stream_from_aprfile(source, delta_pool),
                  svn_stream_from_aprfile(middle, delta_pool),
                  delta_pool);

      svn_txdelta(&txdelta_stream_B,
                  svn_stream_from_aprfile(middle_copy, delta_pool),
                  svn_stream_from_aprfile(target, delta_pool),
                  delta_pool);

      {
        svn_txdelta_window_t *window_A;
        svn_txdelta_window_t *window_B;
        svn_txdelta_window_t *composite;
        apr_pool_t *wpool = svn_pool_create(delta_pool);

        do
          {
            SVN_ERR(svn_txdelta_next_window(&window_A, txdelta_stream_A,
                                            wpool));
            if (print_windows)
              delta_window_print(window_A, "A ", stdout);
            SVN_ERR(svn_txdelta_next_window(&window_B, txdelta_stream_B,
                                            wpool));
            if (print_windows)
              delta_window_print(window_B, "B ", stdout);
            if (!window_B)
              break;
            assert(window_A != NULL || window_B->src_ops == 0);
            if (window_B->src_ops == 0)
              {
                composite = window_B;
                composite->sview_len = 0;
              }
            else
              composite = svn_txdelta_compose_windows(window_A, window_B,
                                                      wpool);
            if (print_windows)
              delta_window_print(composite, "AB", stdout);

            /* The source view length should not be 0 if there are
               source copy ops in the window. */
            if (composite
                && composite->sview_len == 0 && composite->src_ops > 0)
              return svn_error_create
                (SVN_ERR_FS_GENERAL, NULL,
                 "combined delta window is inconsistent");

            SVN_ERR(handler(composite, handler_baton));
            svn_pool_clear(wpool);
          }
        while (composite != NULL);
        svn_pool_destroy(wpool);
      }

      svn_pool_destroy(delta_pool);

      SVN_ERR(compare_files(target, target_regen, dump_files));

      apr_file_close(source);
      apr_file_close(middle);
      apr_file_close(target);
      apr_file_close(source_copy);
      apr_file_close(middle_copy);
      apr_file_close(target_regen);
    }

  return SVN_NO_ERROR;
}

/* Implements svn_test_driver_t. */
static svn_error_t *
random_combine_test(apr_pool_t *pool)
{
  apr_uint32_t seed;
  svn_error_t *err = do_random_combine_test(pool, &seed);
  return err;
}


/* Change to 1 to enable the unit test for the delta combiner's range index: */
#if 0
#include "range-index-test.h"
#endif



/* The test table.  */

struct svn_test_descriptor_t test_funcs[] =
  {
    SVN_TEST_NULL,
    SVN_TEST_PASS2(random_test,
                   "random delta test"),
    SVN_TEST_PASS2(random_combine_test,
                   "random combine delta test"),
#ifdef SVN_RANGE_INDEX_TEST_H
    SVN_TEST_PASS2(random_range_index_test,
                   "random range index test"),
#endif
    SVN_TEST_NULL
  };
