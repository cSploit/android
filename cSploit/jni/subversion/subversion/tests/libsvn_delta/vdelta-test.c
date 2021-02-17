/* vdelta-test.c -- test driver for text deltas
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

#define APR_WANT_STDIO
#include <apr_want.h>

#include <apr_general.h>
#include <assert.h>

#include "../svn_test.h"

#include "svn_delta.h"
#include "svn_error.h"
#include "svn_pools.h"

#include "../../libsvn_delta/delta.h"
#include "delta-window-test.h"

static apr_off_t
print_delta_window(const svn_txdelta_window_t *window,
                   const char *tag, int quiet, FILE *stream)
{
  if (quiet)
    return delta_window_size_estimate(window);
  else
    return delta_window_print(window, tag, stream);
}


static void
do_one_diff(apr_file_t *source_file, apr_file_t *target_file,
            int *count, apr_off_t *len,
            int quiet, apr_pool_t *pool,
            const char *tag, FILE* stream)
{
  svn_txdelta_stream_t *delta_stream = NULL;
  svn_txdelta_window_t *delta_window = NULL;
  apr_pool_t *fpool = svn_pool_create(pool);
  apr_pool_t *wpool = svn_pool_create(pool);

  *count = 0;
  *len = 0;
  svn_txdelta(&delta_stream,
              svn_stream_from_aprfile(source_file, fpool),
              svn_stream_from_aprfile(target_file, fpool),
              fpool);
  do {
    svn_error_t *err;
    err = svn_txdelta_next_window(&delta_window, delta_stream, wpool);
    if (err)
      svn_handle_error2(err, stderr, TRUE, "vdelta-test: ");
    if (delta_window != NULL)
      {
        *len += print_delta_window(delta_window, tag, quiet, stream);
        svn_pool_clear(wpool);
        ++*count;
      }
  } while (delta_window != NULL);
  fprintf(stream, "%s: (LENGTH %" APR_OFF_T_FMT " +%d)\n", tag, *len, *count);

  svn_pool_destroy(fpool);
  svn_pool_destroy(wpool);
}


static apr_file_t *
open_binary_read(const char *path, apr_pool_t *pool)
{
  apr_status_t apr_err;
  apr_file_t *fp;

  apr_err = apr_file_open(&fp, path, (APR_READ | APR_BINARY),
                          APR_OS_DEFAULT, pool);

  if (apr_err)
    {
      fprintf(stderr, "unable to open \"%s\" for reading\n", path);
      exit(1);
    }

  return fp;
}


int
main(int argc, char **argv)
{
  apr_file_t *source_file_A = NULL;
  apr_file_t *target_file_A = NULL;
  int count_A = 0;
  apr_off_t len_A = 0;

  apr_file_t *source_file_B = NULL;
  apr_file_t *target_file_B = NULL;
  int count_B = 0;
  apr_off_t len_B = 0;

  apr_pool_t *pool;
  int quiet = 0;

  if (argc > 1 && argv[1][0] == '-' && argv[1][1] == 'q')
    {
      quiet = 1;
      --argc; ++argv;
    }

  apr_initialize();
  pool = svn_pool_create(NULL);

  if (argc == 2)
    {
      target_file_A = open_binary_read(argv[1], pool);
    }
  else if (argc == 3)
    {
      source_file_A = open_binary_read(argv[1], pool);
      target_file_A = open_binary_read(argv[2], pool);
    }
  else if (argc == 4)
    {
      source_file_A = open_binary_read(argv[1], pool);
      target_file_A = open_binary_read(argv[2], pool);
      source_file_B = open_binary_read(argv[2], pool);
      target_file_B = open_binary_read(argv[3], pool);
    }
  else
    {
      fprintf(stderr,
              "Usage: vdelta-test [-q] <target>\n"
              "   or: vdelta-test [-q] <source> <target>\n"
              "   or: vdelta-test [-q] <source> <intermediate> <target>\n");
      exit(1);
    }

  do_one_diff(source_file_A, target_file_A,
              &count_A, &len_A, quiet, pool, "A ", stdout);

  if (source_file_B)
    {
      apr_pool_t *fpool = svn_pool_create(pool);
      apr_pool_t *wpool = svn_pool_create(pool);
      svn_txdelta_stream_t *stream_A = NULL;
      svn_txdelta_stream_t *stream_B = NULL;
      svn_txdelta_window_t *window_A = NULL;
      svn_txdelta_window_t *window_B = NULL;
      svn_txdelta_window_t *window_AB = NULL;
      int count_AB = 0;
      apr_off_t len_AB = 0;

      putc('\n', stdout);
      do_one_diff(source_file_B, target_file_B,
                  &count_B, &len_B, quiet, pool, "B ", stdout);

      putc('\n', stdout);

      {
        apr_off_t offset = 0;

        apr_file_seek(source_file_A, APR_SET, &offset);
        apr_file_seek(target_file_A, APR_SET, &offset);
        apr_file_seek(source_file_B, APR_SET, &offset);
        apr_file_seek(target_file_B, APR_SET, &offset);
      }

      svn_txdelta(&stream_A,
                  svn_stream_from_aprfile(source_file_A, fpool),
                  svn_stream_from_aprfile(target_file_A, fpool),
                  fpool);
      svn_txdelta(&stream_B,
                  svn_stream_from_aprfile(source_file_B, fpool),
                  svn_stream_from_aprfile(target_file_B, fpool),
                  fpool);

      for (count_AB = 0; count_AB < count_B; ++count_AB)
        {
          svn_error_t *err;

          err = svn_txdelta_next_window(&window_A, stream_A, wpool);
          if (err)
            svn_handle_error2(err, stderr, TRUE, "vdelta-test: ");
          err = svn_txdelta_next_window(&window_B, stream_B, wpool);
          if (err)
            svn_handle_error2(err, stderr, TRUE, "vdelta-test: ");

          /* Note: It's not possible that window_B is null, we already
             counted the number of windows in the second delta. */
          assert(window_A != NULL || window_B->src_ops == 0);
          if (window_B->src_ops == 0)
            {
              window_AB = window_B;
              window_AB->sview_len = 0;
            }
          else
            window_AB = svn_txdelta_compose_windows(window_A, window_B,
                                                    wpool);
          len_AB += print_delta_window(window_AB, "AB", quiet, stdout);
          svn_pool_clear(wpool);
        }

      fprintf(stdout, "AB: (LENGTH %" APR_OFF_T_FMT " +%d)\n",
              len_AB, count_AB);
    }

  if (source_file_A) apr_file_close(source_file_A);
  if (target_file_A) apr_file_close(target_file_A);
  if (source_file_B) apr_file_close(source_file_B);
  if (target_file_B) apr_file_close(source_file_B);

  svn_pool_destroy(pool);
  apr_terminate();
  exit(0);
}
