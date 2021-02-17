/* diff.c -- test driver for text diffs
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


#include <apr.h>
#include <apr_general.h>
#include <apr_file_io.h>

#include "svn_pools.h"
#include "svn_diff.h"
#include "svn_io.h"
#include "svn_utf.h"

static svn_error_t *
do_diff(svn_stream_t *ostream,
        const char *original,
        const char *modified,
        svn_boolean_t *has_changes,
        svn_diff_file_options_t *options,
        svn_boolean_t show_c_function,
        apr_pool_t *pool)
{
  svn_diff_t *diff;

  SVN_ERR(svn_diff_file_diff_2(&diff, original, modified, options, pool));
  *has_changes = svn_diff_contains_diffs(diff);
  return svn_diff_file_output_unified3(ostream, diff, original, modified,
                                       NULL, NULL, SVN_APR_LOCALE_CHARSET,
                                       NULL, show_c_function, pool);
}

static void
print_usage(svn_stream_t *ostream, const char *progname,
            apr_pool_t *pool)
{
  svn_error_clear(svn_stream_printf(ostream, pool,
     "Usage: %s [OPTIONS] <file1> <file2>\n"
     "\n"
     "Display the differences between <file1> and <file2> in unified diff\n"
     "format.  OPTIONS are diff extensions as described by 'svn help diff'.\n"
     "Use '--' alone to indicate that no more options follow.\n",
     progname));
}

int main(int argc, const char *argv[])
{
  apr_pool_t *pool;
  svn_stream_t *ostream;
  svn_error_t *svn_err;
  svn_boolean_t has_changes;
  svn_diff_file_options_t *diff_options;
  apr_array_header_t *options_array;
  int i;
  const char *from = NULL;
  const char *to = NULL;
  svn_boolean_t show_c_function = FALSE;
  svn_boolean_t no_more_options = FALSE;

  apr_initialize();
  atexit(apr_terminate);

  pool = svn_pool_create(NULL);

  svn_err = svn_stream_for_stdout(&ostream, pool);
  if (svn_err)
    {
      svn_handle_error2(svn_err, stdout, FALSE, "diff: ");
      return 2;
    }

  options_array = apr_array_make(pool, 0, sizeof(const char *));

  for (i = 1 ; i < argc ; i++)
    {
      if (!no_more_options && (argv[i][0] == '-'))
        {
          /* Special case: '--' means "no more options follow" */
          if (argv[i][1] == '-' && !argv[i][2])
            {
              no_more_options = TRUE;
              continue;
            }
          /* Special case: we need to detect '-p' and handle it specially */
          if (argv[i][1] == 'p' && !argv[i][2])
            {
              show_c_function = TRUE;
              continue;
            }
          APR_ARRAY_PUSH(options_array, const char *) = argv[i];
        }
      else
        {
          if (from == NULL)
            from = argv[i];
          else if (to == NULL)
            to = argv[i];
          else
            {
              print_usage(ostream, argv[0], pool);
              return 2;
            }
        }
    }

  if (!from || !to)
    {
      print_usage(ostream, argv[0], pool);
      return 2;
    }

  diff_options = svn_diff_file_options_create(pool);

  svn_err = svn_diff_file_options_parse(diff_options, options_array, pool);
  if (svn_err)
    {
      svn_handle_error2(svn_err, stdout, FALSE, "diff: ");
      return 2;
    }

  svn_err = do_diff(ostream, from, to, &has_changes,
                    diff_options, show_c_function, pool);
  if (svn_err)
    {
      svn_handle_error2(svn_err, stdout, FALSE, "diff: ");
      return 2;
    }

  return has_changes ? 1 : 0;
}
