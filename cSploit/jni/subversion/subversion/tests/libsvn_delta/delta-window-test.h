/* delta-window-test.h -- utilities for delta window output
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

#ifndef SVN_DELTA_WINDOW_TEST_H
#define SVN_DELTA_WINDOW_TEST_H

#define APR_WANT_STDIO
#define APR_WANT_STRFUNC
#include <apr_want.h>

#include "svn_delta.h"
#include "svn_ctype.h"

static apr_off_t
delta_window_size_estimate(const svn_txdelta_window_t *window)
{
  apr_off_t len;
  int i;

  if (!window)
    return 0;

  /* Try to estimate the size of the delta. */
  for (i = 0, len = 0; i < window->num_ops; ++i)
    {
      apr_size_t const offset = window->ops[i].offset;
      apr_size_t const length = window->ops[i].length;
      if (window->ops[i].action_code == svn_txdelta_new)
        {
          len += 1;             /* opcode */
          len += (length > 255 ? 2 : 1);
          len += length;
        }
      else
        {
          len += 1;             /* opcode */
          len += (offset > 255 ? 2 : 1);
          len += (length > 255 ? 2 : 1);
        }
    }

  return len;
}


static apr_off_t
delta_window_print(const svn_txdelta_window_t *window,
                   const char *tag, FILE *stream)
{
  const apr_off_t len = delta_window_size_estimate(window);
  apr_off_t op_offset = 0;
  int i;

  if (!window)
    return 0;

  fprintf(stream, "%s: (WINDOW %" APR_OFF_T_FMT, tag, len);
  fprintf(stream,
          " (%" SVN_FILESIZE_T_FMT
          " %" APR_SIZE_T_FMT " %" APR_SIZE_T_FMT ")",
          window->sview_offset, window->sview_len, window->tview_len);
  for (i = 0; i < window->num_ops; ++i)
    {
      apr_size_t const offset = window->ops[i].offset;
      apr_size_t const length = window->ops[i].length;
      apr_size_t tmp;
      switch (window->ops[i].action_code)
        {
        case svn_txdelta_source:
          fprintf(stream, "\n%s:   (%" APR_OFF_T_FMT " SRC %" APR_SIZE_T_FMT
                  " %" APR_SIZE_T_FMT ")", tag, op_offset, offset, length);
          break;
        case svn_txdelta_target:
          fprintf(stream, "\n%s:   (%" APR_OFF_T_FMT " TGT %" APR_SIZE_T_FMT
                  " %" APR_SIZE_T_FMT ")", tag, op_offset, offset, length);
          break;
        case svn_txdelta_new:
          fprintf(stream, "\n%s:   (%" APR_OFF_T_FMT " NEW %"
                  APR_SIZE_T_FMT " \"", tag, op_offset, length);
          for (tmp = offset; tmp < offset + length; ++tmp)
            {
              int const dat = window->new_data->data[tmp];
              if (svn_ctype_iscntrl(dat) || !svn_ctype_isascii(dat))
                fprintf(stream, "\\%3.3o", dat & 0xff);
              else if (dat == '\\')
                fputs("\\\\", stream);
              else
                putc(dat, stream);
            }
          fputs("\")", stream);
          break;
        default:
          fprintf(stream, "\n%s:   (BAD-OP)", tag);
        }

      op_offset += length;
    }
  fputs(")\n", stream);
  return len;
}


#endif /* SVN_DELTA_WINDOW_TEST_H */
