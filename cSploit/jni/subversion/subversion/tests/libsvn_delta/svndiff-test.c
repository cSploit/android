/* svndiff-test.c -- test driver for text deltas
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

#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <apr_general.h>

#include "../svn_test.h"

#include "svn_base64.h"
#include "svn_quoprint.h"
#include "svn_pools.h"
#include "svn_delta.h"
#include "svn_error.h"


int
main(int argc, char **argv)
{
  svn_error_t *err;
  apr_status_t apr_err;
  apr_file_t *source_file;
  apr_file_t *target_file;
  svn_stream_t *stdout_stream;
  svn_txdelta_stream_t *txdelta_stream;
  svn_txdelta_window_handler_t svndiff_handler;
  svn_stream_t *encoder;
  void *svndiff_baton;
  apr_pool_t *pool;
  int version = 0;

  if (argc < 3)
    {
      printf("usage: %s source target [version]\n", argv[0]);
      exit(0);
    }

  apr_initialize();
  pool = svn_pool_create(NULL);
  apr_err = apr_file_open(&source_file, argv[1], (APR_READ | APR_BINARY),
                          APR_OS_DEFAULT, pool);
  if (apr_err)
    {
      fprintf(stderr, "unable to open \"%s\" for reading\n", argv[1]);
      exit(1);
    }

  apr_err = apr_file_open(&target_file, argv[2], (APR_READ | APR_BINARY),
                          APR_OS_DEFAULT, pool);
  if (apr_err)
    {
      fprintf(stderr, "unable to open \"%s\" for reading\n", argv[2]);
      exit(1);
    }
  if (argc == 4)
    version = atoi(argv[3]);

  svn_txdelta(&txdelta_stream,
              svn_stream_from_aprfile(source_file, pool),
              svn_stream_from_aprfile(target_file, pool),
              pool);

  err = svn_stream_for_stdout(&stdout_stream, pool);
  if (err)
    svn_handle_error2(err, stdout, TRUE, "svndiff-test: ");

#ifdef QUOPRINT_SVNDIFFS
  encoder = svn_quoprint_encode(stdout_stream, pool);
#else
  encoder = svn_base64_encode(stdout_stream, pool);
#endif
  /* use maximum compression level */
  svn_txdelta_to_svndiff3(&svndiff_handler, &svndiff_baton,
                          encoder, version, 9, pool);
  err = svn_txdelta_send_txstream(txdelta_stream,
                                  svndiff_handler,
                                  svndiff_baton,
                                  pool);
  if (err)
    svn_handle_error2(err, stdout, TRUE, "svndiff-test: ");

  apr_file_close(source_file);
  apr_file_close(target_file);
  svn_pool_destroy(pool);
  apr_terminate();
  exit(0);
}
